#define _USE_MATH_DEFINES

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <cmath>

// mpu6050 sensor instance
Adafruit_MPU6050 mpu;

void initIMU() {
    // wait until mpu6050 is connected and responsive
    while (!mpu.begin()) {
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

    // add a delay for sensor stability
    delay(100);
}

void processIMU() {
    // get raw sensor data events
    sensors_event_t a, g, temp;

    static double roll, pitch;
    double ax_mag, ay_mag, az_mag, a_mag;
    double ax_deg, ay_deg;
    double gx_deg, gy_deg;
    static unsigned long t;
    float dt, alpha;

    mpu.getEvent(&a, &g, &temp);

    // accelerometer magnitudes in m/s^2
    ax_mag = a.acceleration.x;
    ay_mag = a.acceleration.y;
    az_mag = a.acceleration.z;

    a_mag = sqrt(pow(ax_mag, 2) + pow(ay_mag, 2) + pow(az_mag, 2));

    // gyroscope rates converted to deg/s
    gx_deg = g.gyro.x * 180.0 / M_PI;
    gy_deg = g.gyro.y * 180.0 / M_PI;

    dt = (float)(millis() - t) / 1000.0;
    t = millis();

    // perform sensor fusion calculation
    alpha = 0.98f;

    ax_deg = acos(ax_mag / a_mag) * 180.0 / M_PI;
    ay_deg = acos(ay_mag / a_mag) * 180.0 / M_PI;

    pitch = alpha * (pitch + gx_deg * dt) + (1.0f - alpha) * ax_deg;
    roll = alpha * (roll + gy_deg * dt) + (1.0f - alpha) * ay_deg;

    delay(100);
}