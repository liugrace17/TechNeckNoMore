#include <Arduino.h>
#include <HardwareSerial.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

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
struct __attribute__((packed)) GpsPacket {
  uint8_t startByte;   // 0xAA
  float latitude;
  float longitude;
  uint8_t valid;
  uint8_t checksum;
};

uint8_t calculateChecksum(uint8_t *data, size_t len) {
  uint8_t cs = 0;

  for (size_t i = 0; i < len; i++) {
    cs ^= data[i];
  }

  return cs;
}

void sendGpsPacket(float lat, float lon, bool valid) {
  GpsPacket packet;

  packet.startByte = 0xAA;
  packet.latitude = lat;
  packet.longitude = lon;
  packet.valid = valid ? 1 : 0;

  packet.checksum = 0;
  packet.checksum = calculateChecksum((uint8_t *)&packet, sizeof(GpsPacket) - 1);

  PiUART.write((uint8_t *)&packet, sizeof(GpsPacket));

  Serial.print("Sent GPS packet -> valid=");
  Serial.print(packet.valid);
  Serial.print(" lat=");
  Serial.print(packet.latitude, 6);
  Serial.print(" lon=");
  Serial.println(packet.longitude, 6);
}

static void piSend(const char *msg);
static void gpsProcessByte(uint8_t ch);
static bool gpsIsRmcSentence(const char *sentence);
static void gpsHandleFixStatus(const char *sentence);
static bool parseRmcLatLon(const char *sentence, float *lat, float *lon, bool *valid);
static float nmeaToDecimalDegrees(const char *nmeaCoord, char direction);

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

  Serial.print("RMC sentence: ");
  Serial.print(gpsSentence);

  gpsHandleFixStatus(gpsSentence);

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
  float lat = 0.0;
  float lon = 0.0;
  bool rmcValid = false;

  bool parsed = parseRmcLatLon(sentence, &lat, &lon, &rmcValid);

  if (!parsed) {
    Serial.println("Failed to parse RMC sentence");
    return;
  }

  if (rmcValid) {
    gpsEverHadFix = true;

    if (!gpsWasLogging) {
      Serial.println("GPS_FIX_VALID_START_LOGGING");
      startLogging();
      gpsWasLogging = true;
    }
  } else {
    if (gpsWasLogging) {
      Serial.println("GPS_FIX_INVALID_STOP_LOGGING");
      stopLogging();
      gpsWasLogging = false;

      if (gpsEverHadFix) {
        dumpLogs();
      }
    }
  }

  sendGpsPacket(lat, lon, rmcValid);
}

static float nmeaToDecimalDegrees(const char *nmeaCoord, char direction) {
  if (nmeaCoord == NULL || strlen(nmeaCoord) < 4) {
    return 0.0;
  }

  float raw = atof(nmeaCoord);

  int degrees = (int)(raw / 100);
  float minutes = raw - (degrees * 100);

  float decimal = degrees + (minutes / 60.0);

  if (direction == 'S' || direction == 'W') {
    decimal *= -1.0;
  }

  return decimal;
}

static bool parseRmcLatLon(const char *sentence, float *lat, float *lon, bool *valid) {
  if (sentence == NULL || lat == NULL || lon == NULL || valid == NULL) {
    return false;
  }

  char buffer[GPS_BUF_RX_SIZE];
  strncpy(buffer, sentence, GPS_BUF_RX_SIZE - 1);
  buffer[GPS_BUF_RX_SIZE - 1] = '\0';

  char *fields[16];
  int fieldCount = 0;

  char *token = strtok(buffer, ",");

  while (token != NULL && fieldCount < 16) {
    fields[fieldCount++] = token;
    token = strtok(NULL, ",");
  }

  if (fieldCount < 7) {
    return false;
  }

  char status = fields[2][0];

  *valid = (status == 'A');

  if (!(*valid)) {
    *lat = 0.0;
    *lon = 0.0;
    return true;
  }

  if (strlen(fields[3]) == 0 || strlen(fields[4]) == 0 ||
      strlen(fields[5]) == 0 || strlen(fields[6]) == 0) {
    return false;
  }

  *lat = nmeaToDecimalDegrees(fields[3], fields[4][0]);
  *lon = nmeaToDecimalDegrees(fields[5], fields[6][0]);

  return true;
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
