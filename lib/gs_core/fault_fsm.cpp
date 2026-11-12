#include "fault_fsm.h"

void fsmInit(FaultFsm& m) {
  m.state = ST_IDLE;
  m.lastButtonDown = false;
  m.activeFault = FAULT_NONE;
  m.faultCount = 0;
  m.historyNext = 0;
  for (int i = 0; i < FAULT_HISTORY_LEN; i++) {
    m.history[i].code = FAULT_NONE;
    m.history[i].timeMs = 0;
  }
}

// save a fault in the ring buffer (oldest one gets overwritten)
static void recordFault(FaultFsm& m, FaultCode code, uint32_t nowMs) {
  m.activeFault = code;
  m.faultCount++;
  m.history[m.historyNext].code = code;
  m.history[m.historyNext].timeMs = nowMs;
  m.historyNext = (m.historyNext + 1) % FAULT_HISTORY_LEN;
}

// pick the next state. "pressed" is true only on the press edge.
static FsmState nextState(FaultFsm& m, const FsmInputs& in, bool pressed) {
  switch (m.state) {
    case ST_IDLE:
      return in.wifiUp ? ST_ONLINE : ST_IDLE;
    case ST_ONLINE:
      if (!in.wifiUp) return ST_IDLE;
      return pressed ? ST_MANUAL : ST_ONLINE;
    case ST_MANUAL:
      if (in.fault == FAULT_NONE) return ST_MANUAL;
      recordFault(m, in.fault, in.nowMs);
      return ST_FAULT;
    case ST_FAULT:
      return (pressed || in.ackCmd) ? ST_RESET_REQUIRED : ST_FAULT;
    case ST_RESET_REQUIRED:
      if (!(pressed || in.resetCmd)) return ST_RESET_REQUIRED;
      m.activeFault = FAULT_NONE;  // recovery: fault is cleared
      return ST_IDLE;
  }
  return ST_IDLE;  // unknown value: go to the safe state
}

bool fsmStep(FaultFsm& m, const FsmInputs& in) {
  bool pressed = in.buttonDown && !m.lastButtonDown;
  m.lastButtonDown = in.buttonDown;

  FsmState next = nextState(m, in, pressed);
  bool changed = (next != m.state);
  m.state = next;
  return changed;
}

bool fsmMotorsAllowed(const FaultFsm& m) {
  return m.state == ST_MANUAL;
}

const char* stateName(FsmState s) {
  switch (s) {
    case ST_IDLE: return "IDLE";
    case ST_ONLINE: return "ONLINE";
    case ST_MANUAL: return "MANUAL";
    case ST_FAULT: return "FAULT";
    case ST_RESET_REQUIRED: return "RESET_REQUIRED";
  }
  return "UNKNOWN";
}

bool fsmHistory(const FaultFsm& m, int i, FaultRecord& out) {
  int stored = (m.faultCount < (uint32_t)FAULT_HISTORY_LEN) ? (int)m.faultCount : FAULT_HISTORY_LEN;
  if (i < 0 || i >= stored) return false;
  int idx = (m.historyNext - 1 - i + FAULT_HISTORY_LEN) % FAULT_HISTORY_LEN;
  out = m.history[idx];
  return true;
}
