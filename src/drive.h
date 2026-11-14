#pragma once
// Turns a drive command into wheel speed targets and runs one PID per wheel.

struct DriveOutput {
  float speedL, speedR;  // measured, ticks per second
  int pwmL, pwmR;        // PID output 0..255
};

void driveInit();

// cmd: 'W' forward, 'S' back, 'A' left, 'D' right, 'X' stop.
// allowed: true only in MANUAL. dt: seconds since the last call.
void driveUpdate(char cmd, bool allowed, float dt, DriveOutput& out);
