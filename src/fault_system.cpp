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
// Ultrasonic pin configuration
NewPing sensor(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);
long distance;
unsigned long objectDetectedTime = 0;

// Temperature and humidity sensor
DHT dht(TEMP_HUMID_PIN, DHTTYPE);
float temperature;

// Sensor thresholds
const int distanceThreshold = 15;
const float tempThreshold = 40.0;

// System states
enum State {
  IDLE,
  ONLINE, 
  MANUAL,
  FAULT,
  RESET_REQUIRED
};

// Tracks current and previous system states
State currentState = IDLE;
State lastState = RESET_REQUIRED;

// Fault statuses
const char* currentFault = "None";

// Tracks previous button state for edge detection
int lastButtonReading = HIGH;

// Fault Tracking
static bool faultActive = false;
static bool motorStalled = false;

// Watchdog timer
unsigned long lastPingTime = 0;

void initFSM() {
    dht.begin();
    Serial.begin(115200);
}

void stateTransitions() {
    switch(currentState) {
        case IDLE:
            //check in if network.cpp ws_connected case is active
            if (WiFi.status() == WL_CONNECTED) {
                currentState = ONLINE;
                logState("ONLINE");

                lastState = IDLE;
            }
            break;
        
        case ONLINE:
            // if button pressed go to manual
            if (digitalRead(BUTTON_PIN) == LOW) {
                currentState = MANUAL;  
                logState("MANUAL");

                lastState = ONLINE;
            }
            break;
        
        case MANUAL:
            // if fault occurs
            if (faultActive) {
                currentState = FAULT;   
                logState("FAULT");

                lastState = MANUAL;
            }
            break;
        
        case FAULT:
            // constantly check if the fault has been resolved, if that condition is true, go to reset_required
            if (digitalRead(BUTTON_PIN) == LOW) {
                currentState = RESET_REQUIRED;
                logState("RESET_REQUIRED");

                lastState = FAULT;
            }
            break;
        
        case RESET_REQUIRED:
            // IF BUTTON pressed go to IDLE
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

void runIdle() {
}

void runOnline() {
}

void stopMotors() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
}

void runManual(float pitch, float roll) {
    //warning and fault detection
    distance = sensor.ping_cm();

    temperature = dht.readTemperature();
    if (isnan(temperature)) return;

    // W01 - Obstacle 15-30 cm away
    if (distance > 15 && distance < 30) {
        triggerAlert("W01: Obstacle 15-30 cm");
        printOLED("W01: Obstacle");
    }

    // W02 - Temperature 
    if (temperature > 35 && temperature < 40) {
        triggerAlert("W02: Temperature 35-40 C");
        printOLED("W02: Temperature");
    }

    // F01 - Obstacle too close < 15 cm
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

    // F02 - Tilt > 30 degrees
    if (abs(pitch) > 30.0 || abs(roll) > 30.0) {
        faultActive = true;
        currentFault = "F02: Tilt";
        triggerAlert(currentFault);
        printOLED(currentFault);
    }

    // F03 - Temperature > 40 degrees
    if (temperature >= tempThreshold)
    {
        faultActive = true;
        currentFault = "F03: Temperature";
        triggerAlert(currentFault);
        printOLED(currentFault);
    } 

    // F04 - Watchdog heartbeat lost
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

void runReset() {
}