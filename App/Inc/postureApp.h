#ifndef POSTUREAPP_H
#define POSTUREAPP_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "gps_pmtk.h"
#include "stm32f4xx_hal.h"
#include <stdbool.h>

#define POSS_BUF_SIZE 22

extern uint8_t rxBuf[POSS_BUF_SIZE];

extern SemaphoreHandle_t uartMutex;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern TaskHandle_t imuTask;
extern TaskHandle_t btTask;
extern TaskHandle_t gpsTask;
extern uint16_t rxBufSize;

void taskBT(void *pvParameters);
void taskIMU(void *pvParameters);
bool verifyCheckSum(uint8_t *imuData);

void taskGPS(void *pvParameters);

void gpsInit();
void sendCommand(const char *cmd);

void startLogging();
void stopLogging();
void dumpLogs();

#endif
