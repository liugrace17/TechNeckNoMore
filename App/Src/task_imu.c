#include "main.h"
#include "task_imu.h"
#include "FreeRTOS.h"
#include "task.h"

void taskIMU(void* pvParameters){
	while (1){


		vTaskDelay(pdMS_TO_TICKS(100));
	}
}


