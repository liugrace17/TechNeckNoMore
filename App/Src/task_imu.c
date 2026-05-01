//File: task_imu.c
#include "main.h"
#include "task_imu.h"
#include "task.h"

#define RX_BUF_SIZE 23

static uint8_t imuData[RX_BUF_SIZE];

void taskIMU(void *pvParameters){
	HAL_UART_Receive_DMA(&huart1, imuData, RX_BUF_SIZE);

	while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		//Transmit over Bluetooth
		if(verifyCheckSum(imuData)){
			xSemaphoreTake(btMutex, portMAX_DELAY);
			HAL_UART_Transmit(&huart3, imuData, RX_BUF_SIZE, 100);
			xSemaphoreGive(btMutex);
		}

	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
    	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(imuTask, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

bool verifyCheckSum(uint8_t* imuData){
	uint8_t cs = 0;
	int i;
	for (i = 1; i < RX_BUF_SIZE - 1; i++){
		cs ^= imuData[i];
	}

	return (cs == imuData[RX_BUF_SIZE-1]);
}
