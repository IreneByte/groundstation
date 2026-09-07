#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void initOLED() {
  if (!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }
  oled.clearDisplay(); 
  oled.setTextSize(1); 
  oled.setTextColor(SSD1306_WHITE); 
  oled.setCursor(0, 2); 
  oled.println(WiFi.localIP()); 
  oled.display(); 
}

void printOLED(const char* message) {
  oled.clearDisplay();
  oled.println(F(message));
}

void updateOLEDStatus(String state, const char* fault) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);

  oled.setCursor(0, 0);
  oled.print(F("State: "));
  oled.println(state);

  oled.setCursor(0, 16);
  oled.print(F("Fault: "));
  oled.println(fault);

  oled.display();
}