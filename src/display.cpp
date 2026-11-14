#include "display.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Wire.h>

// 128x64 OLED at I2C address 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

static Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
static bool s_oledOk = false;

bool oledInit() {
  s_oledOk = oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  if (!s_oledOk) {
    // keep running without a screen instead of hanging forever
    Serial.println("#OLED SSD1306 not found");
    return false;
  }
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.display();
  return true;
}

void oledShow(const char* state, const char* fault, const char* ip) {
  if (!s_oledOk) return;
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.print("State: ");
  oled.println(state);
  oled.setCursor(0, 16);
  oled.print("Fault: ");
  oled.println(fault);
  oled.setCursor(0, 32);
  oled.println(ip);
  oled.display();
}
