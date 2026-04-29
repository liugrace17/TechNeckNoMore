//File: task_gps.c
#include "main.h"
#include "task_gps.h"
#include "task.h"
#include "gps_pmtk.h"
#include <stdbool.h>
#include <string.h>
#include "minmea.h"

#define GPS_BUF_RX_SIZE 128
static uint8_t gpsRxBuf[GPS_BUF_RX_SIZE];
// Need to track how many characters passed

static char gpsSentence[GPS_BUF_RX_SIZE];
static uint16_t gpsSentenceIndex = 0;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

static void gpsProcessByte(uint8_t ch);
static bool gpsIsRmcSentence(const char *sentence);

void taskGPS(void *pvParameters){
	vTaskDelay(pdMS_TO_TICKS(100));
	gpsInit();
	uint8_t ch;
    while(1){
        HAL_UART_Receive(&huart2, &ch, 1, HAL_MAX_DELAY);
        gpsProcessByte(ch);
	}
}

void sendCommand(const char *cmd){
    HAL_UART_Transmit(&huart2, (uint8_t *)cmd, strlen(cmd), 100);
}


static void gpsProcessByte(uint8_t ch) {
    if (gpsSentenceIndex >= GPS_BUF_RX_SIZE - 1) {
        gpsSentenceIndex = 0;
        memset(gpsSentence, 0, GPS_BUF_RX_SIZE);
        return;
    }

    gpsSentence[gpsSentenceIndex] = (char)ch;
    gpsSentenceIndex++;

    if (ch != '\n') {
        return;
    }

    gpsSentence[gpsSentenceIndex] = '\0';

    if (gpsIsRmcSentence(gpsSentence)) {
        // GPS-only test:
        // Put a breakpoint here and inspect gpsSentence.
        volatile int gpsRmcDetected = 1;
        (void)gpsRmcDetected;
    }

    gpsSentenceIndex = 0;
    memset(gpsSentence, 0, GPS_BUF_RX_SIZE);
}

static bool gpsIsRmcSentence(const char *sentence) {
	if (sentence == NULL) {
		return false;
	}
	if (strncmp(sentence, "$GNRMC", 6) == 0 || strncmp(sentence, "$GPRMC", 6) == 0) {
		return true;
	} else {
		return false;
	}
}

void gpsInit(){
	sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
    vTaskDelay(pdMS_TO_TICKS(100));
	sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
    vTaskDelay(pdMS_TO_TICKS(100));
//	sendCommand(PMTK_LOCUS_ERASE_FLASH);

}

//Tells the GPS to start logging data due to lost bluetooth connection
void startLogging(){
	sendCommand(PMTK_LOCUS_STARTLOG);
}

//Tells the GPS to stop logging cause connection confirmed.
void stopLogging(){
	sendCommand(PMTK_LOCUS_STOPLOG);
}

//Grabs all logged data and sends it over bluetooth
//Stretch goal function
void dumpLogs(){
	sendCommand(PMTK_LOCUS_DUMPLOG);
	//This code should, ideally, dump all the stored logs from the GPS
	//and transmit over bluetooth to the raspberry pi for use.
	//Logging only begins when the bluetooth disconnects after being connected.
	//Dumping of the data should only begin once the BT reconnects.

}
