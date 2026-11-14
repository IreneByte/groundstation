#include "drive.h"

#include "motors.h"
#include "pid.h"

// Gains and target speed are first guesses checked only against the
// simple motor model in host_tests/test_pid.cpp, not tuned on a car.
const float KP = 1.5f;
const float KI = 4.0f;
const float KD = 0.02f;
const float TARGET_SPEED = 60.0f;  // ticks per second

static Pid s_pidL;
static Pid s_pidR;

void driveInit() {
  pidInit(s_pidL, KP, KI, KD, 0.0f, 255.0f);
  pidInit(s_pidR, KP, KI, KD, 0.0f, 255.0f);
}

// target speed for each wheel. Turning = one wheel at half speed.
static void targetsFor(char cmd, float& left, float& right, bool& forward) {
  left = right = 0.0f;
  forward = (cmd != 'S');
  if (cmd == 'W' || cmd == 'S') left = right = TARGET_SPEED;
  if (cmd == 'A') { left = TARGET_SPEED / 2; right = TARGET_SPEED; }
  if (cmd == 'D') { left = TARGET_SPEED; right = TARGET_SPEED / 2; }
}

void driveUpdate(char cmd, bool allowed, float dt, DriveOutput& out) {
  long ticksL, ticksR;
  readAndClearTicks(ticksL, ticksR);
  out.speedL = (dt > 0.0f) ? ticksL / dt : 0.0f;
  out.speedR = (dt > 0.0f) ? ticksR / dt : 0.0f;

  float targetL, targetR;
  bool forward;
  targetsFor(cmd, targetL, targetR, forward);

  // not allowed to move, or told to stop: motors off, forget PID history
  if (!allowed || (targetL == 0.0f && targetR == 0.0f)) {
    stopMotors();
    pidReset(s_pidL);
    pidReset(s_pidR);
    out.pwmL = out.pwmR = 0;
    return;
  }

  out.pwmL = (int)pidUpdate(s_pidL, targetL, out.speedL, dt);
  out.pwmR = (int)pidUpdate(s_pidR, targetR, out.speedR, dt);
  setMotor(0, out.pwmL, forward);
  setMotor(1, out.pwmR, forward);
}
