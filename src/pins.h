#pragma once
// All pin numbers in one place. Matches diagram.json (Wokwi).

// HC-SR04 ultrasonic
const int PIN_TRIG = 12;
const int PIN_ECHO = 14;

// push button to GND (uses the internal pull-up, LOW = pressed)
const int PIN_BUTTON = 13;

// DHT22 temperature
const int PIN_DHT = 23;

// L298N motor driver. ENB and IN4 used to be on GPIO34/35, but those
// are input-only on the ESP32, so they could never drive the motor.
const int PIN_ENA = 26;
const int PIN_IN1 = 25;
const int PIN_IN2 = 33;
const int PIN_ENB = 27;
const int PIN_IN3 = 32;
const int PIN_IN4 = 4;

// wheel encoders (one channel each, counts rising edges)
const int PIN_ENC_L = 16;
const int PIN_ENC_R = 17;

// MPU6050 is on its own I2C bus (Wire1). The OLED uses Wire (SDA 21, SCL 22).
const int PIN_IMU_SDA = 18;
const int PIN_IMU_SCL = 19;
