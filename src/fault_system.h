#pragma once

#include <Arduino.h>

void initFSM();
void updateFSM();       // sensors, fault checks, fsm step, motors
String getStateString();
void stopMotors();
void runPID();
