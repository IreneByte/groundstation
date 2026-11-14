#pragma once
// MPU6050 on its own I2C bus (Wire1, pins in pins.h).
#include "comp_filter.h"

// Returns false if the MPU6050 did not answer after a few tries.
bool imuInit();

// Read one accel + gyro sample. Returns false if the IMU is not running.
bool imuRead(ImuSample& s);
