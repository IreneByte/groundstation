#include "network.h"
#include "fault_system.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// wifi credentials for testing in simulator
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

// web server on port 80 and websocket server on port 81
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
unsigned long lastPingTime;

// dashboard html user interface with async fetch and websocket control
String web = R"HTML(
  <html>
    <body>
      <h1>Groundstation</h1>

      <button onclick="sendCmd('W')">Forward (W)</button><br><br>
      <button onclick="sendCmd('A')">Left (A)</button>
      <button onclick="sendCmd('STOP')">STOP</button>
      <button onclick="sendCmd('D')">Right (D)</button><br><br>
      <button onclick="sendCmd('S')">Back (S)</button>

      <h2>Fault Status: <span id="faultCode">None</span></h2>
      <button onclick="sendCmd('ACK')">ACK Fault</button>
      <button onclick="sendCmd('RESET')">RESET System</button><br><br>

      <script>
        // websocket connection for real-time telemetry and fault messages
        var connection = new WebSocket('ws://' + location.hostname + ':8181/');

        function sendCmd(cmd) {
          if (connection.readyState === WebSocket.OPEN) {
            connection.send(cmd);
          }
        }

        // wasd keyboard shortcuts for manual drive testing
        document.addEventListener('keydown', function(event) {
          if(event.key === 'w' || event.key === 'W') sendCmd('W');
          if(event.key === 'a' || event.key === 'A') sendCmd('A');
          if(event.key === 's' || event.key === 'S') sendCmd('S');
          if(event.key === 'd' || event.key === 'D') sendCmd('D');
          if(event.key === ' ') sendCmd('STOP'); 
        });

        // periodic ping to keep watchdog alive
        setInterval(function() {
          sendCmd('ping');
        }, 200);

        // handle incoming websocket text messages
        connection.onmessage = function(event) {
          if (event.data.startsWith('F') || event.data.startsWith('W')) {
            document.getElementById("faultCode").innerText = event.data;
          } else if (event.data === "CLEAR") {
            document.getElementById("faultCode").innerText = "None";
          }
        };
      </script>
    </body>
  </html>
)HTML";

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  // handle websocket connection events and client payloads
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("WS: DISCONNECTED");
      break;
    case WStype_CONNECTED:
      Serial.println("WS: CONNECTED");
      break;
    case WStype_TEXT: {
      String msg = String((char*)payload, length);
      Serial.println(msg);

      // evaluate incoming dashboard control commands
      if (msg == "W") Serial.println("Would drive forward");
      else if (msg == "S") Serial.println("Would drive backward");
      else if (msg == "A") Serial.println("Would turn left");
      else if (msg == "D") Serial.println("Would turn right");
      else if (msg == "STOP") Serial.println("Would stop");
      else if (msg == "ACK") Serial.println("Dashboard ACK received");
      else if (msg == "RESET") {
        Serial.println("Dashboard RESET received");
        faultActive = false;
      }
      else if (msg == "ping") lastPingTime = millis();
      break;
    }
    default:
      break;
  }
}

void initNetwork() {
  // configure station mode and begin wifi connection
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);

  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }

  Serial.println("Connected");
  Serial.println(WiFi.localIP());

  // setup http server root route
  server.on("/", []() {
    server.send(200, "text/html", web);
  });

  server.begin();  
  webSocket.begin(); 
  webSocket.onEvent(webSocketEvent); 

  Serial.println("HTTP & WebSocket Servers Started!");
}

void updateNetwork() {
  // process incoming client and websocket requests
  server.handleClient();
  webSocket.loop();
  delay(2);
}

void sendTelemetry(float currentPitch, float currentRoll) {
    // build telemetry json string payload
    String json = R"json({
        "state": ")" + getStateString() + R"json(",
        "speed_left": "N/A",
        "speed_right": "N/A",
        "pitch": )" + String(currentPitch, 2) + R"json(,
        "roll": )" + String(currentRoll, 2) + R"json(,
        "heading": "N/A",
        "obstacle_cm": "N/A",
        "temperature": "N/A",
        "humidity": "N/A",
        "fault": null,
        "uptime_ms": )" + String(millis()) + R"json(
    })json";

    webSocket.broadcastTXT(json);
}

void handleTelemetry(WebSocketsServer &webSocket, float currentPitch, float currentRoll) {
  // broadcast telemetry over websocket every 100ms
  static unsigned long lastBroadcast = 0;

  if (millis() - lastBroadcast >= 100) {
    lastBroadcast = millis();
    sendTelemetry(currentPitch, currentRoll);
  }
}

void triggerAlert(const char* errorCode) {
    // print error code and broadcast via websocket
    Serial.println(errorCode); 
    webSocket.broadcastTXT(errorCode);
}