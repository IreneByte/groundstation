#pragma once
// Fault and warning checks. Plain C++ so they can be tested on the PC.
#include <stdint.h>

enum FaultCode {
  FAULT_NONE = 0,
  F01_DISTANCE,   // obstacle <= 15 cm for more than 1 s
  F02_TILT,       // |pitch| or |roll| > 30 deg
  F03_TEMP,       // temperature >= 40 C
  F04_WATCHDOG    // no "ping" from the browser for > 500 ms
};

enum WarnCode {
  WARN_NONE = 0,
  W01_OBSTACLE,   // obstacle 16-29 cm
  W02_TEMP        // temperature 35-40 C
};

// limits (same numbers as the first version of the project)
const int DIST_FAULT_CM = 15;
const int DIST_WARN_CM = 30;
const uint32_t DIST_HOLD_MS = 1000;
const float TILT_FAULT_DEG = 30.0f;
const float TEMP_FAULT_C = 40.0f;
const float TEMP_WARN_C = 35.0f;
const uint32_t WATCHDOG_MS = 500;

struct SensorInputs {
  uint32_t nowMs;       // current time
  int distanceCm;       // 0 means "no echo" (nothing in range)
  float tempC;          // NAN if the DHT read failed
  float pitchDeg;       // filter output
  float rollDeg;        // filter output
  uint32_t lastPingMs;  // time of the last browser ping
};

// F01 needs memory: how long has the obstacle been close?
struct FaultChecker {
  bool close;           // obstacle currently <= 15 cm
  uint32_t closeSinceMs;
};

void checkerInit(FaultChecker& c);

// Returns the first active fault in the order F01, F02, F03, F04.
FaultCode checkFaults(FaultChecker& c, const SensorInputs& in);
WarnCode checkWarnings(const SensorInputs& in);

const char* faultName(FaultCode f);  // "F01" ... or "-"
const char* warnName(WarnCode w);    // "W01" ... or "-"
