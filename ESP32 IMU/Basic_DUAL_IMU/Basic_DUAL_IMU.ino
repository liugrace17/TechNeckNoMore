#include <SparkFun_BNO080_Arduino_Library.h>
#include <Wire.h>
#include <math.h>

#define SDA_PIN 8
#define SCL_PIN 9
#define MOTOR_PIN 2

BNO080 imu1;
BNO080 imu2;
uint8_t activityConfidences[9];

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

    while (diff > 180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;

    return fabs(diff);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Setup MOTOR_PIN
    pinMode(MOTOR_PIN, OUTPUT);
    digitalWrite(MOTOR_PIN, LOW); // start OFF

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

    imu1.enableRotationVector(20);
    imu2.enableRotationVector(20);

    Serial.println("Setup complete");
}

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

void loop() {
    bool imu1Ready = imu1.dataAvailable();
    bool imu2Ready = imu2.dataAvailable();

    if (imu1Ready && imu2Ready) {
        float w1 = imu1.getQuatReal();
        float x1 = imu1.getQuatI();
        float y1 = imu1.getQuatJ();
        float z1 = imu1.getQuatK();

        float w2 = imu2.getQuatReal();
        float x2 = imu2.getQuatI();
        float y2 = imu2.getQuatJ();
        float z2 = imu2.getQuatK();

        float roll1, pitch1, yaw1;
        float roll2, pitch2, yaw2;

        quatToEulerDegrees(w1, x1, y1, z1, roll1, pitch1, yaw1);
        quatToEulerDegrees(w2, x2, y2, z2, roll2, pitch2, yaw2);

        float rollDiff = angleDifferenceDeg(roll1, roll2);

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

        if (rollDiff > 30.0f) {
            Serial.println("slouch");
        } else {
            Serial.println("straight");
        }

    } else {
        if (!imu1Ready) Serial.print("IMU1 not ready ");
        if (!imu2Ready) Serial.print("IMU2 not ready ");
        Serial.println();
    }

    // Controlling haptic motor logic 
    digitalWrite(MOTOR_PIN, HIGH); // motor ON
    delay(1000);

    digitalWrite(MOTOR_PIN, LOW);  // motor OFF
    delay(1000);
    delay(100);
}