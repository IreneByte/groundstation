#pragma once
// PID controller for ONE wheel. Plain C++ (no Arduino includes) so it
// can be tested on the PC with g++.
//
// Each wheel gets its own Pid struct. The old code kept the integral in
// "static" variables inside one function, so both wheels shared it.

struct Pid {
  float kp;               // proportional gain
  float ki;               // integral gain (per second)
  float kd;               // derivative gain (seconds)
  float outMin, outMax;   // output limit, e.g. 0..255 PWM
  float iMin, iMax;       // clamp for the integral term (anti-windup)
  float integral;         // running sum of ki * error * dt
  float lastMeasured;     // last speed, used for the derivative
  bool first;             // true until the first update
};

// Set gains and limits. The integral clamp starts equal to the output limit.
void pidInit(Pid& p, float kp, float ki, float kd, float outMin, float outMax);

// Forget the integral and derivative history (used when motors stop).
void pidReset(Pid& p);

// One PID step. setpoint and measured are in encoder ticks per second,
// dt is in seconds. Returns the clamped output (PWM).
float pidUpdate(Pid& p, float setpoint, float measured, float dt);
