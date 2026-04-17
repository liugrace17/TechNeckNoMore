/*
 * Copyright 2018-23 CEVA, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License and 
 * any applicable agreements you may have with CEVA, Inc.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * I2C-based HALs for SH2 and DFU.
 * Modified for STM32F4 Discovery board with two BNO085 IMUs.
 * IMU1: address 0x4A, INT on PA10
 * IMU2: address 0x4B, INT on PA9
 * Shared I2C1 on PB8 (SCL) / PB9 (SDA)
 */

#include "i2c_hal.h"

#include "sh2_hal_init.h"
#include "sh2_hal.h"
#include "sh2_err.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_hal_i2c.h"

#define RSTN_PORT       GPIOE
#define RSTN_PIN        GPIO_PIN_0

#define BOOTN_PORT      GPIOE
#define BOOTN_PIN       GPIO_PIN_1

#define PS0_WAKEN_PORT  GPIOE
#define PS0_WAKEN_PIN   GPIO_PIN_2

#define PS1_PORT        GPIOE
#define PS1_PIN         GPIO_PIN_3

#define INTN_PORT       GPIOA
#define INTN_PIN        GPIO_PIN_10

// Keep reset asserted this long.
// (Some targets have a long RC decay on reset.)
#define RESET_DELAY_US (10000)

// Wait up to this long to see first interrupt from SH
#define START_DELAY_US (4000000)

// Wait this long before assuming bootloader is ready
#define DFU_BOOT_DELAY_US (3000000)

// Initial read of I2C bus is this long.  This reads the full header, then
// a subsequent read will read the full payload (along with a new header.)
#define READ_LEN (4)

// A very brief wait before writes to avoid NAK
#define I2C_WRITE_DELAY_US (10)

// ------------------------------------------------------------------------
// Private types

enum BusState_e {
    BUS_INIT,
    BUS_IDLE,
    BUS_READING_LEN,
    BUS_GOT_LEN,
    BUS_READING_TRANSFER,
    BUS_WRITING,
    BUS_READING_DFU,
    BUS_WRITING_DFU,
};

// ------------------------------------------------------------------------
// Private data

// isOpen[0] = IMU1 (SH2), isOpen[1] = IMU2 (DFU)
static bool isOpen[2] = {false, false};

// Timer handle
TIM_HandleTypeDef tim2;

// I2C Peripheral, I2C1
I2C_HandleTypeDef i2c;

static volatile enum BusState_e i2cBusState;

volatile uint32_t rxTimestamp_uS;

// Receive Buffer
static uint8_t rxBuf[SH2_HAL_MAX_TRANSFER_IN];
static volatile uint32_t rxBufLen;
static uint8_t hdrBuf[READ_LEN];
static volatile uint32_t hdrBufLen;
static uint16_t payloadLen;

// Transmit buffer
static uint8_t txBuf[SH2_HAL_MAX_TRANSFER_OUT];
static uint32_t txBufLen;

// True after INTN observed, until read starts
static bool rxDataReady;

#define MAX_RETRIES (2)
static int i2cRetries = 0;

// I2C Addr (in 7 MSB positions)
static uint16_t i2cAddr;

// True between asserting reset and seeing first INTN assertion
static volatile bool inReset;

// ------------------------------------------------------------------------
// Private methods

static void enableInts(void)
{
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
}

static void disableInts(void)
{
    HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);
    HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
    HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
}

static void enableI2cInts(void)
{
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
}

static void disableI2cInts(void)
{
    HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);
    HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
}

static void hal_init_gpio(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    /* Configure PS0_WAKEN */
    HAL_GPIO_WritePin(PS0_WAKEN_PORT, PS0_WAKEN_PIN, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = PS0_WAKEN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(PS0_WAKEN_PORT, &GPIO_InitStruct);

    /* Configure PS1 */
    HAL_GPIO_WritePin(PS1_PORT, PS1_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = PS1_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(PS1_PORT, &GPIO_InitStruct);

    /* Configure RSTN */
    HAL_GPIO_WritePin(RSTN_PORT, RSTN_PIN, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = RSTN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RSTN_PORT, &GPIO_InitStruct);

    /* Configure BOOTN */
    HAL_GPIO_WritePin(BOOTN_PORT, BOOTN_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = BOOTN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BOOTN_PORT, &GPIO_InitStruct);

    /* Configure INTN - IMU1 on PA10 */
    GPIO_InitStruct.Pin = INTN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(INTN_PORT, &GPIO_InitStruct);

    /* Configure INTN2 - IMU2 on PA9 */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* EXTI interrupt init */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
}

static void hal_init_i2c(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    // PB8 : I2C1_SCL
    // PB9 : I2C1_SDA
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    __HAL_RCC_I2C1_CLK_ENABLE();

    i2c.Instance = I2C1;
    i2c.Init.ClockSpeed = 400000;
    i2c.Init.DutyCycle = I2C_DUTYCYCLE_2;
    i2c.Init.OwnAddress1 = 0;
    i2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    i2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLED;
    i2c.Init.OwnAddress2 = 0;
    i2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLED;
    i2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLED;

    HAL_I2C_Init(&i2c);

    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 5, 0);
    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 5, 0);
}

static void hal_init_timer(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();

    uint32_t timerClk = 2 * HAL_RCC_GetPCLK1Freq();
    uint32_t prescaler = (timerClk / 1000000) - 1;

    tim2.Instance = TIM2;
    tim2.Init.Period = 0xFFFFFFFF;
    tim2.Init.Prescaler = prescaler;
    tim2.Init.ClockDivision = 0;
    tim2.Init.CounterMode = TIM_COUNTERMODE_UP;

    HAL_TIM_Base_Init(&tim2);
    HAL_TIM_Base_Start(&tim2);
}

static void hal_init_hw(void)
{
    hal_init_timer();
    hal_init_gpio();
    hal_init_i2c();
}

static void bootn(bool state)
{
    HAL_GPIO_WritePin(BOOTN_PORT, BOOTN_PIN,
                      state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void rstn(bool state)
{
    HAL_GPIO_WritePin(RSTN_PORT, RSTN_PIN,
                      state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void ps0_waken(bool state)
{
    HAL_GPIO_WritePin(PS0_WAKEN_PORT, PS0_WAKEN_PIN,
                      state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void ps1(bool state)
{
    HAL_GPIO_WritePin(PS1_PORT, PS1_PIN,
                      state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static uint32_t timeNowUs(void)
{
    return __HAL_TIM_GET_COUNTER(&tim2);
}

static void delay_us(uint32_t t)
{
    uint32_t now = timeNowUs();
    uint32_t start = now;
    while ((now - start) < t)
    {
        now = timeNowUs();
    }
}

static void reset_delay_us(uint32_t t)
{
    uint32_t now = timeNowUs();
    uint32_t start = now;
    while (((now - start) < t) && (inReset))
    {
        now = timeNowUs();
    }
}

// ----------------------------------------------------------------------------------
// Callbacks for ISR, I2C Operations
// ----------------------------------------------------------------------------------

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *pI2c)
{
    enum BusState_e busState = i2cBusState;

    if (busState == BUS_READING_LEN)
    {
        uint16_t len = (hdrBuf[0] + (hdrBuf[1] << 8)) & ~0x8000;
        if (len > sizeof(rxBuf))
        {
            payloadLen = sizeof(rxBuf);
        }
        else
        {
            payloadLen = len;
        }
        hdrBufLen = READ_LEN;
        i2cBusState = BUS_GOT_LEN;
    }
    else if (busState == BUS_READING_TRANSFER)
    {
        rxBufLen = payloadLen;
        i2cBusState = BUS_IDLE;
    }
    else if (busState == BUS_READING_DFU)
    {
        hdrBufLen = 0;
        rxBufLen = payloadLen;
        i2cBusState = BUS_IDLE;
    }
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *i2c)
{
    enum BusState_e busState = i2cBusState;

    if (busState == BUS_WRITING)
    {
        i2cBusState = BUS_IDLE;
    }
    else if (busState == BUS_WRITING_DFU)
    {
        i2cBusState = BUS_IDLE;
    }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *i2c)
{
    bool abort = true;

    if (i2cRetries < MAX_RETRIES)
    {
        i2cRetries++;

        switch (i2cBusState)
        {
            case BUS_WRITING:
            case BUS_WRITING_DFU:
                delay_us(I2C_WRITE_DELAY_US);
                HAL_I2C_Master_Transmit_IT(i2c, i2cAddr, txBuf, txBufLen);
                abort = false;
                break;
            case BUS_READING_LEN:
                HAL_I2C_Master_Receive_IT(i2c, i2cAddr, hdrBuf, READ_LEN);
                abort = false;
                break;
            case BUS_READING_TRANSFER:
            case BUS_READING_DFU:
                HAL_I2C_Master_Receive_IT(i2c, i2cAddr, rxBuf, payloadLen);
                abort = false;
                break;
            default:
                break;
        }
    }

    if (abort)
    {
        hdrBufLen = 0;
        rxBufLen = 0;
        txBufLen = 0;
        i2cBusState = BUS_IDLE;
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t n)
{
    enum BusState_e busState = i2cBusState;

    if (busState == BUS_INIT)
    {
        return;
    }

    inReset = false;

    if ((busState == BUS_IDLE) && (hdrBufLen == 0) && (rxBufLen == 0))
    {
        rxTimestamp_uS = timeNowUs();
        i2cRetries = 0;
        i2cBusState = BUS_READING_LEN;
        HAL_I2C_Master_Receive_IT(&i2c, i2cAddr, hdrBuf, READ_LEN);
    }
    else if ((busState == BUS_GOT_LEN) && (rxBufLen == 0))
    {
        i2cRetries = 0;
        i2cBusState = BUS_READING_TRANSFER;
        HAL_I2C_Master_Receive_IT(&i2c, i2cAddr, rxBuf, payloadLen);
    }
    else
    {
        rxDataReady = true;
    }
}

// IMU1 INT on PA10
void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
}

// IMU2 INT on PA9
void EXTI9_5_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
}

void I2C1_EV_IRQHandler(void)
{
    HAL_I2C_EV_IRQHandler(&i2c);
}

void I2C1_ER_IRQHandler(void)
{
    HAL_I2C_ER_IRQHandler(&i2c);
}

// ------------------------------------------------------------------------
// SH2 HAL Methods

static int shtp_i2c_hal_open(sh2_Hal_t *self_)
{
    i2c_hal_t *self = (i2c_hal_t *)self_;

    if (isOpen[0])
    {
        return SH2_ERR;
    }

    i2cBusState = BUS_INIT;
    i2cAddr = self->i2c_addr << 1;
    isOpen[0] = true;

    hal_init_hw();

    rstn(false);
    inReset = true;
    i2cBusState = BUS_IDLE;

    rxBufLen = 0;
    hdrBufLen = 0;
    rxDataReady = false;

    ps0_waken(false);
    ps1(false);
    bootn(!self->dfu);

    delay_us(RESET_DELAY_US);

    enableInts();

    rstn(true);

    reset_delay_us(START_DELAY_US);

    bootn(true);

    return SH2_OK;
}

static void shtp_i2c_hal_close(sh2_Hal_t *self_)
{
    i2cBusState = BUS_INIT;
    delay_us(1000);

    rstn(false);
    bootn(true);

    HAL_I2C_DeInit(&i2c);

    disableInts();

    __HAL_TIM_DISABLE(&tim2);

    isOpen[0] = false;
}

static int shtp_i2c_hal_read(sh2_Hal_t *self_, uint8_t *pBuffer, unsigned len, uint32_t *t)
{
    int retval = 0;

    disableInts();

    enum BusState_e busState = i2cBusState;

    if (hdrBufLen > 0)
    {
        if (len < hdrBufLen)
        {
            hdrBufLen = 0;
            retval = SH2_ERR_BAD_PARAM;
        }
        else
        {
            memcpy(pBuffer, hdrBuf, hdrBufLen);
            retval = hdrBufLen;
            hdrBufLen = 0;
            *t = rxTimestamp_uS;
        }
    }
    else if (rxBufLen > 0)
    {
        if (len < rxBufLen)
        {
            rxBufLen = 0;
            retval = SH2_ERR_BAD_PARAM;
        }
        else
        {
            memcpy(pBuffer, rxBuf, rxBufLen);
            retval = rxBufLen;
            rxBufLen = 0;
            *t = rxTimestamp_uS;
        }
    }

    enableInts();

    if (rxDataReady)
    {
        if ((busState == BUS_IDLE) && (hdrBufLen == 0))
        {
            i2cRetries = 0;
            rxDataReady = false;
            i2cBusState = BUS_READING_LEN;
            HAL_I2C_Master_Receive_IT(&i2c, i2cAddr, hdrBuf, READ_LEN);
        }
        else if ((busState == BUS_GOT_LEN) && (rxBufLen == 0))
        {
            memcpy(pBuffer, hdrBuf, READ_LEN);
            retval = READ_LEN;
            hdrBufLen = 0;
            *t = rxTimestamp_uS;

            i2cRetries = 0;
            rxDataReady = false;
            i2cBusState = BUS_READING_TRANSFER;
            HAL_I2C_Master_Receive_IT(&i2c, i2cAddr, rxBuf, payloadLen);
        }
    }

    return retval;
}

static int shtp_i2c_hal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len)
{
    int retval = 0;

    if ((pBuffer == 0) || (len == 0) || (len > sizeof(txBuf)))
    {
        return SH2_ERR_BAD_PARAM;
    }

    disableI2cInts();

    if (i2cBusState == BUS_IDLE)
    {
        i2cBusState = BUS_WRITING;
        i2cRetries = 0;
        txBufLen = len;
        memcpy(txBuf, pBuffer, len);
        delay_us(I2C_WRITE_DELAY_US);
        HAL_I2C_Master_Transmit_IT(&i2c, i2cAddr, txBuf, txBufLen);
        retval = len;
    }

    enableI2cInts();

    return retval;
}

static uint32_t shtp_i2c_hal_getTimeUs(sh2_Hal_t *self)
{
    return timeNowUs();
}

// ------------------------------------------------------------------------
// DFU HAL Methods

static int bno_dfu_i2c_hal_open(sh2_Hal_t *self_)
{
    i2c_hal_t *self = (i2c_hal_t *)self_;

    if (isOpen[1])
    {
        return SH2_ERR;
    }

    i2cBusState = BUS_INIT;
    i2cAddr = self->i2c_addr << 1;
    isOpen[1] = true;

    hal_init_hw();

    rstn(false);
    inReset = true;

    delay_us(RESET_DELAY_US);

    rxBufLen = 0;
    hdrBufLen = 0;
    rxDataReady = false;

    i2cBusState = BUS_IDLE;

    enableI2cInts();

    ps0_waken(false);
    ps1(false);
    bootn(false);

    rstn(true);

    delay_us(DFU_BOOT_DELAY_US);

    return SH2_OK;
}

static void bno_dfu_i2c_hal_close(sh2_Hal_t *self)
{
    rstn(false);

    disableInts();

    HAL_I2C_DeInit(&i2c);

    __HAL_TIM_DISABLE(&tim2);

    isOpen[1] = false;
}

static int bno_dfu_i2c_hal_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t)
{
    int retval = 0;

    enum BusState_e busState = i2cBusState;

    if ((busState != BUS_READING_DFU) && (rxBufLen > 0))
    {
        if (len < rxBufLen)
        {
            rxBufLen = 0;
            retval = SH2_ERR_BAD_PARAM;
        }
        else
        {
            memcpy(pBuffer, rxBuf, rxBufLen);
            retval = rxBufLen;
            *t = rxTimestamp_uS;
            rxBufLen = 0;
        }
    }
    else
    {
        if (busState == BUS_IDLE)
        {
            i2cRetries = 0;
            payloadLen = len;
            i2cBusState = BUS_READING_DFU;
            HAL_I2C_Master_Receive_IT(&i2c, i2cAddr, rxBuf, payloadLen);
        }
    }

    return retval;
}

static int bno_dfu_i2c_hal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len)
{
    int retval = 0;

    if ((pBuffer == 0) || (len == 0) || (len > sizeof(txBuf)))
    {
        return SH2_ERR_BAD_PARAM;
    }

    disableI2cInts();

    enum BusState_e busState = i2cBusState;

    if (busState == BUS_IDLE)
    {
        i2cBusState = BUS_WRITING_DFU;
        i2cRetries = 0;
        txBufLen = len;
        memcpy(txBuf, pBuffer, len);
        HAL_I2C_Master_Transmit_IT(&i2c, i2cAddr, txBuf, txBufLen);
        retval = len;
    }

    enableI2cInts();

    return retval;
}

static uint32_t bno_dfu_i2c_hal_getTimeUs(sh2_Hal_t *self)
{
    return timeNowUs();
}

// ------------------------------------------------------------------------
// Public init functions

sh2_Hal_t *shtp_i2c_hal_init(i2c_hal_t *pHal, bool dfu, uint8_t addr)
{
    pHal->dfu = dfu;
    pHal->i2c_addr = addr;

    pHal->sh2_hal.open = shtp_i2c_hal_open;
    pHal->sh2_hal.close = shtp_i2c_hal_close;
    pHal->sh2_hal.read = shtp_i2c_hal_read;
    pHal->sh2_hal.write = shtp_i2c_hal_write;
    pHal->sh2_hal.getTimeUs = shtp_i2c_hal_getTimeUs;

    return &pHal->sh2_hal;
}

sh2_Hal_t *bno_dfu_i2c_hal_init(i2c_hal_t *pHal, bool dfu, uint8_t addr)
{
    pHal->dfu = dfu;
    pHal->i2c_addr = addr;

    pHal->sh2_hal.open = bno_dfu_i2c_hal_open;
    pHal->sh2_hal.close = bno_dfu_i2c_hal_close;
    pHal->sh2_hal.read = bno_dfu_i2c_hal_read;
    pHal->sh2_hal.write = bno_dfu_i2c_hal_write;
    pHal->sh2_hal.getTimeUs = bno_dfu_i2c_hal_getTimeUs;

    return &pHal->sh2_hal;
}
