#include <Arduino.h>
#include <HardwareSerial.h>
#include <string.h>
#include <stdbool.h>

// =======================
// ESP32 GPS UART SETTINGS
// =======================
// GPS TX -> ESP32 GPIO16
// GPS RX -> ESP32 GPIO17, optional
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17
#define PI_RX_PIN 20
#define PI_TX_PIN 21
#define GPS_BAUD 9600
#define PI_BAUD 115200



HardwareSerial GPS(1);
HardwareSerial PiUART(2);


// =======================
// Your STM32 GPS logic
// =======================

#define GPS_BUF_RX_SIZE 128

static char gpsSentence[GPS_BUF_RX_SIZE];
static uint16_t gpsSentenceIndex = 0;

volatile char latest_gps_line[GPS_BUF_RX_SIZE];
volatile uint8_t latest_gps_line_ready = 0;

static unsigned long gpsDumpStartTime = 0; 
static bool gpsDumpActive = false;
static bool gpsWasLogging = false;
static bool gpsEverHadFix = false;
static unsigned long gpsProcessdumpedTime = 5000;

static void piSend(const char *msg);
static void gpsProcessByte(uint8_t ch);
static bool gpsIsRmcSentence(const char *sentence);
static void gpsHandleFixStatus(const char *sentence);

// =======================
// PMTK commands
// Copy these from your gps_pmtk.h later if needed
// =======================

#define PMTK_SET_NMEA_UPDATE_1HZ      "$PMTK220,1000*1F\r\n"
#define PMTK_SET_NMEA_OUTPUT_RMCONLY  "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n"
#define PMTK_LOCUS_STARTLOG           "$PMTK185,0*22\r\n"
#define PMTK_LOCUS_STOPLOG            "$PMTK185,1*23\r\n"
#define PMTK_LOCUS_DUMPLOG            "$PMTK622,1*29\r\n"

// =======================
// Arduino setup/loop
// =======================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32 GPS REAL DATA TEST STARTED");

  GPS.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  
  PiUART.begin(PI_BAUD, SERIAL_8N1, PI_RX_PIN, PI_TX_PIN);

  delay(100);

  gpsInit();

  piSend("GPS_TASK_STARTED\r\n");
}

void loop() {
  if (!gpsDumpActive) {
    gpsNormalProcess();
  } else {
    gpsDumpProcess();
  }
}



void sendCommand(const char *cmd) {
  GPS.print(cmd);

  Serial.print("Sent GPS command: ");
  Serial.print(cmd);
}

static void piSend(const char *msg) {
  PiUART.print(msg);
  Serial.print(msg);
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
    strncpy((char *)latest_gps_line, gpsSentence, GPS_BUF_RX_SIZE - 1);
    latest_gps_line[GPS_BUF_RX_SIZE - 1] = '\0';
    latest_gps_line_ready = 1;

    gpsHandleFixStatus(gpsSentence);

    piSend(gpsSentence);

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

static void gpsHandleFixStatus(const char *sentence) {
  /*
    Your STM32 code used minmea_parse_rmc() here.

    For now, this keeps the same behavior:
    RMC has:
      A = valid GPS fix
      V = invalid GPS fix

    Example:
      $GNRMC,....,A,...
      $GNRMC,....,V,...
  */

  bool rmcValid = false;

  if (strstr(sentence, ",A,") != NULL) {
    rmcValid = true;
  }

  if (rmcValid) {
    gpsEverHadFix = true;

    if (!gpsWasLogging) {
      piSend("GPS_FIX_VALID_START_LOGGING\r\n");
      startLogging();
      gpsWasLogging = true;
    }

  } else {
    if (gpsWasLogging) {
      piSend("GPS_FIX_INVALID_STOP_LOGGING\r\n");
      stopLogging();
      gpsWasLogging = false;


      if (gpsEverHadFix) {
        dumpLogs();
      }
    }
  }
}

void gpsInit() {
  sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  delay(100);

  sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  delay(100);
}

void startLogging() {
  sendCommand(PMTK_LOCUS_STARTLOG);
}

void stopLogging() {
  sendCommand(PMTK_LOCUS_STOPLOG);
}

void gpsNormalProcess() {
  while(GPS.available()) {
    uint8_t ch = GPS.read(); 
    gpsProcessByte(ch);
  }
}
void gpsDumpProcess() { 
  while (GPS.available()) {
    uint8_t ch = GPS.read();
    Serial.write(ch);
    PiUART.write(ch);
  }

  if ((millis() - gpsDumpStartTime) >= gpsProcessdumpedTime) { 
    piSend("\r\nGPS_DUMP_END\r\n");
    gpsDumpActive = false; 
  }
}

void dumpLogs() {
  if (gpsDumpActive) {
    return;
  }

  piSend("GPS_DUMP_START\r\n");

  sendCommand(PMTK_LOCUS_DUMPLOG);

  gpsDumpStartTime = millis();

  gpsDumpActive = true;
}