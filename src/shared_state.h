#pragma once
// Data shared between FreeRTOS tasks.
//
// RULE: every read or write of g_shared happens between lockShared()
// and unlockShared(). Keep the locked part short (just copy values).
//
// Who writes what:
//   netTask       -> lastPingMs, ackCmd, resetCmd, driveCmd, wifiUp, ip
//   sensorTask    -> distanceCm, tempC
//   controlTask   -> everything in the "control outputs" block,
//                    and clears ackCmd / resetCmd after using them
//   telemetryTask -> only reads (takes a copy)
#include <Arduino.h>

#include "fault_fsm.h"

struct SharedState {
  // from the browser (netTask)
  uint32_t lastPingMs;
  bool ackCmd;        // one-shot, controlTask clears it
  bool resetCmd;      // one-shot, controlTask clears it
  char driveCmd;      // 'W', 'A', 'S', 'D' or 'X' (stop)
  bool wifiUp;
  char ip[16];

  // from the slow sensors (sensorTask)
  int distanceCm;     // 0 = no echo
  float tempC;        // NAN until the first good read

  // control outputs (controlTask)
  float pitchDeg, rollDeg;
  float speedL, speedR;
  int pwmL, pwmR;
  FsmState state;
  FaultCode fault;
  WarnCode warn;
  uint32_t faultCount;
  uint32_t loopAvgUs, loopMaxUs, periodMaxUs;
};

extern SharedState g_shared;

void sharedInit();    // create the mutex and fill in start values
void lockShared();    // waits until the mutex is free
void unlockShared();
