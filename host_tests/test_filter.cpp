// Host test for lib/gs_core/comp_filter. Build: see run_host_tests.sh
#include <cassert>
#include <cmath>
#include <cstdio>

#include "comp_filter.h"

static const float G = 9.81f;
static const float DEG = 3.14159265f / 180.0f;

static bool near(float a, float b, float tol) { return fabsf(a - b) <= tol; }

// accel reading for a board tilted by pitch and roll (degrees), no motion
static ImuSample tilted(float pitchDeg, float rollDeg) {
  float p = pitchDeg * DEG, r = rollDeg * DEG;
  ImuSample s = {-G * sinf(p), G * cosf(p) * sinf(r), G * cosf(p) * cosf(r), 0, 0, 0};
  return s;
}

static void testAccelAngles() {
  ImuSample level = tilted(0, 0);
  // the old acos() math gave 90 deg here
  assert(near(accelPitchDeg(level), 0.0f, 0.01f));
  assert(near(accelRollDeg(level), 0.0f, 0.01f));
  assert(near(accelPitchDeg(tilted(30, 0)), 30.0f, 0.01f));
  assert(near(accelRollDeg(tilted(0, -20)), -20.0f, 0.01f));
  assert(near(accelPitchDeg(tilted(-45, 10)), -45.0f, 0.01f));
}

static void testFirstSampleNoJump() {
  TiltFilter f;
  filterInit(f, 0.98f);
  ImuSample s = tilted(10, 5);
  s.gx = 1.0f;                  // big gyro value
  filterUpdate(f, s, 1000.0f);  // huge dt like the old "t = 0" start
  assert(near(f.pitchDeg, 10.0f, 0.01f));
  assert(near(f.rollDeg, 5.0f, 0.01f));
}

static void testGyroOnly() {
  // alpha = 1: pure gyro. 10 deg/s around X for 1 s -> roll 10 deg
  TiltFilter f;
  filterInit(f, 1.0f);
  ImuSample s = tilted(0, 0);
  filterUpdate(f, s, 0.01f);
  s.gx = 10.0f * DEG;
  for (int i = 0; i < 100; i++) filterUpdate(f, s, 0.01f);
  assert(near(f.rollDeg, 10.0f, 0.05f));
  assert(near(f.pitchDeg, 0.0f, 0.01f));
}

static void testConvergesToAccel() {
  // start level, then accel says 20 deg pitch with no gyro motion.
  // With alpha 0.98 at 50 Hz the time constant is about 1 s.
  TiltFilter f;
  filterInit(f, 0.98f);
  filterUpdate(f, tilted(0, 0), 0.02f);
  ImuSample s = tilted(20, 0);
  for (int i = 0; i < 50; i++) filterUpdate(f, s, 0.02f);  // 1 s
  printf("  after 1 s: pitch %.2f (target 20)\n", f.pitchDeg);
  assert(f.pitchDeg > 10.0f && f.pitchDeg < 15.0f);  // about 63%
  for (int i = 0; i < 250; i++) filterUpdate(f, s, 0.02f);  // 5 s more
  assert(near(f.pitchDeg, 20.0f, 0.1f));
}

int main() {
  testAccelAngles();
  testFirstSampleNoJump();
  testGyroOnly();
  testConvergesToAccel();
  printf("test_filter: all passed\n");
  return 0;
}
