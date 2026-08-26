#include <Arduino.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

String web = R"HTML(
  <html>
    <body>
      <h1>Groundstation</h1>

      <button onclick="sendCmd('W')">Forward (W)</button><br><br>
      <button onclick="sendCmd('A')">Left (A)</button>
      <button onclick="sendCmd('STOP')">STOP</button>
      <button onclick="sendCmd('D')">Right (D)</button><br><br>
      <button onclick="sendCmd('S')">Back (S)</button>

      <script>
        var connection = new WebSocket('ws://' + location.hostname + ':8181/');

        function sendCmd(cmd) {
          if (connection.readyState === WebSocket.OPEN) {
            connection.send(cmd);
          }
        }

        document.addEventListener('keydown', function(event) {
          if(event.key === 'w' || event.key === 'W') sendCmd('W');
          if(event.key === 'a' || event.key === 'A') sendCmd('A');
          if(event.key === 's' || event.key === 'S') sendCmd('S');
          if(event.key === 'd' || event.key === 'D') sendCmd('D');
          if(event.key === ' ') sendCmd('STOP'); 
        });

        setInterval(function() {
          sendCmd('ping');
        }, 200);
      </script>
    </body>
  </html>
)HTML";

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);

  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }

  Serial.println("Connected");
  Serial.println(WiFi.localIP());
}

void initOLED() {
  if (!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }
  oled.clearDisplay(); 
  oled.setTextSize(1); 
  oled.setTextColor(SSD1306_WHITE); 
  oled.setCursor(0, 2); 
  oled.println(WiFi.localIP()); 
  oled.display(); 
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
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

      if (msg == "W") Serial.println("Would drive forward");
      else if (msg == "S") Serial.println("Would drive backward");
      else if (msg == "A") Serial.println("Would turn left");
      else if (msg == "D") Serial.println("Would turn right");
      else if (msg == "STOP") Serial.println("Would stop");
      else if (msg == "ping") {}
      break;
    }
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  initWiFi();
  initOLED();

  server.on("/", []() {
    server.send(200, "text/html", web);
  });

  server.begin(); 
  webSocket.begin(); 
  webSocket.onEvent(webSocketEvent); 

  Serial.println("HTTP & WebSocket Servers Started!");
}

void loop() {
  server.handleClient();
  webSocket.loop();
  delay(2);
}