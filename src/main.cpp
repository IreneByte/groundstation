#include "display.h"
#include "mpu_sensor.h"
#include "network.h"
#include "fault_system.h"

#include <Arduino.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void setup() {
  // initialize serial communication and modules
  Serial.begin(115200);
  initNetwork();
  initOLED();
}

void loop() {
  // handle incoming network and client requests
  updateNetwork();

  // update finite state machine transitions and logic
  stateTransitions();
  stateLogic();

  // send telemetry updates over websocket
  handleTelemetry(webSocket, 0.0, 0.0);

  // small delay for freertos scheduler
  delay(10);
}