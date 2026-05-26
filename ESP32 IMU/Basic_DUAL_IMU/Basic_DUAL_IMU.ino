#include <Arduino.h>
#include <SparkFun_BNO080_Arduino_Library.h>
#include <Adafruit_GPS.h>
#include <Wire.h>
#include <math.h>

#define SDA_PIN 8
#define SCL_PIN 9
#define MOTOR_PIN 2

// External status LED on GPIO15.
// Wiring: GPIO15 -> 220/330 ohm resistor -> LED anode, LED cathode -> GND.
#define STATUS_LED_PIN 15

// Calibration pushbutton on GPIO4.
// Wiring: GPIO4 -> pushbutton -> GND. Uses internal pull-up, so pressed = LOW.
#define CAL_BUTTON_PIN 4

#define WALK_ID 6
#define RUN_ID 7
#define BIKE_ID 2
#define STAIRS_ID 8
#define IDLE_ID 4

#define LAST_TX_INTERVAL 750
#define IMU_STUCK_THRESHOLD 5
#define PULSE_DURATION 50
#define POSTURE_THRESHOLD_DEG 8.5f
#define BUTTON_DEBOUNCE_MS 40
#define CAL_LED_FLASH_INTERVAL_MS 250

#if STATUS_LED_PIN == MOTOR_PIN
#error "STATUS_LED_PIN must not be the same as MOTOR_PIN"
#endif

#if CAL_BUTTON_PIN == MOTOR_PIN
#error "CAL_BUTTON_PIN must not be the same as MOTOR_PIN"
#endif

BNO080 imu1;
BNO080 imu2;

TaskHandle_t imuTask;
TaskHandle_t gpsTask;
SemaphoreHandle_t btMutex;

uint8_t activityConfidences[9];

typedef struct {
  uint8_t start;
  bool goodPos;
  uint8_t activityType;
  uint32_t idleTime;
  uint32_t activeTime;
  uint32_t goodPosCount;
  uint32_t badPosCount;
  uint16_t steps;
  uint8_t checksum;
} __attribute__((packed)) imuMsg;

typedef struct {
  uint8_t start;
  int32_t latitude;
  int32_t longitude;
} __attribute__((packed)) gpsMsg;

imuMsg msg;
gpsMsg msgGPS;

uint32_t idleTime, startIdle, activeTime, startActive, goodCount, badCount;
uint16_t steps, stepOffset, lastReportedSteps;
uint8_t activityType, priorActivity;
bool goodPos;

static unsigned long lastTx = 0;
static int notReadyCount = 0;

// Calibration state. Motor is not allowed to activate until this becomes true.
static bool calibrationDone = false;
static float baselineRollDiffDeg = 0.0f;

HardwareSerial bt(Serial2);
Adafruit_GPS GPS(&Serial1);

void taskIMU(void *pvParameters);
void taskGPS(void *pvParameters);
void computeImuCS(imuMsg *compute);

void setupStatusLed() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
}

void statusLedWrite(bool on) {
  digitalWrite(STATUS_LED_PIN, on ? HIGH : LOW);
}

void updateCalibrationLed() {
  static unsigned long lastToggleTime = 0;
  static bool ledState = false;

  if (calibrationDone) {
    ledState = false;
    statusLedWrite(false);
    return;
  }

  if (millis() - lastToggleTime >= CAL_LED_FLASH_INTERVAL_MS) {
    ledState = !ledState;
    statusLedWrite(ledState);
    lastToggleTime = millis();
  }
}

void setupCalibrationButton() {
  pinMode(CAL_BUTTON_PIN, INPUT_PULLUP);
}

bool calibrationButtonPressed() {
  static bool lastStableState = HIGH;
  static bool lastReading = HIGH;
  static unsigned long lastDebounceTime = 0;

  bool reading = digitalRead(CAL_BUTTON_PIN);

  if (reading != lastReading) {
    lastDebounceTime = millis();
    lastReading = reading;
  }

  if ((millis() - lastDebounceTime) > BUTTON_DEBOUNCE_MS) {
    if (reading != lastStableState) {
      lastStableState = reading;

      // Button uses INPUT_PULLUP, so a new LOW state means a press.
      if (lastStableState == LOW) {
        return true;
      }
    }
  }

  return false;
}

void turnOnMotor() {
  // Safety guard: do not allow motor activation before calibration.
  if (!calibrationDone) {
    digitalWrite(MOTOR_PIN, LOW);
    return;
  }
  digitalWrite(MOTOR_PIN, HIGH);
}

void turnOffMotor() {
  digitalWrite(MOTOR_PIN, LOW);
}

void pulseMotor() {
  // Safety guard: do not allow motor activation before calibration.
  if (!calibrationDone) {
    turnOffMotor();
    return;
  }

  unsigned long currTime = millis();
  digitalWrite(MOTOR_PIN, HIGH);
  while ((millis() - currTime) < PULSE_DURATION) {
    vTaskDelay(pdMS_TO_TICKS(10));  // yield to scheduler during pulse
  }
  digitalWrite(MOTOR_PIN, LOW);
}

void enableReports() {
  imu1.enableRotationVector(15);
  imu1.enableStepCounter(200);
  imu2.enableRotationVector(15);
  imu2.enableActivityClassifier(50, 0x1F, activityConfidences);
}

void resetI2C() {
  unsigned long time = millis();
  Serial.println("Resetting I2C bus...");
  Wire.end();
  vTaskDelay(pdMS_TO_TICKS(50));
  Wire.begin(SDA_PIN, SCL_PIN);
  vTaskDelay(pdMS_TO_TICKS(50));
  imu1.begin(0x4A, Wire);
  imu2.begin(0x4B, Wire);
  enableReports();
  notReadyCount = 0;
  Serial.printf("Took %lu ms\n", millis() - time);
  Serial.println("I2C reset complete");
}

void quatToEulerDegrees(float w, float x, float y, float z,
                        float &roll, float &pitch, float &yaw) {
  // Roll (x-axis rotation)
  float sinr_cosp = 2.0f * (w * x + y * z);
  float cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
  roll = atan2(sinr_cosp, cosr_cosp);

  // Pitch (y-axis rotation)
  float sinp = 2.0f * (w * y - z * x);
  sinp = constrain(sinp, -1.0f, 1.0f);
  pitch = asin(sinp);

  // Yaw (z-axis rotation)
  float siny_cosp = 2.0f * (w * z + x * y);
  float cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
  yaw = atan2(siny_cosp, cosy_cosp);

  // Convert radians to degrees
  roll *= 180.0f / PI;
  pitch *= 180.0f / PI;
  yaw *= 180.0f / PI;
}

// Returns signed angle difference in degrees, wrapped to [-180, +180].
float signedAngleDifferenceDeg(float a, float b) {
  float diff = a - b;
  while (diff > 180.0f) diff -= 360.0f;
  while (diff < -180.0f) diff += 360.0f;
  return diff;
}

// Returns absolute smallest difference between two angles in degrees.
float angleDifferenceDeg(float a, float b) {
  return fabs(signedAngleDifferenceDeg(a, b));
}

/*
void translateActivity(uint8_t state) {
    switch (state) {
        case 1: Serial.print("In Vehicle"); break;
        case 2: Serial.print("Biking"); break;
        case 3: Serial.print("On Foot"); break;
        case 4: Serial.print("Still"); break;
        case 5: Serial.print("Tilting"); break;
        case 6: Serial.print("Walking"); break;
        case 7: Serial.print("Running"); break;
        case 8: Serial.print("On Stairs"); break;
    }
}
*/

void setup() {
  Serial.begin(115200);
  bt.begin(9600, SERIAL_8N1, 13, 12);
  Serial1.begin(9600, SERIAL_8N1, 17, 18);
  GPS.begin(9600);
  delay(1000);

  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);

  setupStatusLed();
  setupCalibrationButton();

  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  delay(250);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!imu1.begin(0x4A, Wire)) {
    Serial.println("Failed to find IMU1");
    while (1) {
      turnOffMotor();
      delay(10);
    }
  }
  Serial.println("IMU1 found");

  if (!imu2.begin(0x4B, Wire)) {
    Serial.println("Failed to find IMU2");
    while (1) {
      turnOffMotor();
      delay(10);
    }
  }
  Serial.println("IMU2 found");

  enableReports();

  msgGPS.start = 0xFE;

  msg.start = 0xFF;
  steps = 0;
  stepOffset = 0;
  lastReportedSteps = 0;
  goodCount = 0;
  badCount = 0;
  activeTime = 0;
  idleTime = 0;
  goodPos = true;
  priorActivity = 0;

  btMutex = xSemaphoreCreateMutex();
  xTaskCreate(taskIMU, "IMU Task", 4096, NULL, 1, &imuTask);
  xTaskCreate(taskGPS, "GPS Task", 4096, NULL, 1, &gpsTask);

  Serial.println("Setup complete");
  Serial.println("Waiting for calibration button press on GPIO4...");
}

void loop() {
}

void taskIMU(void *pvParameters) {
    unsigned long lastCalibrationReminder = 0;

    while (1) {
        // Blink the external LED while waiting for calibration.
        // Once calibrated, updateCalibrationLed() keeps the LED off.
        updateCalibrationLed();

        // Extra safety: motor must remain off until calibration has happened.
        if (!calibrationDone) {
            turnOffMotor();
        }

        // Check for IMU resets and re-enable reports
        if (imu1.hasReset()) {
            Serial.println("IMU1 reset detected");
            enableReports();
        }
        if (imu2.hasReset()) {
            Serial.println("IMU2 reset detected");
            enableReports();
        }

        bool imu1Ready = imu1.dataAvailable();
        bool imu2Ready = imu2.dataAvailable();

        // Prevent infinite loop of imu1 not ready
        if (!imu1Ready) {
            notReadyCount++;
            if (notReadyCount >= IMU_STUCK_THRESHOLD) {
                resetI2C();
            }
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        notReadyCount = 0;

        // Get IMU1 data
        float w1 = imu1.getQuatReal();
        float x1 = imu1.getQuatI();
        float y1 = imu1.getQuatJ();
        float z1 = imu1.getQuatK();

        // Step count to offset the resetting of reports
        uint16_t reportSteps = imu1.getStepCount();
        if (reportSteps < lastReportedSteps) {
            stepOffset += lastReportedSteps;
        }
        lastReportedSteps = reportSteps;
        steps = stepOffset + reportSteps;

        // Get IMU2 data if ready
        float w2, x2, y2, z2;
        if (imu2Ready) {
            w2 = imu2.getQuatReal();
            x2 = imu2.getQuatI();
            y2 = imu2.getQuatJ();
            z2 = imu2.getQuatK();
            activityType = imu2.getActivityClassifier();
        }
        else {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        float roll1, pitch1, yaw1;
        float roll2, pitch2, yaw2;

        quatToEulerDegrees(w1, x1, y1, z1, roll1, pitch1, yaw1);
        quatToEulerDegrees(w2, x2, y2, z2, roll2, pitch2, yaw2);

        // Signed roll difference lets the calibration baseline preserve direction.
        float signedRollDiff = signedAngleDifferenceDeg(roll1, roll2);

        // Pressing the button records the current roll difference as the neutral posture baseline.
        if (calibrationButtonPressed()) {
            baselineRollDiffDeg = signedRollDiff;
            if(calibrationDone){
              Serial.println("Recalibrated");
            }
            calibrationDone = true;
            goodPos = true;
            goodCount = 0;
            badCount = 0;
            turnOffMotor();
            statusLedWrite(false);  // LED turns off once calibration is complete.

            Serial.print("Calibration complete. Baseline roll diff = ");
            Serial.print(baselineRollDiffDeg, 2);
            Serial.println(" deg");
        }

        if (!calibrationDone) {
            goodPos = true;
            turnOffMotor();

            if (millis() - lastCalibrationReminder >= 1000) {
                Serial.println("Waiting for calibration: hold normal posture, then press GPIO4 button.");
                lastCalibrationReminder = millis();
            }
        }
        else {
            float calibratedRollError = angleDifferenceDeg(signedRollDiff, baselineRollDiffDeg);

            if (calibratedRollError > POSTURE_THRESHOLD_DEG) {
                Serial.print("slouch | error = ");
                Serial.println(calibratedRollError, 2);
                badCount++;
                goodPos = false;
                pulseMotor();
            }
            else {
                goodCount++;
                goodPos = true;
                turnOffMotor();
            }
        }

        // Activity timing
        bool isActive = (activityType == WALK_ID || activityType == RUN_ID || activityType == STAIRS_ID || activityType == BIKE_ID);
        bool wasActive = (priorActivity == WALK_ID || priorActivity == RUN_ID || priorActivity == STAIRS_ID || priorActivity == BIKE_ID);

        if (isActive) {
            if (!wasActive) {
                startActive = millis();
            }
            else {
                activeTime += millis() - startActive;
                startActive = millis();
            }
            idleTime = 0;
            priorActivity = activityType;
        }
        else {
            // Everything non-active is considered idle
            if (wasActive) {
                startIdle = millis();
            } else {
                idleTime += millis() - startIdle;
                startIdle = millis();
            }
            priorActivity = activityType;
        }

        // Transmit packet periodically
        if (millis() - lastTx >= LAST_TX_INTERVAL) {
            msg.activityType = activityType;
            msg.badPosCount = badCount;
            msg.goodPosCount = goodCount;
            msg.idleTime = idleTime;
            msg.activeTime = activeTime;
            msg.steps = steps;
            msg.goodPos = goodPos;
            computeImuCS(&msg);
            xSemaphoreTake(btMutex, portMAX_DELAY);
            bt.write((uint8_t *)&msg, sizeof(msg));
            xSemaphoreGive(btMutex);
            Serial.println("IMU packet transmitted");
            lastTx = millis();
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void taskGPS(void *pvParameters) {
    while (1) {
        while (GPS.available()) {
            GPS.read();
            if (GPS.newNMEAreceived()) {
                GPS.parse(GPS.lastNMEA());
            }
        }
        if (GPS.fix) {
            msgGPS.latitude = GPS.latitude_fixed;
            msgGPS.longitude = GPS.longitude_fixed;
            xSemaphoreTake(btMutex, portMAX_DELAY);
            bt.write((uint8_t *)&msgGPS, sizeof(msgGPS));
            xSemaphoreGive(btMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void computeImuCS(imuMsg *compute) {
  uint8_t cs = 0;
  for (int i = 1; i < (int)sizeof(*compute) - 1; i++) {
    cs ^= ((uint8_t *)compute)[i];
  }
  compute->checksum = cs;
}