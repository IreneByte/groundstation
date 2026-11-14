#pragma once
// L298N motor pins and the wheel encoder interrupts.

void motorsInit();

// side: 0 = left, 1 = right. pwm 0..255.
void setMotor(int side, int pwm, bool forward);
void stopMotors();

// Copy the encoder counts since the last call and reset them to 0.
// Safe to call while the encoder interrupts are running.
void readAndClearTicks(long& left, long& right);
