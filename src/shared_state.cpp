#include "shared_state.h"

#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

SharedState g_shared;
static SemaphoreHandle_t s_mutex = NULL;

void sharedInit() {
  s_mutex = xSemaphoreCreateMutex();
  memset(&g_shared, 0, sizeof(g_shared));
  g_shared.driveCmd = 'X';
  g_shared.tempC = NAN;
  g_shared.state = ST_IDLE;
  strcpy(g_shared.ip, "0.0.0.0");
}

void lockShared() {
  xSemaphoreTake(s_mutex, portMAX_DELAY);
}

void unlockShared() {
  xSemaphoreGive(s_mutex);
}
