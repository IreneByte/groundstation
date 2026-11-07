#include "fault_system.h"
#include "network.h"
#include "display.h"
#include "mpu_sensor.h"
#include "pid.h"

#include <DHT.h>
#include <NewPing.h>

#define DHTTYPE DHT22
#define MAX_DISTANCE 200

#define TRIG_PIN 12
#define BUTTON_PIN 13
#define ECHO_PIN 14
#define TEMP_HUMID_PIN 23

#define ENA 26
#define IN1 25
#define IN2 33
// ENB/IN4 were on 34/35, which are input-only on the ESP32
#define ENB 27
#define IN3 32
#define IN4 4

//* component configurations
NewPing sensor(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);
long distance;
unsigned long objectDetectedTime = 0;

DHT dht(TEMP_HUMID_PIN, DHTTYPE);
float temperature;

const int distanceThreshold = 15;
const float tempThreshold = 40.0;

// finite state machine states
enum State {
    IDLE,
    ONLINE, 
    MANUAL,
    FAULT,
    RESET_REQUIRED
};

State currentState = IDLE;

// fault monitoring
const char* currentFault = "None";

// not static: network.cpp uses it too (extern in network.h)
bool faultActive = false;

// pid control variables, one pid per wheel
Pid leftPid;
Pid rightPid;
float TARGET_SPEED = 60.0; // encoder ticks per second
unsigned long lastPidTime = 0;
long leftEncoderTicks = 0;
long rightEncoderTicks = 0;

void initFSM() {
    // set up sensor and motor pins
    dht.begin();
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // button goes to GND, LOW = pressed
    const int motorPins[] = {ENA, IN1, IN2, ENB, IN3, IN4};
    for (int pin : motorPins) pinMode(pin, OUTPUT);

    // gains: kp, ki, kd, output limit 0..255 (pwm)
    pidInit(leftPid, 1.5, 4.0, 0.02, 0, 255);
    pidInit(rightPid, 1.5, 4.0, 0.02, 0, 255);
}

void stateTransitions() {
    // handle transitions between fsm states
    switch(currentState) {
        case IDLE:
            if (WiFi.status() == WL_CONNECTED) {
                currentState = ONLINE;
                logState("ONLINE");
            }
            break;
        
        case ONLINE:
            if (digitalRead(BUTTON_PIN) == LOW) {
                currentState = MANUAL;  
                logState("MANUAL");
            }
            break;
        
        case MANUAL:
            if (faultActive) {
                currentState = FAULT;   
                logState("FAULT");
            }
            break;
        
        case FAULT:
            if (digitalRead(BUTTON_PIN) == LOW) {
                currentState = RESET_REQUIRED;
                logState("RESET_REQUIRED");
            }
            break;
        
        case RESET_REQUIRED:
            if (digitalRead(BUTTON_PIN) == LOW) {
                faultActive = false;
                currentState = IDLE;
                logState("IDLE");
            }
            break;

        default:
            break;
    }
}

void stateLogic() {
    // execute actions corresponding to current fsm state
    switch(currentState) {
        case IDLE:
            runIdle();
            break;

        case ONLINE:
            runOnline();
            break;
        
        case MANUAL:
            runManual(getPitch(), getRoll());
            runPID(); // run pid during manual control mode
            break;

        case FAULT:
            runFault();
            break;

        case RESET_REQUIRED:
            runReset();
            break;        

        default:
            break;
    }
}

String getStateString() {
    // return string representation of current state
    switch(currentState) {
        case IDLE: return "IDLE";
        case ONLINE: return "ONLINE";
        case MANUAL: return "MANUAL";
        case FAULT: return "FAULT";
        case RESET_REQUIRED: return "RESET_REQUIRED";
    }
    return "UNKNOWN";
}

void logState(String state) {
    // print state change to serial monitor
    Serial.print(F("STATE: "));
    Serial.println(state);
}

void runIdle() {}
void runOnline() {}

void stopMotors() {
    // stop all motor driver pins and clear pwm outputs
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
}

void runManual(float pitch, float roll) {
    // read distance and temperature sensors
    distance = sensor.ping_cm();

    temperature = dht.readTemperature();
    if (isnan(temperature)) return;

    // check warning thresholds
    if (distance > 15 && distance < 30) {
        triggerAlert("W01: Obstacle 15-30 cm");
        printOLED("W01: Obstacle");
    }

    if (temperature > 35 && temperature < 40) {
        triggerAlert("W02: Temperature 35-40 C");
        printOLED("W02: Temperature");
    }

    // check critical fault triggers
    if (distance <= distanceThreshold) {
        if (objectDetectedTime == 0) {
            objectDetectedTime = millis();
        }
        if (millis() - objectDetectedTime > 1000) {
            faultActive = true;
            currentFault = "F01: Distance";
            triggerAlert(currentFault);
            printOLED(currentFault);
        }
    }

    if (abs(pitch) > 30.0 || abs(roll) > 30.0) {
        faultActive = true;
        currentFault = "F02: Tilt";
        triggerAlert(currentFault);
        printOLED(currentFault);
    }

    if (temperature >= tempThreshold) {
        faultActive = true;
        currentFault = "F03: Temperature";
        triggerAlert(currentFault);
        printOLED(currentFault);
    } 

    if (millis() - lastPingTime > 500) {
        faultActive = true;
        currentFault = "F04: Watchdog Lost";
        triggerAlert(currentFault);
        printOLED(currentFault);
    }
}

void runFault() {
    // halt system and display active fault
    stopMotors();
    printOLED(currentFault);
}

void runReset() {}

void runPID() {
    // run pid loop every 100ms
    if (millis() - lastPidTime >= 100) {
        float dt = (millis() - lastPidTime) / 1000.0;
        lastPidTime = millis();

        // speed in ticks per second
        float leftSpeed = leftEncoderTicks / dt;
        float rightSpeed = rightEncoderTicks / dt;
        leftEncoderTicks = 0;
        rightEncoderTicks = 0;

        // each wheel has its own pid state now (lib/gs_core/pid.h)
        float leftPwm = pidUpdate(leftPid, TARGET_SPEED, leftSpeed, dt);
        float rightPwm = pidUpdate(rightPid, TARGET_SPEED, rightSpeed, dt);

        analogWrite(ENA, (int)leftPwm);
        analogWrite(ENB, (int)rightPwm);
    }
}
