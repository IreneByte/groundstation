#include "motors.h"

#include <Arduino.h>

#include "pins.h"

// Encoder counts. The interrupts add to them and controlTask reads and
// zeroes them. "Read then zero" is two steps, so an edge in between
// would get lost. A spinlock (critical section) makes it one step.
// A mutex can't be used here because interrupts can't wait on a mutex.
static volatile long s_ticksL = 0;
static volatile long s_ticksR = 0;
static portMUX_TYPE s_tickLock = portMUX_INITIALIZER_UNLOCKED;

static void IRAM_ATTR onTickL() {
  portENTER_CRITICAL_ISR(&s_tickLock);
  s_ticksL++;
  portEXIT_CRITICAL_ISR(&s_tickLock);
}

static void IRAM_ATTR onTickR() {
  portENTER_CRITICAL_ISR(&s_tickLock);
  s_ticksR++;
  portEXIT_CRITICAL_ISR(&s_tickLock);
}

void motorsInit() {
  const int outs[] = {PIN_ENA, PIN_IN1, PIN_IN2, PIN_ENB, PIN_IN3, PIN_IN4};
  for (int pin : outs) pinMode(pin, OUTPUT);
  stopMotors();

  pinMode(PIN_ENC_L, INPUT_PULLUP);
  pinMode(PIN_ENC_R, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_L), onTickL, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_R), onTickR, RISING);
}

void setMotor(int side, int pwm, bool forward) {
  int inA = (side == 0) ? PIN_IN1 : PIN_IN3;
  int inB = (side == 0) ? PIN_IN2 : PIN_IN4;
  int en = (side == 0) ? PIN_ENA : PIN_ENB;

  // direction: one input HIGH, the other LOW
  digitalWrite(inA, forward ? HIGH : LOW);
  digitalWrite(inB, forward ? LOW : HIGH);
  analogWrite(en, constrain(pwm, 0, 255));
}

void stopMotors() {
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, LOW);
  digitalWrite(PIN_IN3, LOW);
  digitalWrite(PIN_IN4, LOW);
  analogWrite(PIN_ENA, 0);
  analogWrite(PIN_ENB, 0);
}

void readAndClearTicks(long& left, long& right) {
  portENTER_CRITICAL(&s_tickLock);
  left = s_ticksL;
  right = s_ticksR;
  s_ticksL = 0;
  s_ticksR = 0;
  portEXIT_CRITICAL(&s_tickLock);
}
