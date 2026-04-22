#include <SparkFun_BNO080_Arduino_Library.h>
#include <Wire.h>

#define SDA_PIN 8
#define SCL_PIN 9

BNO080 imu1;
BNO080 imu2;
uint8_t activityConfidences[9];

#define INT_IMU_1 1
#define INT_IMU_2 2

void setup() {
    Serial.begin(115200);
    delay(1000);

    Wire.begin(SDA_PIN, SCL_PIN);

    pinMode(INT_IMU_1, INPUT);
    
    if (!imu1.begin(0x4A, Wire, INT_IMU_1)) {
        Serial.println("Failed to find IMU1");
        while (1) delay(10);
    }
    Serial.println("IMU1 found");

    if (!imu2.begin(0x4B, Wire, INT_IMU_2)) {
        Serial.println("Failed to find IMU2");
        while (1) delay(10);
    }
    Serial.println("IMU2 found");

    imu1.enableRotationVector(20);
    // imu1.enableStepCounter(1000);
    // imu1.enableActivityClassifier(1000, 0x1F, activityConfidences);

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
    if (imu1.dataAvailable()) {
    //     // switch (imu1.getActivityClassifier()) {
    //     //     case 0xFF:
    //     //         break;
    //     //     default:
    //     //         Serial.print("Activity: ");
    //     //         translateActivity(imu1.getActivityClassifier());
    //     //         Serial.println();
    //     // }

        Serial.print("IMU1 Quat r:");
        Serial.print(imu1.getQuatReal());
        Serial.print(" i:");
        Serial.print(imu1.getQuatI());
        Serial.print(" j:");
        Serial.print(imu1.getQuatJ());
        Serial.print(" k:");
        Serial.print(imu1.getQuatK());
        Serial.print(" ");

    //     // Serial.printf("Steps: %d\n", imu1.getStepCount());
    } else {
      Serial.print("IMU1 not ready ");
    }

    if (imu2.dataAvailable()) {
        Serial.print("IMU2 Quat r:");
        Serial.print(imu2.getQuatReal());
        Serial.print(" i:");
        Serial.print(imu2.getQuatI());
        Serial.print(" j:");
        Serial.print(imu2.getQuatJ());
        Serial.print(" k:");
        Serial.println(imu2.getQuatK());
    } else {
        Serial.println("IMU2 not ready");
    }

    delay(100);
}