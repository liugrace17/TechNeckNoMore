#include <SparkFun_BNO080_Arduino_Library.h>
#include <Wire.h>
#include <math.h>

#define SDA_PIN 8
#define SCL_PIN 9
#define MOTOR_PIN 2

#define SLOUCH_THRESHOLD_DEG 30.0f
#define IMU_PERIOD_MS        20
#define DEBOUNCE_SAMPLES     12   // 12 * 20ms = ~240ms of continuous slouch before buzz
#define PRINT_EVERY_N        5    // print readings every Nth sample (5 * 20ms = ~100ms)

BNO080 imu1;
BNO080 imu2;

struct PostureSample {
    float    roll1, pitch1, yaw1;
    float    roll2, pitch2, yaw2;
    float    rollDiff;
    uint32_t steps;
};

static QueueHandle_t postureQueue;

static void turnOnMotor()  { digitalWrite(MOTOR_PIN, HIGH); }
static void turnOffMotor() { digitalWrite(MOTOR_PIN, LOW);  }

static void quatToEulerDegrees(float w, float x, float y, float z,
                               float &roll, float &pitch, float &yaw) {
    float sinr_cosp = 2.0f * (w * x + y * z);
    float cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
    roll = atan2(sinr_cosp, cosr_cosp);

    float sinp = 2.0f * (w * y - z * x);
    sinp = constrain(sinp, -1.0f, 1.0f);
    pitch = asin(sinp);

    float siny_cosp = 2.0f * (w * z + x * y);
    float cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
    yaw = atan2(siny_cosp, cosy_cosp);

    roll  *= 180.0f / PI;
    pitch *= 180.0f / PI;
    yaw   *= 180.0f / PI;
}

static float angleDifferenceDeg(float a, float b) {
    float diff = a - b;
    while (diff >  180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    return fabs(diff);
}

// Polls both IMUs at the BNO080 rotation-vector report rate, computes the
// roll difference, and posts the latest sample to postureQueue.
// Owns the shared Wire bus — no other task touches I2C.
static void imuTask(void *arg) {
    const TickType_t period = pdMS_TO_TICKS(IMU_PERIOD_MS);
    TickType_t lastWake = xTaskGetTickCount();

    for (;;) {
        if (imu1.dataAvailable() && imu2.dataAvailable()) {
            PostureSample s;
            quatToEulerDegrees(imu1.getQuatReal(), imu1.getQuatI(),
                               imu1.getQuatJ(),    imu1.getQuatK(),
                               s.roll1, s.pitch1, s.yaw1);
            quatToEulerDegrees(imu2.getQuatReal(), imu2.getQuatI(),
                               imu2.getQuatJ(),    imu2.getQuatK(),
                               s.roll2, s.pitch2, s.yaw2);
            s.rollDiff = angleDifferenceDeg(s.roll1, s.roll2);
            s.steps    = imu1.getStepCount();
            xQueueOverwrite(postureQueue, &s);   // latest-wins, never blocks
        } else if (!imu1.dataAvailable() && !imu2.dataAvailable()) {
            Serial.println("BOTH BROKE");
        } else if (!imu1.dataAvailable()) {
            Serial.println("only imu1 broke");
        } else if (!imu2.dataAvailable()) {
            Serial.println("only imu2 broke");
        }
        vTaskDelayUntil(&lastWake, period);
    }
}

// Consumes posture samples, applies hysteresis so a brief lean doesn't buzz,
// and drives the motor + serial output only on state changes.
static void postureTask(void *arg) {
    PostureSample s;
    int      slouchCount = 0;
    int      printCount  = 0;
    bool     buzzing     = false;
    uint32_t lastSteps   = 0;

    for (;;) {
        if (xQueueReceive(postureQueue, &s, portMAX_DELAY) != pdTRUE) continue;

        if (s.rollDiff > SLOUCH_THRESHOLD_DEG) {
            if (slouchCount < DEBOUNCE_SAMPLES) slouchCount++;
        } else {
            slouchCount = 0;
        }

        bool shouldBuzz = (slouchCount >= DEBOUNCE_SAMPLES);
        if (shouldBuzz != buzzing) {
            buzzing = shouldBuzz;
            if (buzzing) turnOnMotor();
            else         turnOffMotor();
        }

        if (++printCount >= PRINT_EVERY_N) {
            printCount = 0;
            Serial.print("IMU1 R:"); Serial.print(s.roll1, 1);
            Serial.print(" P:");     Serial.print(s.pitch1, 1);
            Serial.print(" Y:");     Serial.print(s.yaw1, 1);
            Serial.print(" | IMU2 R:"); Serial.print(s.roll2, 1);
            Serial.print(" P:");        Serial.print(s.pitch2, 1);
            Serial.print(" Y:");        Serial.print(s.yaw2, 1);
            Serial.print(" | dR:");     Serial.print(s.rollDiff, 1);
            Serial.print(" | ");        Serial.println(buzzing ? "SLOUCH" : "straight");
        }

        if (s.steps != lastSteps) {
            lastSteps = s.steps;
            Serial.print("steps=");
            Serial.println(s.steps);
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(MOTOR_PIN, OUTPUT);
    digitalWrite(MOTOR_PIN, LOW);

    Wire.begin(SDA_PIN, SCL_PIN);

    while (!imu1.begin(0x4A, Wire)) {
        Serial.println("IMU1 (0x4A) not found, retrying...");
        delay(500);
    }
    Serial.println("IMU1 found");
    imu1.enableStepCounter(500);

    while (!imu2.begin(0x4B, Wire)) {
        Serial.println("IMU2 (0x4B) not found, retrying...");
        delay(500);
    }
    Serial.println("IMU2 found");

    imu1.enableRotationVector(IMU_PERIOD_MS);
    imu2.enableRotationVector(IMU_PERIOD_MS);

    postureQueue = xQueueCreate(1, sizeof(PostureSample));
    if (postureQueue == NULL) {
        Serial.println("Failed to create postureQueue");
        while (1) delay(10);
    }

    // Both tasks pinned to core 1 (same core as Arduino's loopTask) so I2C
    // and motor GPIO stay on one core. Core 0 is left for WiFi/BT stack.
    xTaskCreatePinnedToCore(imuTask,     "imu",     4096, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(postureTask, "posture", 2048, NULL, 2, NULL, 1);

    Serial.println("Setup complete");
}

void loop() {
    // All work happens in the tasks above; kill the default loopTask.
    vTaskDelete(NULL);
}
