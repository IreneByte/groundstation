#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <Wire.h>

// oled display setup (128x64 i2c address 0x3C)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// initialize ssd1306 oled display instance
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void initOLED() {
  // test i2c communication with ssd1306 display
  if (!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1); // infinite loop if display init fails
  }
  oled.clearDisplay(); 
  oled.setTextSize(1); 
  oled.setTextColor(SSD1306_WHITE); 
  oled.setCursor(0, 2); 
  oled.println(WiFi.localIP()); // print local ip for debugging
  oled.display(); 
}

void printOLED(const char* message) {
  // clear screen and display custom status message
  oled.clearDisplay();
  oled.println(F(message));
  oled.display();
}

void updateOLEDStatus(String state, const char* fault) {
  // clear screen for updated runtime state and faults
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);

  // print current finite state machine state
  oled.setCursor(0, 0);
  oled.print(F("State: "));
  oled.println(state);

  // print current active fault code
  oled.setCursor(0, 16);
  oled.print(F("Fault: "));
  oled.println(fault);

  oled.display();
}