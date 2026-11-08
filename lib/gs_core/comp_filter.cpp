#include "comp_filter.h"

#include <cmath>

static const float RAD_TO_DEG_F = 57.2957795f;

void filterInit(TiltFilter& f, float alpha) {
  f.alpha = alpha;
  f.pitchDeg = 0.0f;
  f.rollDeg = 0.0f;
  f.started = false;
}

// pitch = nose up/down = rotation around the Y axis
float accelPitchDeg(const ImuSample& s) {
  return atan2f(-s.ax, sqrtf(s.ay * s.ay + s.az * s.az)) * RAD_TO_DEG_F;
}

// roll = tip left/right = rotation around the X axis
float accelRollDeg(const ImuSample& s) {
  return atan2f(s.ay, s.az) * RAD_TO_DEG_F;
}

void filterUpdate(TiltFilter& f, const ImuSample& s, float dt) {
  float accPitch = accelPitchDeg(s);
  float accRoll = accelRollDeg(s);

  // first sample or a strange dt: start from the accel angles
  if (!f.started || dt <= 0.0f || dt > 0.5f) {
    f.pitchDeg = accPitch;
    f.rollDeg = accRoll;
    f.started = true;
    return;
  }

  // gyro rates in deg/s. gy turns pitch, gx turns roll.
  float pitchRate = s.gy * RAD_TO_DEG_F;
  float rollRate = s.gx * RAD_TO_DEG_F;

  f.pitchDeg = f.alpha * (f.pitchDeg + pitchRate * dt) + (1.0f - f.alpha) * accPitch;
  f.rollDeg = f.alpha * (f.rollDeg + rollRate * dt) + (1.0f - f.alpha) * accRoll;
}
