// Groundstation firmware: setup() starts everything, then the work runs
// in four FreeRTOS tasks.
//
// Task       Core  Prio  Period   Stack (bytes)  Job
// control     1     5    20 ms    4096           IMU, filter, faults, FSM, PID
// sensor      0     3    100 ms   3072           ultrasonic, DHT22 every 2 s
// telemetry   1     2    100 ms   4096           build $GS line, Serial, OLED
// net         0     2    10 ms    6144           WiFi, HTTP, WebSocket
//
// Every task uses vTaskDelayUntil() so its period stays fixed.
// (On ESP32 the stack size passed to xTaskCreate is in bytes.)
#include <Arduino.h>

#include "display.h"
#include "motors.h"
#include "mpu_sensor.h"
#include "network.h"
#include "pins.h"
#include "shared_state.h"
#include "tasks.h"

TaskHandle_t g_controlHandle = NULL;
TaskHandle_t g_sensorHandle = NULL;
TaskHandle_t g_telemetryHandle = NULL;
TaskHandle_t g_netHandle = NULL;

static void startTasks() {
  xTaskCreatePinnedToCore(controlTask, "control", 4096, NULL, 5, &g_controlHandle, 1);
  xTaskCreatePinnedToCore(sensorTask, "sensor", 3072, NULL, 3, &g_sensorHandle, 0);
  xTaskCreatePinnedToCore(telemetryTask, "telemetry", 4096, NULL, 2, &g_telemetryHandle, 1);
  xTaskCreatePinnedToCore(netTask, "net", 6144, NULL, 2, &g_netHandle, 0);
}

void setup() {
  Serial.begin(115200);
  Serial.println("#BOOT groundstation");

  sharedInit();  // mutex first, tasks use it right away
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  motorsInit();
  oledInit();
  imuInit();
  sensorsInit();
  controlInit();
  netInit();
  startTasks();
}

void loop() {
  // nothing to do here, everything runs in the tasks above
  vTaskDelete(NULL);
}
