#pragma once

#include <Arduino.h>

void initFSM();
void stateTransitions();
void stateLogic();

void logState(String state);
String getStateString();
void runIdle();
void runOnline();
void runManual(float pitch, float roll);
void runFault();
void runReset();