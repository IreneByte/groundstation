#include "network.h"

#include <WebServer.h>
#include <WebSocketsServer.h>
#include <WiFi.h>

#include "shared_state.h"
#include "tasks.h"
#include "web_page.h"

// WiFi name/password live in include/secrets.h (not committed).
#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "Copy include/secrets.example.h to include/secrets.h and fill it in"
#endif

static WebServer server(80);
static WebSocketsServer webSocket(81);
QueueHandle_t g_txQueue = NULL;

// handle one text command from the browser
static void handleCommand(const char* msg) {
  lockShared();
  if (strcmp(msg, "ping") == 0) g_shared.lastPingMs = millis();
  else if (strcmp(msg, "ACK") == 0) g_shared.ackCmd = true;
  else if (strcmp(msg, "RESET") == 0) g_shared.resetCmd = true;
  else if (strcmp(msg, "STOP") == 0) g_shared.driveCmd = 'X';
  else if (strlen(msg) == 1 && strchr("WASD", msg[0])) g_shared.driveCmd = msg[0];
  unlockShared();
  if (strcmp(msg, "ping") != 0) Serial.printf("#CMD %s\n", msg);
}

static void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  (void)num;
  if (type == WStype_CONNECTED) Serial.println("#WS connected");
  if (type == WStype_DISCONNECTED) Serial.println("#WS disconnected");
  if (type != WStype_TEXT || length >= 16) return;  // commands are short

  char msg[16];
  memcpy(msg, payload, length);
  msg[length] = '\0';
  handleCommand(msg);
}

void netInit() {
  g_txQueue = xQueueCreate(TX_QUEUE_LEN, sizeof(TxLine));
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  Serial.println("#NET connecting to WiFi");
}

// called once, the first time WiFi is up
static void startServers() {
  server.on("/", []() { server.send(200, "text/html", WEB_PAGE); });
  server.begin();
  webSocket.begin();
  webSocket.onEvent(onWsEvent);
  Serial.printf("#NET up, open http://%s/\n", WiFi.localIP().toString().c_str());
}

static void updateWifiState(bool& started, bool& wasUp) {
  bool up = (WiFi.status() == WL_CONNECTED);
  if (up && !started) {
    startServers();
    started = true;
  }
  char ip[16] = "";
  if (up && !wasUp) {  // just connected: read the IP once, outside the lock
    strncpy(ip, WiFi.localIP().toString().c_str(), sizeof(ip) - 1);
  }
  lockShared();
  g_shared.wifiUp = up;
  if (ip[0] != '\0') strcpy(g_shared.ip, ip);
  unlockShared();
  wasUp = up;
}

void netTask(void* arg) {
  (void)arg;
  TickType_t lastWake = xTaskGetTickCount();
  bool started = false;
  bool wasUp = false;
  TxLine line;

  for (;;) {
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(NET_PERIOD_MS));
    updateWifiState(started, wasUp);
    if (!started) {
      xQueueReset(g_txQueue);  // nobody to send to yet, drop old lines
      continue;
    }

    server.handleClient();
    webSocket.loop();  // runs onWsEvent() for incoming messages
    // send every telemetry line that is waiting
    while (xQueueReceive(g_txQueue, &line, 0) == pdTRUE) {
      webSocket.broadcastTXT(line.text);
    }
  }
}
