#include "display.h"
#include "mpu_sensor.h"
#include "network.h"

#include <Arduino.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void setup() {
  Serial.begin(115200);
  initNetwork();
  initOLED();
}

void loop() {
  updateNetwork();
}