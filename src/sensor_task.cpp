// sensorTask: the slow sensors. Ultrasonic every 100 ms, DHT22 every 2 s.
// Runs on core 0 because both reads busy-wait for several ms, and that
// should not delay the 50 Hz control loop on core 1.
#include <Arduino.h>
#include <DHT.h>
#include <NewPing.h>

#include "pins.h"
#include "shared_state.h"
#include "tasks.h"

const int MAX_DISTANCE_CM = 200;
const uint32_t DHT_PERIOD_MS = 2000;  // DHT22 can't be read faster than this

static NewPing s_sonar(PIN_TRIG, PIN_ECHO, MAX_DISTANCE_CM);
static DHT s_dht(PIN_DHT, DHT22);

void sensorsInit() {
  s_dht.begin();
}

void sensorTask(void* arg) {
  (void)arg;
  TickType_t lastWake = xTaskGetTickCount();
  uint32_t lastDhtMs = 0;

  for (;;) {
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(SENSOR_PERIOD_MS));

    int distance = (int)s_sonar.ping_cm();  // 0 = no echo
    bool readTemp = (millis() - lastDhtMs >= DHT_PERIOD_MS);
    float temp = NAN;
    if (readTemp) {
      temp = s_dht.readTemperature();  // NAN if the read failed
      lastDhtMs = millis();
    }

    lockShared();
    g_shared.distanceCm = distance;
    if (readTemp) g_shared.tempC = temp;
    unlockShared();
  }
}
