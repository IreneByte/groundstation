# Groundstation

Groundstation is an ESP32 firmware project for a WiFi-controlled robot. It uses a finite state machine to manage sensor data, a web dashboard, and automatic fault handling instead of using messy `if` statements.
<br/> <br/>
![Platform](https://img.shields.io/badge/Platform-ESP32-%233186A0?style=flat-square)
![Environment](https://img.shields.io/badge/Environment-VSCode-007ACC?style=flat-square)
![Framework](https://img.shields.io/badge/Framework-PlatformIO-F48225?style=flat-square)
![Language](https://img.shields.io/badge/Language-C%2B%2B-orange?style=flat-square)
![Comms](https://img.shields.io/badge/Comms-WebSocket-blue?style=flat-square)
![Status](https://img.shields.io/badge/Status-Complete-success?style=flat-square)

### Wokwi Simulation
<!-- add Wokwi diagram/link here -->

### Schematic Diagram
<!-- add KiCad schematic image here -->

## What It Does

When the ESP32 turns on, it connects to WiFi and starts a web server. A user can open the web page in a browser to see live telemetry data update every 100ms through a WebSocket connection. The web page also sends drive commands to the robot using WASD keys or buttons on the screen.

The robot uses several sensors to measure its environment:
* An HC-SR04 ultrasonic sensor measures distance.
* A DHT22 sensor measures temperature.
* An MPU6050 accelerometer and gyroscope measures tilt, using a filter to calculate pitch and roll.
* An SSD1306 OLED screen displays the current system state and fault codes directly on the robot.

## State Machine & Safety

The program uses a five-state system: `IDLE`, `ONLINE`, `MANUAL`, `FAULT`, and `RESET_REQUIRED`. The robot only moves and checks sensors when it is in the `MANUAL` state. 

If a sensor reading goes past a set limit, the code forces the system into the `FAULT` state and cuts power to the motors. The robot cannot go right back to work from a fault. A user must press a physical button to enter `RESET_REQUIRED`, and then press it again to clear the fault and return to `IDLE`.

```
IDLE --(wifi connects)--> ONLINE --(button)--> MANUAL
                                                |
                                         fault detected
                                                v
RESET_REQUIRED <--(button ack)-- FAULT <-----------+
    |
 button (confirm clear)
    v
   IDLE
```

This code snippet from `fault_system.cpp` shows how the buttons handle the fault states:

```cpp
case FAULT:
    if (digitalRead(BUTTON_PIN) == LOW) {
        currentState = RESET_REQUIRED;
        logState("RESET_REQUIRED");
        lastState = FAULT;
    }
    break;

case RESET_REQUIRED:
    if (digitalRead(BUTTON_PIN) == LOW) {
        faultActive = false;
        currentState = IDLE;
        logState("IDLE");
        lastState = RESET_REQUIRED;
    }
    break;
```

## Hardware

* **ESP32:** Chosen because it has built-in WiFi, which removes the need for an extra networking module.
* **Motor Driver:** Uses a dual H-bridge (L298N layout) to control the motors.
* **I2C Bus:** The OLED screen and the MPU6050 share the same I2C pins because the board has limited pins available.

| Component | Part |
|-----------|------|
| Microcontroller | ESP32 |
| Display | SSD1306 OLED (128x64, I2C) |
| Orientation Sensor | MPU6050 |
| Distance Sensor | HC-SR04 |
| Temperature Sensor | DHT22 |
| Motor Driver | Dual H-bridge, L298N-style pinout |

### Pin Mapping
| Pin | Component | Role |
|-----|-----------|------|
| 12 | HC-SR04 | Trigger |
| 13 | Push Button | Input |
| 14 | HC-SR04 | Echo |
| 23 | DHT22 | Data |
| 25, 26 | Motor A (IN1, ENA) | Output |
| 32-35 | Motor B (IN3, IN4, ENB) | Output |
| I2C (SDA/SCL) | OLED and MPU6050 | Shared Bus |

### Physical Build
<!-- add photos of the finished robot here -->

## Fault and Warning Codes
Code | Source | Description | Trigger Condition
--- | --- | --- | ---
W01 | HC-SR04 | Obstacle warning | Distance between 15 and 30 cm
W02 | DHT22 | Temperature warning | Temperature between 35°C and 40°C
F01 | HC-SR04 | Object jam | Distance under 15 cm for more than 1 second
F02 | MPU6050 | Tilt fault | Pitch or roll exceeds 30 degrees (not currently active, see Design Notes)
F03 | DHT22 | Overtemperature | Temperature at or above 40°C
F04 | Watchdog | Connection lost | No ping from dashboard in 500ms

## Setup
1. Clone the repo and open it in the Arduino IDE or PlatformIO.
2. Install `Adafruit_SSD1306`, `Adafruit_GFX`, `Adafruit_MPU6050`, `Adafruit_Sensor`, the DHT sensor library, `NewPing`, `ArduinoJson`, and `WebSocketsServer` through the Library Manager.
3. Select the ESP32 board and COM port, upload, then check the serial monitor for the board's IP address. Open that address in a browser to load the dashboard.

## Design Choices & Findings
**Broadcast vs. targeted messaging.** Telemetry and fault messages are sent with `broadcastTXT()` rather than tracked per client ID. This keeps the networking code simple, but any browser on the network can connect and issue drive commands, and multiple simultaneous connections have no arbitration between them. Session tokens would fix this and are out of scope for the current build.

**WebSocket over MQTT.** A single persistent WebSocket connection was sufficient for one board talking to one dashboard; MQTT's broker model wasn't necessary at this scale.

**Tilt fault not wired.** F02 does not currently trigger. `runManual()` is called with pitch and roll hardcoded to `0.0` in `stateLogic()`, so the fault check never receives live data, even though `processIMU()` is computing real orientation values elsewhere. This is a known gap, not an intentional omission.

**Duplicate fault removed.** An earlier version included a dedicated motor-stall fault. It was removed because HC-SR04-based jam detection (F01) already covers the same failure case, making the stall check redundant.

## Next Steps
- Wire the live IMU output into the tilt fault check in place of the hard-coded 0.0 placeholder
- Replace simulated PID feedback (fixed encoder ticks) with real encoder input to close the control loop
- Add authentication to the dashboard so only one client can control the robot at a time
