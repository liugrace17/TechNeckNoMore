//File: task_gps.c
#include "main.h"
#include "task_gps.h"
#include "task.h"
#include "gps_pmtk.h"
#include <stdbool.h>
#include <string.h>
#include "minmea.h"


#define GPS_BUF_RX_SIZE 128
// Need to track how many characters passed

static char gpsSentence[GPS_BUF_RX_SIZE];
static uint16_t gpsSentenceIndex = 0;

// Debut variable to read gps line
volatile char latest_gps_line[GPS_BUF_RX_SIZE];
volatile uint8_t latest_gps_line_ready = 0;

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
static bool gpsWasLogging = false;
static bool gpsEverHadFix = false;
static void btSend(const char *msg);
static void gpsProcessByte(uint8_t ch);
static bool gpsIsRmcSentence(const char *sentence);


static void gpsHandleFixStatus(const char *sentence)
{
    struct minmea_sentence_rmc rmc;

    if (!minmea_parse_rmc(&rmc, sentence)) {
        return;
    }

    if (rmc.valid) {
        gpsWasLogging = true;
        gpsEverHadFix = true;
        startLogging();
    } else {
    	stopLogging();
    	if(gpsWasLogging == true) {
    		dumpLogs();
    		gpsWasLogging = false;
    	}
    }
}
void taskGPS(void *pvParameters){
	vTaskDelay(pdMS_TO_TICKS(100));
	gpsInit();
	uint8_t ch;
    while(1){
        HAL_UART_Receive(&huart3, &ch, 1, HAL_MAX_DELAY);
        gpsProcessByte(ch);
	}
}

void sendCommand(const char *cmd){
    HAL_UART_Transmit(&huart3, (uint8_t *)cmd, strlen(cmd), 100);
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
        strncpy((char *)latest_gps_line, gpsSentence, GPS_BUF_RX_SIZE - 1);
        latest_gps_line[GPS_BUF_RX_SIZE - 1] = '\0';
        latest_gps_line_ready = 1;
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

void btSend(const char *msg) {
    HAL_UART_Transmit(&huart2, (uint8_t *)cmd, strlen(cmd), 100);
}

//Grabs all logged data and sends it over bluetooth
//Stretch goal function
void dumpLogs()
{
    uint8_t ch;

    btSend("GPS_DUMP_START\r\n");

    sendCommand(PMTK_LOCUS_DUMPLOG);

    uint32_t startTick = HAL_GetTick();

    while ((HAL_GetTick() - startTick) < 5000) {
        if (HAL_UART_Receive(&huart3, &ch, 1, 100) == HAL_OK) {
            HAL_UART_Transmit(&huart2, &ch, 1, 100);
        }
    }

    btSend("\r\nGPS_DUMP_END\r\n");
}
