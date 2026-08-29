#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
#define WIFI_CHANNEL 6

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

void initNetwork() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);

  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }

  Serial.println("Connected");
  Serial.println(WiFi.localIP());

    server.on("/", []() {
    server.send(200, "text/html", web);
  });

  server.begin(); 
  webSocket.begin(); 
  webSocket.onEvent(webSocketEvent); 

  Serial.println("HTTP & WebSocket Servers Started!");
}

void updateNetwork() {
  server.handleClient();
  webSocket.loop();
  delay(2);
}

void sendTelemetry(float currentPitch, float currentRoll) {
    String json = "{";
    json += "\"state\":\"MANUAL\",";
    json += "\"speed_left\":\"N/A\",";
    json += "\"speed_right\":\"N/A\",";
    json += "\"pitch\":" + String(currentPitch, 2) + ",";
    json += "\"roll\":" + String(currentRoll, 2) + ",";
    json += "\"heading\":\"N/A\",";
    json += "\"obstacle_cm\":\"N/A\",";
    json += "\"temperature\":\"N/A\",";
    json += "\"humidity\":\"N/A\",";
    json += "\"fault\":null,";
    json += "\"uptime_ms\":" + String(millis());
    json += "}";

    webSocket.broadcastTXT(json);
}

void handleTelemetry(WebSocketsServer &webSocket, float currentPitch, float currentRoll) {
  static unsigned long lastBroadcast = 0;

  if (millis() - lastBroadcast >= 100) {
    lastBroadcast = millis();
    sendTelemetry(currentPitch, currentRoll);
  }
}
