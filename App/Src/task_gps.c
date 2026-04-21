//File: task_gps.c
#include "main.h"
#include "task_gps.h"
#include "task.h"
#include "gps_pmtk.h"
#include "minmea.h"

void taskGPS(void* pvParameters){
	while(1){


		vTaskDelay(pdMS_TO_TICKS(100));
	}
}
