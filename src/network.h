#pragma once
// WiFi, HTTP page (port 80) and WebSocket (port 81).
// Only netTask touches the WebSocket library, because it is not
// thread safe. Other tasks hand it lines through g_txQueue.
#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// one telemetry line waiting to be sent
struct TxLine {
  char text[160];
};

const int TX_QUEUE_LEN = 8;  // ~0.8 s of telemetry
extern QueueHandle_t g_txQueue;

void netInit();           // start WiFi (does not wait for it)
void netTask(void* arg);  // serves HTTP + WebSocket every 10 ms
