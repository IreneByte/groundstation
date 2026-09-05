#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

void webSocketEvent();
void initNetwork();
void updateNetwork();
String buildTelemetryJSON(float currentPitch, float currentRoll);
void triggerAlert(const char* errorCode);