// Host test for lib/gs_core/fault_checks. Build: see run_host_tests.sh
#include <cassert>
#include <cmath>
#include <cstdio>

#include "fault_checks.h"

// everything fine: no obstacle (0 = no echo), 22 C, level, fresh ping
static SensorInputs ok(uint32_t now) {
  SensorInputs in = {now, 0, 22.0f, 0.0f, 0.0f, now};
  return in;
}

static void testAllClear() {
  FaultChecker c;
  checkerInit(c);
  assert(checkFaults(c, ok(5000)) == FAULT_NONE);
  assert(checkWarnings(ok(5000)) == WARN_NONE);
}

static void testDistance() {
  FaultChecker c;
  checkerInit(c);
  SensorInputs in = ok(1000);
  in.distanceCm = 10;
  assert(checkFaults(c, in) == FAULT_NONE);       // just got close
  in.nowMs = in.lastPingMs = 1900;
  assert(checkFaults(c, in) == FAULT_NONE);       // 0.9 s
  in.nowMs = in.lastPingMs = 2100;
  assert(checkFaults(c, in) == F01_DISTANCE);     // 1.1 s
}

static void testDistanceTimerRestarts() {
  // old bug: the timer was never reset, so the 2nd obstacle faulted at once
  FaultChecker c;
  checkerInit(c);
  SensorInputs in = ok(1000);
  in.distanceCm = 10;
  checkFaults(c, in);
  in = ok(1500);                                  // obstacle gone
  checkFaults(c, in);
  in = ok(5000);
  in.distanceCm = 10;                             // new obstacle
  assert(checkFaults(c, in) == FAULT_NONE);
}

static void testNoEchoIsClear() {
  // old bug: ping_cm() = 0 (nothing in range) counted as 0 cm -> F01
  FaultChecker c;
  checkerInit(c);
  for (uint32_t t = 0; t < 3000; t += 100) {
    assert(checkFaults(c, ok(t)) == FAULT_NONE);
  }
}

static void testTiltTempWatchdog() {
  FaultChecker c;
  checkerInit(c);
  SensorInputs in = ok(1000);
  in.rollDeg = -31.0f;
  assert(checkFaults(c, in) == F02_TILT);
  in = ok(1000);
  in.pitchDeg = 30.0f;                            // exactly 30 is still ok
  assert(checkFaults(c, in) == FAULT_NONE);
  in = ok(1000);
  in.tempC = 40.0f;
  assert(checkFaults(c, in) == F03_TEMP);
  in = ok(1000);
  in.lastPingMs = 400;                            // 600 ms old
  assert(checkFaults(c, in) == F04_WATCHDOG);
}

static void testBadTempDoesNotHideWatchdog() {
  // old bug: a NAN temperature returned early and skipped every check
  FaultChecker c;
  checkerInit(c);
  SensorInputs in = ok(1000);
  in.tempC = NAN;
  assert(checkFaults(c, in) == FAULT_NONE);
  in.lastPingMs = 0;
  assert(checkFaults(c, in) == F04_WATCHDOG);
}

static void testWatchdogWrap() {
  // millis() wraps after ~49 days; unsigned math must still work
  FaultChecker c;
  checkerInit(c);
  SensorInputs in = ok(100);
  in.lastPingMs = 0xFFFFFF00u;                    // 356 ms before the wrap
  assert(checkFaults(c, in) == FAULT_NONE);
}

static void testWarnings() {
  SensorInputs in = ok(0);
  in.distanceCm = 20;
  assert(checkWarnings(in) == W01_OBSTACLE);
  in = ok(0);
  in.tempC = 37.0f;
  assert(checkWarnings(in) == W02_TEMP);
  assert(faultName(F04_WATCHDOG)[2] == '4' && warnName(WARN_NONE)[0] == '-');
}

int main() {
  testAllClear();
  testDistance();
  testDistanceTimerRestarts();
  testNoEchoIsClear();
  testTiltTempWatchdog();
  testBadTempDoesNotHideWatchdog();
  testWatchdogWrap();
  testWarnings();
  printf("test_faults: all passed\n");
  return 0;
}
