#pragma once

void initOLED();
void printOLED(const char* message);

void updateOLEDStatus(String state, const char* fault);