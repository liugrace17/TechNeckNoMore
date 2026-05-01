#ifndef TASK_IMU_H
#define TASK_IMU_H

#include "FreeRTOS.h"
#include "semphr.h"
#include <stdbool.h>

extern SemaphoreHandle_t btMutex;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;
extern TaskHandle_t imuTask;

void taskIMU(void *pvParameters);
bool verifyCheckSum(uint8_t *imuData);

#endif
