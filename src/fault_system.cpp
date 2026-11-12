// glue between the hardware and the fsm / fault checks in lib/gs_core
#include "fault_system.h"
#include "network.h"
#include "display.h"
#include "mpu_sensor.h"
#include "pid.h"
#include "fault_checks.h"
#include "fault_fsm.h"

#include <DHT.h>
#include <NewPing.h>

#define DHTTYPE DHT22
#define MAX_DISTANCE 200

#define TRIG_PIN 12
#define BUTTON_PIN 13
#define ECHO_PIN 14
#define TEMP_HUMID_PIN 23

// ENB/IN4 were on 34/35, which are input-only on the ESP32
#define ENA 26
#define IN1 25
#define IN2 33
#define ENB 27
#define IN3 32
#define IN4 4

//* component configurations
NewPing sensor(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);
DHT dht(TEMP_HUMID_PIN, DHTTYPE);
int distance = 0;          // 0 = no echo
float temperature = NAN;   // NAN until the first good read
unsigned long lastPingRead = 0;
unsigned long lastDhtRead = 0;

// fsm and fault check state
FaultFsm fsm;
FaultChecker checker;
WarnCode lastWarn = WARN_NONE;

// set by the websocket callback, used once by the fsm
bool ackRequested = false;
bool resetRequested = false;

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
    fsmInit(fsm);
    checkerInit(checker);
}

void readSensors() {
    // ultrasonic every 100 ms, dht22 every 2 s (it can't go faster)
    if (millis() - lastPingRead >= 100) {
        lastPingRead = millis();
        distance = sensor.ping_cm();
    }
    if (millis() - lastDhtRead >= 2000) {
        lastDhtRead = millis();
        temperature = dht.readTemperature();
    }
}

void showWarning(WarnCode warn) {
    // send a new warning once, not every loop
    if (warn != lastWarn && warn != WARN_NONE) triggerAlert(warnName(warn));
    lastWarn = warn;
}

void updateFSM() {
    readSensors();
    SensorInputs s = {(uint32_t)millis(), distance, temperature,
                      getPitch(), getRoll(), (uint32_t)lastPingTime};
    FaultCode fault = checkFaults(checker, s);

    bool buttonDown = (digitalRead(BUTTON_PIN) == LOW);
    FsmInputs in = {WiFi.status() == WL_CONNECTED, buttonDown,
                    ackRequested, resetRequested, fault, (uint32_t)millis()};
    ackRequested = false;
    resetRequested = false;

    if (fsmStep(fsm, in)) {
        Serial.print(F("STATE: "));
        Serial.println(stateName(fsm.state));
        if (fsm.state == ST_FAULT) {
            triggerAlert(faultName(fsm.activeFault));
            printOLED(faultName(fsm.activeFault));
        }
    }
    showWarning(checkWarnings(s));

    // motors only run in MANUAL
    if (fsmMotorsAllowed(fsm)) runPID();
    else stopMotors();
}

String getStateString() {
    return stateName(fsm.state);
}

void stopMotors() {
    // stop all motor driver pins and clear pwm outputs
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
}

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
