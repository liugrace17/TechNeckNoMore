// File: task_gps.h

#ifndef TASK_GPS_H
#define TASK_GPS_H

#include "FreeRTOS.h"
#include "semphr.h"
#include <stdint.h>
#include <stdbool.h>

#define GPS_BUF_RX_SIZE 96

extern SemaphoreHandle_t btMutex;
extern TaskHandle_t gpsTask;

void taskGPS(void *pvParameters);

void gpsInit();
void sendCommand(const char *cmd);

void startLogging();
void stopLogging();
void dumpLogs();

#endif // TASK_GPS_H
