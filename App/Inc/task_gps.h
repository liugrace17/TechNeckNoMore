// File: task_gps.h

#ifndef TASK_GPS_H
#define TASK_GPS_H

#include "FreeRTOS.h"
#include "semphr.h"
#include <stdint.h>
#include <stdbool.h>

void taskGPS(void *pvParameters);

void gpsInit(void);
void sendCommand(const char *cmd);

void startLogging(void);
void stopLogging(void);
void dumpLogs(void);

#endif // TASK_GPS_H