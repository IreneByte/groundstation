#include "fault_checks.h"

#include <cmath>

void checkerInit(FaultChecker& c) {
  c.close = false;
  c.closeSinceMs = 0;
}

// NewPing returns 0 when there is no echo, so 0 means "clear", not 0 cm
static bool obstacleClose(int distanceCm) {
  return distanceCm > 0 && distanceCm <= DIST_FAULT_CM;
}

// F01: obstacle has to stay close for DIST_HOLD_MS. The timer restarts
// every time the obstacle goes away.
static bool distanceFault(FaultChecker& c, const SensorInputs& in) {
  if (!obstacleClose(in.distanceCm)) {
    c.close = false;
    return false;
  }
  if (!c.close) {
    c.close = true;
    c.closeSinceMs = in.nowMs;
  }
  return (in.nowMs - c.closeSinceMs) > DIST_HOLD_MS;
}

FaultCode checkFaults(FaultChecker& c, const SensorInputs& in) {
  bool f01 = distanceFault(c, in);  // always run so the timer stays right
  if (f01) return F01_DISTANCE;
  if (fabsf(in.pitchDeg) > TILT_FAULT_DEG || fabsf(in.rollDeg) > TILT_FAULT_DEG) {
    return F02_TILT;
  }
  // a failed DHT read (NAN) skips only the temperature check
  if (!std::isnan(in.tempC) && in.tempC >= TEMP_FAULT_C) return F03_TEMP;
  // unsigned subtraction still works when millis() wraps around
  if (in.nowMs - in.lastPingMs > WATCHDOG_MS) return F04_WATCHDOG;
  return FAULT_NONE;
}

WarnCode checkWarnings(const SensorInputs& in) {
  if (in.distanceCm > DIST_FAULT_CM && in.distanceCm < DIST_WARN_CM) return W01_OBSTACLE;
  if (!std::isnan(in.tempC) && in.tempC > TEMP_WARN_C && in.tempC < TEMP_FAULT_C) {
    return W02_TEMP;
  }
  return WARN_NONE;
}

const char* faultName(FaultCode f) {
  switch (f) {
    case F01_DISTANCE: return "F01";
    case F02_TILT: return "F02";
    case F03_TEMP: return "F03";
    case F04_WATCHDOG: return "F04";
    default: return "-";
  }
}

const char* warnName(WarnCode w) {
  switch (w) {
    case W01_OBSTACLE: return "W01";
    case W02_TEMP: return "W02";
    default: return "-";
  }
}
