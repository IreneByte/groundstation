#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

extern WebSocketsServer webSocket;
extern bool ackRequested;
extern bool resetRequested;
extern unsigned long lastPingTime;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void initNetwork();
void updateNetwork();
String buildTelemetryJSON(float currentPitch, float currentRoll);
void handleTelemetry(WebSocketsServer &webSocket, float currentPitch, float currentRoll);
void triggerAlert(const char* errorCode);