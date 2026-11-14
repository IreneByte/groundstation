#pragma once
// FreeRTOS task functions and their timing. The README has the same
// numbers in a table.
#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

const uint32_t CONTROL_PERIOD_MS = 20;     // 50 Hz: IMU, filter, FSM, PID
const uint32_t SENSOR_PERIOD_MS = 100;     // 10 Hz: ultrasonic (+ DHT every 2 s)
const uint32_t TELEMETRY_PERIOD_MS = 100;  // 10 Hz: build + send one message
const uint32_t NET_PERIOD_MS = 10;         // 100 Hz: HTTP + WebSocket service

// handles so telemetryTask can print how much stack each task has left
extern TaskHandle_t g_controlHandle;
extern TaskHandle_t g_sensorHandle;
extern TaskHandle_t g_telemetryHandle;
extern TaskHandle_t g_netHandle;

void controlInit();
void controlTask(void* arg);

void sensorsInit();
void sensorTask(void* arg);

void telemetryTask(void* arg);
