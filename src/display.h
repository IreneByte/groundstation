#pragma once
// SSD1306 OLED on the default I2C bus (Wire). Only telemetryTask uses it.

bool oledInit();
void oledShow(const char* state, const char* fault, const char* ip);
