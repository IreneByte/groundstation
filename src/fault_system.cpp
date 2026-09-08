#include "network.h"
#include "display.h"

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
#define ENB 34
#define IN3 32
#define IN4 35

//* COMPONENT CONFIGURATIONS
NewPing sensor(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);
long distance;
unsigned long objectDetectedTime = 0;

DHT dht(TEMP_HUMID_PIN, DHTTYPE);
float temperature;

const int distanceThreshold = 15;
const float tempThreshold = 40.0;

enum State {
    IDLE,
    ONLINE, 
    MANUAL,
    FAULT,
    RESET_REQUIRED
};

State currentState = IDLE;
State lastState = RESET_REQUIRED;

const char* currentFault = "None";
int lastButtonReading = HIGH;

static bool faultActive = false;
static bool motorStalled = false;

unsigned long lastPingTime = 0;

// PID Variables
float TARGET_SPEED = 50.0;
unsigned long lastPidTime = 0;
long leftEncoderTicks = 0;
long rightEncoderTicks = 0;

void initFSM() {
    dht.begin();
    Serial.begin(115200);
}

void stateTransitions() {
    switch(currentState) {
        case IDLE:
            if (WiFi.status() == WL_CONNECTED) {
                currentState = ONLINE;
                logState("ONLINE");
                lastState = IDLE;
            }
            break;
        
        case ONLINE:
            if (digitalRead(BUTTON_PIN) == LOW) {
                currentState = MANUAL;  
                logState("MANUAL");
                lastState = ONLINE;
            }
            break;
        
        case MANUAL:
            if (faultActive) {
                currentState = FAULT;   
                logState("FAULT");
                lastState = MANUAL;
            }
            break;
        
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

        default:
            break;
    }
}

void stateLogic() {
    switch(currentState) {
        case IDLE:
            runIdle();
            break;

        case ONLINE:
            runOnline();
            break;
        
        case MANUAL:
            runManual(0.0, 0.0);
            runPID(); // Run PID during manual control
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
    Serial.print(F("STATE: "));
    Serial.println(state);
}

void runIdle() {}
void runOnline() {}

void stopMotors() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
}

void runManual(float pitch, float roll) {
    distance = sensor.ping_cm();

    temperature = dht.readTemperature();
    if (isnan(temperature)) return;

    if (distance > 15 && distance < 30) {
        triggerAlert("W01: Obstacle 15-30 cm");
        printOLED("W01: Obstacle");
    }

    if (temperature > 35 && temperature < 40) {
        triggerAlert("W02: Temperature 35-40 C");
        printOLED("W02: Temperature");
    }

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
    stopMotors();
    printOLED(currentFault);
}

void runReset() {}

float calculatePID(float setPoint, float processValue) {
    float output, error;
    static float reset = 0.0;
    static float lastError = 0.0;
    float K = 1.0;
    float tau_i = 5.0;

    error = setPoint - processValue;
    reset = reset + K/tau_i * error;
    output = K * error + reset + K/tau_i * (error - lastError);
    lastError = error;

    return output;
}

void runPID() {
    if (millis() - lastPidTime >= 100) {
        lastPidTime = millis();

        // Simulated loop ticks so math runs cleanly without hardware
        leftEncoderTicks += 10;
        rightEncoderTicks += 10;

        float leftPwm = calculatePID(TARGET_SPEED, leftEncoderTicks);
        float rightPwm = calculatePID(TARGET_SPEED, rightEncoderTicks);

        leftEncoderTicks = 0;
        rightEncoderTicks = 0;

        analogWrite(ENA, constrain((int)leftPwm, 0, 255));
        analogWrite(ENB, constrain((int)rightPwm, 0, 255));
    }
}