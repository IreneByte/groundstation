// controlTask: 50 Hz loop. IMU -> filter -> fault checks -> FSM -> PID.
#include <Arduino.h>

#include "drive.h"
#include "fault_checks.h"
#include "fault_fsm.h"
#include "loop_stats.h"
#include "mpu_sensor.h"
#include "pins.h"
#include "shared_state.h"
#include "tasks.h"

static TiltFilter s_filter;
static FaultChecker s_checker;
static FaultFsm s_fsm;

// copy of what other tasks wrote, taken once per loop
struct ControlInputs {
  uint32_t lastPingMs;
  bool ack, reset, wifiUp;
  char driveCmd;
  int distanceCm;
  float tempC;
};

void controlInit() {
  filterInit(s_filter, 0.98f);
  checkerInit(s_checker);
  fsmInit(s_fsm);
  driveInit();
}

static void readShared(ControlInputs& in) {
  lockShared();
  in.lastPingMs = g_shared.lastPingMs;
  in.ack = g_shared.ackCmd;
  in.reset = g_shared.resetCmd;
  in.wifiUp = g_shared.wifiUp;
  in.driveCmd = g_shared.driveCmd;
  in.distanceCm = g_shared.distanceCm;
  in.tempC = g_shared.tempC;
  g_shared.ackCmd = false;    // one-shot commands: used now, so clear
  g_shared.resetCmd = false;
  unlockShared();
}

// log every state change and the fault history on serial
static void logTransition(FsmState from, uint32_t now) {
  Serial.printf("#FSM t=%lu %s -> %s fault=%s faults=%lu\n", (unsigned long)now,
                stateName(from), stateName(s_fsm.state), faultName(s_fsm.activeFault),
                (unsigned long)s_fsm.faultCount);
  FaultRecord r;
  for (int i = 0; s_fsm.state == ST_FAULT && fsmHistory(s_fsm, i, r); i++) {
    Serial.printf("#HIST %d %s t=%lu\n", i, faultName(r.code), (unsigned long)r.timeMs);
  }
}

static void writeShared(const DriveOutput& d, WarnCode warn) {
  lockShared();
  g_shared.pitchDeg = s_filter.pitchDeg;
  g_shared.rollDeg = s_filter.rollDeg;
  g_shared.speedL = d.speedL;
  g_shared.speedR = d.speedR;
  g_shared.pwmL = d.pwmL;
  g_shared.pwmR = d.pwmR;
  g_shared.state = s_fsm.state;
  g_shared.fault = s_fsm.activeFault;
  g_shared.warn = warn;
  g_shared.faultCount = s_fsm.faultCount;
  unlockShared();
}

// one pass of the control loop. dt in seconds.
static void controlStep(float dt) {
  uint32_t now = millis();
  ControlInputs in;
  readShared(in);

  ImuSample imu;
  if (imuRead(imu)) filterUpdate(s_filter, imu, dt);

  SensorInputs s = {now, in.distanceCm, in.tempC, s_filter.pitchDeg,
                    s_filter.rollDeg, in.lastPingMs};
  FaultCode fault = checkFaults(s_checker, s);

  bool buttonDown = (digitalRead(PIN_BUTTON) == LOW);
  FsmInputs f = {in.wifiUp, buttonDown, in.ack, in.reset, fault, now};
  FsmState before = s_fsm.state;
  if (fsmStep(s_fsm, f)) logTransition(before, now);

  DriveOutput d;
  driveUpdate(in.driveCmd, fsmMotorsAllowed(s_fsm), dt, d);
  writeShared(d, checkWarnings(s));
}

static void publishStats(const LoopStats& run, const LoopStats& period) {
  lockShared();
  g_shared.loopAvgUs = statsAvg(run);
  g_shared.loopMaxUs = run.maxUs;
  g_shared.periodMaxUs = period.maxUs;
  unlockShared();
}

void controlTask(void* arg) {
  (void)arg;
  LoopStats run, period;
  statsReset(run);
  statsReset(period);
  TickType_t lastWake = xTaskGetTickCount();
  uint32_t lastStartUs = micros();

  for (;;) {
    // fixed 20 ms period, no drift (unlike delay())
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(CONTROL_PERIOD_MS));
    uint32_t startUs = micros();
    uint32_t sinceLastUs = startUs - lastStartUs;
    lastStartUs = startUs;

    controlStep(sinceLastUs / 1e6f);
    statsAdd(period, sinceLastUs);
    statsAdd(run, micros() - startUs);

    if (run.count >= 5) {  // 5 loops = 100 ms window, same as telemetry
      publishStats(run, period);
      statsReset(run);
      statsReset(period);
    }
  }
}
