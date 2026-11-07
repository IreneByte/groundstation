// Host test for lib/gs_core/pid. Build: see run_host_tests.sh
#include <cassert>
#include <cmath>
#include <cstdio>

#include "pid.h"

static const float DT = 0.02f;  // 50 Hz, same as the control task

// very simple motor model: speed moves toward GAIN * pwm with a lag
static float motorStep(float speed, float pwm) {
  const float GAIN = 0.5f;  // ticks/s per PWM step
  const float TAU = 0.15f;  // seconds
  return speed + (GAIN * pwm - speed) * (DT / TAU);
}

static void testStepResponse() {
  Pid p;
  pidInit(p, 1.5f, 4.0f, 0.02f, 0.0f, 255.0f);
  float speed = 0.0f, peak = 0.0f;
  for (int i = 0; i < 250; i++) {  // 5 seconds
    float pwm = pidUpdate(p, 60.0f, speed, DT);
    speed = motorStep(speed, pwm);
    if (speed > peak) peak = speed;
  }
  printf("  step: final %.2f ticks/s, peak %.2f\n", speed, peak);
  assert(fabsf(speed - 60.0f) < 1.0f);  // reaches the target
  assert(peak < 60.0f * 1.2f);          // less than 20% overshoot
}

static void testOutputLimit() {
  Pid p;
  pidInit(p, 10.0f, 0.0f, 0.0f, 0.0f, 255.0f);
  assert(pidUpdate(p, 1000.0f, 0.0f, DT) == 255.0f);   // never above max
  pidReset(p);
  assert(pidUpdate(p, 0.0f, 1000.0f, DT) == 0.0f);     // never below min
}

static void testIntegralClamp() {
  Pid p;
  pidInit(p, 1.0f, 4.0f, 0.0f, 0.0f, 255.0f);
  // wheel stuck at 0 for 100 s: integral must stop at the clamp
  for (int i = 0; i < 5000; i++) pidUpdate(p, 60.0f, 0.0f, DT);
  assert(p.integral <= p.iMax);
  assert(p.integral == 255.0f);
  // wheel frees up and overshoots: output has to come down fast.
  // Without the clamp the integral would be ~24000 and stay at 255.
  float out = 255.0f;
  int steps = 0;
  while (out > 100.0f && steps < 1000) {
    out = pidUpdate(p, 60.0f, 120.0f, DT);
    steps++;
  }
  printf("  clamp: output below 100 after %d steps\n", steps);
  assert(steps < 100);  // under 2 seconds
}

static void testWheelsIndependent() {
  // the old code shared one static integral between both wheels
  Pid left, right;
  pidInit(left, 1.0f, 4.0f, 0.0f, 0.0f, 255.0f);
  pidInit(right, 1.0f, 4.0f, 0.0f, 0.0f, 255.0f);
  for (int i = 0; i < 50; i++) pidUpdate(left, 60.0f, 0.0f, DT);
  assert(left.integral > 0.0f);
  assert(right.integral == 0.0f);
}

int main() {
  testStepResponse();
  testOutputLimit();
  testIntegralClamp();
  testWheelsIndependent();
  printf("test_pid: all passed\n");
  return 0;
}
