#include <SparkFun_BNO080_Arduino_Library.h>
#include <Wire.h>
#include <math.h>

#define SDA_PIN 8
#define SCL_PIN 9
#define MOTOR_PIN 2

#define WALK_ID 6
#define RUN_ID 7
#define STAIRS_ID 8
#define IDLE_ID 4

#define LAST_TX_INTERVAL 750
#define IMU_STUCK_THRESHOLD 5

BNO080 imu1;
BNO080 imu2;

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

imuMsg msg;

uint32_t idleTime, startIdle, activeTime, startActive, goodCount, badCount;
uint16_t steps, stepOffset, lastReportedSteps;
uint8_t activityType, priorActivity;
bool goodPos;

static unsigned long lastTx = 0;
static int notReadyCount = 0;

HardwareSerial uart(Serial2);

void turnOnMotor()  { digitalWrite(MOTOR_PIN, HIGH); }
void turnOffMotor() { digitalWrite(MOTOR_PIN, LOW);  }

void computeImuCS(imuMsg *compute);

void enableReports() {
    imu1.enableRotationVector(15);
    imu1.enableStepCounter(200);
    imu1.enableActivityClassifier(50, 0x1F, activityConfidences);
    imu2.enableRotationVector(15);
}

void resetI2C() {
    unsigned long time = millis();
    Serial.println("Resetting I2C bus...");
    Wire.end();
    delay(50);
    Wire.begin(SDA_PIN, SCL_PIN);
    delay(50);
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
    roll  *= 180.0f / PI;
    pitch *= 180.0f / PI;
    yaw   *= 180.0f / PI;
}

// Returns the smallest difference between two angles in degrees
float angleDifferenceDeg(float a, float b) {
    float diff = a - b;
    while (diff >  180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    return fabs(diff);
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
    uart.begin(115200, SERIAL_8N1, 13, 12);
    delay(1000);

    pinMode(MOTOR_PIN, OUTPUT);
    digitalWrite(MOTOR_PIN, LOW);

    Wire.begin(SDA_PIN, SCL_PIN);

    if (!imu1.begin(0x4A, Wire)) {
        Serial.println("Failed to find IMU1");
        while (1) delay(10);
    }
    Serial.println("IMU1 found");

    if (!imu2.begin(0x4B, Wire)) {
        Serial.println("Failed to find IMU2");
        while (1) delay(10);
    }
    Serial.println("IMU2 found");

    enableReports();

    msg.start = 0xFF;
    steps = 0;
    stepOffset = 0;
    lastReportedSteps = 0;
    goodCount = 0;
    badCount = 0;
    activeTime = 0;
    idleTime = 0;
    goodPos = false;
    priorActivity = 0;

    Serial.println("Setup complete");
}

void loop() {
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
       //Serial.println("IMU1 not ready ");
        if (notReadyCount >= IMU_STUCK_THRESHOLD) {
            resetI2C();
        }
        delay(50);
        return;
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

    activityType = imu1.getActivityClassifier();

    // Get IMU2 data if ready
    float w2, x2, y2, z2;
    if (imu2Ready) {
        w2 = imu2.getQuatReal();
        x2 = imu2.getQuatI();
        y2 = imu2.getQuatJ();
        z2 = imu2.getQuatK();
    } else {
        //Serial.println("IMU2 not ready ");
        delay(50);
        return;
    }

    float roll1, pitch1, yaw1;
    float roll2, pitch2, yaw2;

    quatToEulerDegrees(w1, x1, y1, z1, roll1, pitch1, yaw1);
    quatToEulerDegrees(w2, x2, y2, z2, roll2, pitch2, yaw2);

    float rollDiff = angleDifferenceDeg(roll1, roll2);

    /*
    Serial.print("IMU1 Quat r:");
    Serial.print(w1, 4);
    Serial.print(" i:");
    Serial.print(x1, 4);
    Serial.print(" j:");
    Serial.print(y1, 4);
    Serial.print(" k:");
    Serial.print(z1, 4);
    Serial.print(" | Roll:");
    Serial.print(roll1, 2);
    Serial.print(" Pitch:");
    Serial.print(pitch1, 2);
    Serial.print(" Yaw:");
    Serial.print(yaw1, 2);
    Serial.print(" || IMU2 Quat r:");
    Serial.print(w2, 4);
    Serial.print(" i:");
    Serial.print(x2, 4);
    Serial.print(" j:");
    Serial.print(y2, 4);
    Serial.print(" k:");
    Serial.print(z2, 4);
    Serial.print(" | Roll:");
    Serial.print(roll2, 2);
    Serial.print(" Pitch:");
    Serial.print(pitch2, 2);
    Serial.print(" Yaw:");
    Serial.print(yaw2, 2);
    Serial.print(" | Roll Diff:");
    Serial.println(rollDiff, 2);
    Serial.print(" | Steps:");
    Serial.println(steps);
    */

    if (rollDiff > 30.0f) {
        Serial.println("slouch");
        badCount++;
        goodPos = false;
        //turnOnMotor();
    } else {
        // Serial.println("straight");
        goodCount++;
        goodPos = true;
        //turnOffMotor();
    }

    // Activity timing
    bool isActive = (activityType == WALK_ID ||
                     activityType == RUN_ID  ||
                     activityType == STAIRS_ID);
    bool wasActive = (priorActivity == WALK_ID ||
                      priorActivity == RUN_ID  ||
                      priorActivity == STAIRS_ID);

    if (isActive) {
        if (!wasActive) {
            startActive = millis();
        } else {
            activeTime += millis() - startActive;
            startActive = millis();
        }
        idleTime = 0;
        priorActivity = activityType;
    } else if (activityType == IDLE_ID) {
        if (priorActivity != IDLE_ID) {
            startIdle = millis();
        } else {
            idleTime += millis() - startIdle;
            startIdle = millis();
        }
        priorActivity = IDLE_ID;
    } else {
        idleTime = 0;
        priorActivity = activityType;
    }

    // Transmit packet once per second
    if (millis() - lastTx >= LAST_TX_INTERVAL) {
        msg.activityType  = activityType;
        msg.badPosCount   = badCount;
        msg.goodPosCount  = goodCount;
        msg.idleTime      = idleTime;
        msg.activeTime    = activeTime;
        msg.steps         = steps;
        msg.goodPos       = goodPos;
        computeImuCS(&msg);
        uart.write((uint8_t *)&msg, sizeof(msg));
        Serial.println("IMU packet transmitted");
        lastTx = millis();
    }

    delay(75);
}

void computeImuCS(imuMsg* compute) {
    uint8_t cs = 0;
    for (int i = 1; i < (int)sizeof(*compute) - 1; i++)
        cs ^= ((uint8_t *)compute)[i];
    compute->checksum = cs;
}