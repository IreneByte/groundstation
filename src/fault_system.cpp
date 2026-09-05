#include "network.h"

#include <DHT.h>
#include <NewPing.h>

#define DHTTYPE DHT22
#define MAX_DISTANCE 200

#define TRIG_PIN 12
#define BUTTON_PIN 13
#define ECHO_PIN 14
#define TEMP_HUMID_PIN 23

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
const float tempThreshold = 30.0;

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
const char* currentFault = "";
const char* lastFault = "INIT";

// Tracks previous button state for edge detection
int lastButtonReading = HIGH;

// Fault Tracking
static bool faultActive = false;

void initFSM() {
    dht.begin();
    Serial.begin(115200);
}

bool processWebSocket(WStype_t type) {
    return (type == WStype_CONNECTED);
}

void stateTransitions() {
    switch(currentState) {
        case IDLE:
            //check in if network.cpp ws_connected case is active
            if (processWebSocket) {
                currentState = ONLINE;
                lastState = IDLE;
            }
            break;
        
        case ONLINE:
            // if button pressed go to manual
            if (digitalRead(BUTTON_PIN) == LOW) {
                currentState = MANUAL;                
                lastState = ONLINE;
            }
            break;
        
        case MANUAL:
            // if button pressed go to online
            if (digitalRead(BUTTON_PIN) == LOW) {
                currentState = ONLINE;                
                lastState = MANUAL;
            }
            break;
        
        case FAULT:
            // constantly check if the fault has been resolved, if that condition is true, go to reset_required
            if (!faultActive) {
                currentState = RESET_REQUIRED;
                lastState = FAULT;
            }
            break;
        
        case RESET_REQUIRED:
            // IF BUTTON pressed go to IDLE
            if (digitalRead(BUTTON_PIN) == LOW) {
                currentState = IDLE;
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
            runManual();
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

void logState(String state) {
    Serial.print(F("STATE: "));
    Serial.println(state);
}

void runIdle() {
}

void runOnline() {
}

void runManual() {
    //warning and fault detection
    distance = sensor.ping_cm();

    temperature = dht.readTemperature();
    if (isnan(temperature)) return;

    // W01 - Obstacle 15-30 cm away
    if (distance > 15 && distance < 30) {
        triggerAlert("W01: Obstacle 15-30 cm");
    }

    // W02 - Temperature 
    if (temperature > 35 && temperature < 40) {
        triggerAlert("W02: Temperature 35-40 C");
    }

    // F01 - Obstacle too close < 15 cm
    if (distance <= distanceThreshold) {
        if (objectDetectedTime == 0) {
            objectDetectedTime = millis();
        }
        if (millis() - objectDetectedTime > 1000) {
            faultActive = true;
            triggerAlert("F01: Obstacle < 15 cm");
        }
    }

    // F02 - Tilt > 30 degrees


    // F03 - Temperature > 40 degrees
    if (temperature >= tempThreshold)
    {
        faultActive = true;
        triggerAlert("F03: Temperature > 40 C");
    } 

    // F04 - Motor stall

    // F05 - Watchdog heartbeat lost

}

void runFault() {
}

void runReset() {
}