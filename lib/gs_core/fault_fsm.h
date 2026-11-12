#pragma once
// The 5-state fault FSM. Plain C++ so every transition can be tested
// on the PC.
//
//   IDLE --wifi up--> ONLINE --button--> MANUAL --fault--> FAULT
//   ONLINE --wifi down--> IDLE
//   FAULT --button or ACK--> RESET_REQUIRED --button or RESET--> IDLE
//
// The button acts on the press EDGE (up -> down). Holding it down only
// counts once, so it can't skip through FAULT and RESET_REQUIRED.
#include <stdint.h>

#include "fault_checks.h"

enum FsmState { ST_IDLE = 0, ST_ONLINE, ST_MANUAL, ST_FAULT, ST_RESET_REQUIRED };

struct FsmInputs {
  bool wifiUp;        // WiFi connected
  bool buttonDown;    // physical button level (true = pressed)
  bool ackCmd;        // "ACK" from the dashboard
  bool resetCmd;      // "RESET" from the dashboard
  FaultCode fault;    // result of checkFaults()
  uint32_t nowMs;
};

const int FAULT_HISTORY_LEN = 8;

struct FaultRecord {
  FaultCode code;
  uint32_t timeMs;
};

struct FaultFsm {
  FsmState state;
  bool lastButtonDown;      // for edge detection
  FaultCode activeFault;    // fault that sent us to FAULT
  uint32_t faultCount;      // total faults since boot
  FaultRecord history[FAULT_HISTORY_LEN];  // ring buffer, newest at next-1
  int historyNext;
};

void fsmInit(FaultFsm& m);

// Run one step. Returns true if the state changed.
bool fsmStep(FaultFsm& m, const FsmInputs& in);

// Motors may only run in MANUAL.
bool fsmMotorsAllowed(const FaultFsm& m);

const char* stateName(FsmState s);

// i = 0 is the newest fault. Returns false if there is no such record.
bool fsmHistory(const FaultFsm& m, int i, FaultRecord& out);
