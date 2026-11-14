// telemetryTask: 10 Hz. Copies the shared state, builds one $GS line
// (format in lib/gs_core/telemetry_format.h), prints it on Serial and
// queues it for netTask to send on the WebSocket.
#include <Arduino.h>

#include "display.h"
#include "network.h"
#include "shared_state.h"
#include "tasks.h"
#include "telemetry_format.h"

static void fillFrame(const SharedState& s, uint32_t seq, TelemetryFrame& f) {
  f.seq = seq;
  f.timeMs = millis();
  f.pitchDeg = s.pitchDeg;
  f.rollDeg = s.rollDeg;
  f.speedL = s.speedL;
  f.speedR = s.speedR;
  f.pwmL = s.pwmL;
  f.pwmR = s.pwmR;
  f.distanceCm = s.distanceCm;
  f.tempC = s.tempC;
  f.state = stateName(s.state);
  f.fault = faultName(s.fault);
  f.warn = warnName(s.warn);
  f.faultCount = s.faultCount;
  f.loopAvgUs = s.loopAvgUs;
  f.loopMaxUs = s.loopMaxUs;
  f.periodMaxUs = s.periodMaxUs;
}

// free stack (bytes) of each task, so the sizes in main.cpp can be checked
static void printStackLeft() {
  Serial.printf("#STACK control=%u sensor=%u telemetry=%u net=%u\n",
                (unsigned)uxTaskGetStackHighWaterMark(g_controlHandle),
                (unsigned)uxTaskGetStackHighWaterMark(g_sensorHandle),
                (unsigned)uxTaskGetStackHighWaterMark(g_telemetryHandle),
                (unsigned)uxTaskGetStackHighWaterMark(g_netHandle));
}

void telemetryTask(void* arg) {
  (void)arg;
  TickType_t lastWake = xTaskGetTickCount();
  uint32_t seq = 0;
  uint32_t queueFull = 0;  // lines the WebSocket side could not take

  for (;;) {
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(TELEMETRY_PERIOD_MS));

    SharedState copy;
    lockShared();
    copy = g_shared;  // short lock: just a struct copy
    unlockShared();

    TelemetryFrame f;
    fillFrame(copy, seq++, f);
    TxLine line;
    if (formatTelemetry(line.text, sizeof(line.text), f) < 0) continue;

    Serial.printf("%s\n", line.text);
    if (xQueueSend(g_txQueue, &line, 0) != pdTRUE) {
      queueFull++;
      Serial.printf("#TXQ full, %lu lines not sent on WebSocket\n", (unsigned long)queueFull);
    }

    if (seq % 10 == 0) oledShow(f.state, f.fault, copy.ip);  // 1 Hz
    if (seq % 50 == 0) printStackLeft();                      // every 5 s
  }
}
