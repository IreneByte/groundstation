#pragma once
// Complementary filter for pitch and roll from the MPU6050.
// Plain C++ so it can be tested on the PC.
//
// Idea: the gyro is smooth but drifts, the accelerometer doesn't drift
// but is noisy (motor vibration). Mix them:
//   angle = alpha * (angle + gyroRate * dt) + (1 - alpha) * accelAngle
// alpha = 0.98 means "trust the gyro short term, the accel long term".

struct ImuSample {
  float ax, ay, az;   // acceleration in m/s^2
  float gx, gy, gz;   // rotation rate in rad/s
};

struct TiltFilter {
  float alpha;        // 0..1, how much to trust the gyro
  float pitchDeg;     // filter output
  float rollDeg;      // filter output
  bool started;       // false until the first sample
};

void filterInit(TiltFilter& f, float alpha);

// Tilt angles from the accelerometer alone (degrees).
// Level board (0, 0, 9.81) gives pitch 0 and roll 0.
float accelPitchDeg(const ImuSample& s);
float accelRollDeg(const ImuSample& s);

// One filter step. dt in seconds. A bad dt (<= 0 or > 0.5 s) resets the
// angles to the accel angles instead of integrating a huge gyro step.
void filterUpdate(TiltFilter& f, const ImuSample& s, float dt);
