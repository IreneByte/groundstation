#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#include "comp_filter.h"

// mpu6050 sensor instance (on its own i2c bus, see diagram.json)
Adafruit_MPU6050 mpu;
#define IMU_SDA 18
#define IMU_SCL 19

// complementary filter state (math is in lib/gs_core/comp_filter.cpp)
static TiltFilter filter;
static unsigned long lastImuTime = 0;

void initIMU() {
    // mpu6050 is wired to 18/19, not the default 21/22
    Wire1.begin(IMU_SDA, IMU_SCL);

    // wait until mpu6050 is connected and responsive
    while (!mpu.begin(MPU6050_I2CADDR_DEFAULT, &Wire1)) {
        Serial.println("MPU6050 not connected!");
        delay(1000);
    }
    Serial.println("MPU6050 ready!");

    // set accelerometer range to +-8G
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);

    // set gyro range to +- 500 deg/s
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);

    // set filter bandwidth to 21 hz
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    // alpha 0.98: trust the gyro short term, the accel long term
    filterInit(filter, 0.98f);
}

void updateIMU() {
    // get raw sensor data events (m/s^2 and rad/s)
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    ImuSample s;
    s.ax = a.acceleration.x;
    s.ay = a.acceleration.y;
    s.az = a.acceleration.z;
    s.gx = g.gyro.x;
    s.gy = g.gyro.y;
    s.gz = g.gyro.z;

    // time since the last sample. the first call starts from the accel angles
    float dt = (millis() - lastImuTime) / 1000.0f;
    lastImuTime = millis();
    filterUpdate(filter, s, dt);
}

float getPitch() { return filter.pitchDeg; }
float getRoll() { return filter.rollDeg; }
