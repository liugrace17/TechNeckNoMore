//File: task_imu.c
#include <postureApp.h>
#include "main.h"
#include "task.h"
#include <string.h>

#define IMU_PACKET 22
#define GPS_PACKET 9
uint8_t rxBuf[POSS_BUF_SIZE];

void taskBT(void *pvParameters){
	HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(USART2_IRQn);
	vTaskDelay(pdMS_TO_TICKS(1000));
	__HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
	HAL_UART_Receive_DMA(&huart2, rxBuf, POSS_BUF_SIZE);
	while (1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (rxBuf[0] == 0xFF) {
			HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15);
			xTaskNotifyGive(imuTask);
		}
		else if((rxBuf[0] == '$')){
			HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
			xTaskNotifyGive(gpsTask);
		}
	}
}

void taskIMU(void *pvParameters){
	while(1){
		//Transmit over UART
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if(verifyCheckSum(rxBuf)){
			xSemaphoreTake(uartMutex, portMAX_DELAY);
			HAL_UART_Transmit(&huart1, rxBuf, IMU_PACKET, 100);
			xSemaphoreGive(uartMutex);
		}
	}
}

void taskGPS(void *pvParameters){
    while(1){
    	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    	xSemaphoreTake(uartMutex, portMAX_DELAY);
    	HAL_UART_Transmit(&huart1, rxBuf, GPS_PACKET, 100);
    	xSemaphoreGive(uartMutex);
	}
}

void sendCommand(const char *cmd){
    HAL_UART_Transmit(&huart2, (uint8_t *)cmd, strlen(cmd), 100);
    HAL_UART_Transmit(&huart2, (uint8_t *)"\r\n", 2, 100);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
    	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(imuTask, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

bool verifyCheckSum(uint8_t* imuData){
	uint8_t cs = 0;
	int i;
	for (i = 1; i < IMU_PACKET - 1; i++){
		cs ^= imuData[i];
	}

	return (cs == imuData[IMU_PACKET-1]);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        HAL_UART_AbortReceive(&huart2);
        HAL_UART_DeInit(&huart2);
        HAL_UART_Init(&huart2);
        HAL_UART_Receive_DMA(&huart2, rxBuf, POSS_BUF_SIZE);
    }
    if (huart->Instance == USART1)
    {
        HAL_UART_DeInit(&huart1);
        HAL_UART_Init(&huart1);
    }
}
