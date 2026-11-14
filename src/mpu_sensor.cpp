#include "mpu_sensor.h"

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <Wire.h>

#include "pins.h"

static Adafruit_MPU6050 mpu;
static bool s_imuOk = false;

bool imuInit() {
  Wire1.begin(PIN_IMU_SDA, PIN_IMU_SCL);

  // try a few times instead of waiting forever
  for (int i = 0; i < 5 && !s_imuOk; i++) {
    s_imuOk = mpu.begin(MPU6050_I2CADDR_DEFAULT, &Wire1);
    if (!s_imuOk) delay(200);
  }
  if (!s_imuOk) {
    Serial.println("#IMU MPU6050 not found, tilt stays 0");
    return false;
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);  // +-8 g
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);       // +-500 deg/s
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);    // on-chip low-pass
  Serial.println("#IMU MPU6050 ready");
  return true;
}

bool imuRead(ImuSample& s) {
  if (!s_imuOk) return false;
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Adafruit gives m/s^2 and rad/s, which is what the filter wants
  s.ax = a.acceleration.x;
  s.ay = a.acceleration.y;
  s.az = a.acceleration.z;
  s.gx = g.gyro.x;
  s.gy = g.gyro.y;
  s.gz = g.gyro.z;
  return true;
}
