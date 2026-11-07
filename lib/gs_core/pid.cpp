#include "pid.h"

// keep a value inside [lo, hi]
static float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void pidInit(Pid& p, float kp, float ki, float kd, float outMin, float outMax) {
  p.kp = kp;
  p.ki = ki;
  p.kd = kd;
  p.outMin = outMin;
  p.outMax = outMax;
  p.iMin = outMin;
  p.iMax = outMax;
  pidReset(p);
}

void pidReset(Pid& p) {
  p.integral = 0.0f;
  p.lastMeasured = 0.0f;
  p.first = true;
}

float pidUpdate(Pid& p, float setpoint, float measured, float dt) {
  float error = setpoint - measured;

  // integral: add up error over time, then clamp so it can't wind up
  // while the wheel is stuck or the output is already at the limit
  p.integral += p.ki * error * dt;
  p.integral = clampf(p.integral, p.iMin, p.iMax);

  // derivative on the measurement (not the error) so a setpoint jump
  // does not cause a big spike. Skipped on the first call.
  float deriv = 0.0f;
  if (!p.first && dt > 0.0f) {
    deriv = -(measured - p.lastMeasured) / dt;
  }
  p.first = false;
  p.lastMeasured = measured;

  float out = p.kp * error + p.integral + p.kd * deriv;
  return clampf(out, p.outMin, p.outMax);
}
