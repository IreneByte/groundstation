// Host test for lib/gs_core/fault_fsm: every state, every transition,
// and the full fault -> recovery path. Build: see run_host_tests.sh
#include <cassert>
#include <cstdio>

#include "fault_fsm.h"

// inputs with nothing happening (WiFi up, no button, no fault)
static FsmInputs quiet() {
  FsmInputs in = {true, false, false, false, FAULT_NONE, 1000};
  return in;
}

// press = one step with the button down, then one step with it up
static void press(FaultFsm& m) {
  FsmInputs in = quiet();
  in.buttonDown = true;
  fsmStep(m, in);
  fsmStep(m, quiet());
}

static void goToManual(FaultFsm& m) {
  fsmInit(m);
  fsmStep(m, quiet());  // IDLE -> ONLINE
  press(m);             // ONLINE -> MANUAL
  assert(m.state == ST_MANUAL);
}

static void testIdleOnline() {
  FaultFsm m;
  fsmInit(m);
  FsmInputs in = quiet();
  in.wifiUp = false;
  assert(!fsmStep(m, in) && m.state == ST_IDLE);   // stays without WiFi
  in.buttonDown = true;
  fsmStep(m, in);
  assert(m.state == ST_IDLE);                      // button does nothing in IDLE
  assert(fsmStep(m, quiet()) && m.state == ST_ONLINE);
  in.buttonDown = false;
  assert(fsmStep(m, in) && m.state == ST_IDLE);    // WiFi lost -> IDLE
}

static void testOnlineManual() {
  FaultFsm m;
  fsmInit(m);
  fsmStep(m, quiet());
  fsmStep(m, quiet());
  assert(m.state == ST_ONLINE);                    // waits for the button
  assert(!fsmMotorsAllowed(m));
  press(m);
  assert(m.state == ST_MANUAL && fsmMotorsAllowed(m));
  fsmStep(m, quiet());
  assert(m.state == ST_MANUAL);                    // no fault: stays
}

static void testEveryFaultCode() {
  FaultCode codes[] = {F01_DISTANCE, F02_TILT, F03_TEMP, F04_WATCHDOG};
  for (FaultCode c : codes) {
    FaultFsm m;
    goToManual(m);
    FsmInputs in = quiet();
    in.fault = c;
    assert(fsmStep(m, in) && m.state == ST_FAULT);
    assert(m.activeFault == c && !fsmMotorsAllowed(m));
  }
}

static void testFaultAndRecovery() {
  FaultFsm m;
  goToManual(m);
  FsmInputs in = quiet();
  in.fault = F03_TEMP;
  fsmStep(m, in);
  assert(m.state == ST_FAULT);
  fsmStep(m, quiet());
  assert(m.state == ST_FAULT);                     // stays until button/ACK
  press(m);
  assert(m.state == ST_RESET_REQUIRED && m.activeFault == F03_TEMP);
  fsmStep(m, quiet());
  assert(m.state == ST_RESET_REQUIRED);            // stays until button/RESET
  in = quiet();
  in.buttonDown = true;
  fsmStep(m, in);
  assert(m.state == ST_IDLE && m.activeFault == FAULT_NONE);
  fsmStep(m, quiet());
  assert(m.state == ST_ONLINE);                    // back to normal
  press(m);
  assert(m.state == ST_MANUAL);                    // can drive again
}

static void testDashboardAckReset() {
  FaultFsm m;
  goToManual(m);
  FsmInputs in = quiet();
  in.fault = F04_WATCHDOG;
  fsmStep(m, in);
  in = quiet();
  in.resetCmd = true;
  fsmStep(m, in);
  assert(m.state == ST_FAULT);                     // RESET alone can't skip ACK
  in = quiet();
  in.ackCmd = true;
  fsmStep(m, in);
  assert(m.state == ST_RESET_REQUIRED);
  in = quiet();
  in.resetCmd = true;
  fsmStep(m, in);
  assert(m.state == ST_IDLE);
}

static void testHeldButtonOnlyCountsOnce() {
  // old bug: holding the button went FAULT -> RESET_REQUIRED -> IDLE
  FaultFsm m;
  goToManual(m);
  FsmInputs in = quiet();
  in.fault = F01_DISTANCE;
  fsmStep(m, in);
  in = quiet();
  in.buttonDown = true;
  for (int i = 0; i < 20; i++) fsmStep(m, in);    // held for 20 steps
  assert(m.state == ST_RESET_REQUIRED);
}

static void testHistory() {
  FaultFsm m;
  fsmInit(m);
  FaultRecord r;
  assert(!fsmHistory(m, 0, r));
  for (int i = 0; i < 10; i++) {                   // more than the buffer
    m.state = ST_MANUAL;                           // shortcut back to MANUAL
    FsmInputs in = quiet();
    in.fault = (i % 2) ? F02_TILT : F01_DISTANCE;
    in.nowMs = 100u * i;
    fsmStep(m, in);
  }
  assert(m.faultCount == 10);
  assert(fsmHistory(m, 0, r) && r.code == F02_TILT && r.timeMs == 900);
  assert(fsmHistory(m, 7, r) && r.timeMs == 200);  // oldest kept
  assert(!fsmHistory(m, 8, r));
}

int main() {
  testIdleOnline();
  testOnlineManual();
  testEveryFaultCode();
  testFaultAndRecovery();
  testDashboardAckReset();
  testHeldButtonOnlyCountsOnce();
  testHistory();
  printf("test_fsm: all passed\n");
  return 0;
}
