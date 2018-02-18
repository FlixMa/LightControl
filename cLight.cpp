
#include "cLight.h"

#define TRANSMIT(x) {\
    if (Serial) {\
        Serial.print(x);\
    }\
}

#define TRANSMITLN(x) {\
    if (Serial) {\
        Serial.println(x);\
    }\
}

#define DEBUG_PRINT(x) {\
    if (this->debugMode) {\
        TRANSMIT(x);\
    }\
}

#define DEBUG_PRINTLN(x) {\
    if (this->debugMode) {\
        TRANSMITLN(x);\
    }\
}

cLight::cLight(bool invertsDimDirectionOnStop, int outputPin, int manualTriggerPin = 0) {
    this->invertsDimDirectionOnStop = invertsDimDirectionOnStop;
    this->outputPin = outputPin;
    this->manualTriggerPin = manualTriggerPin;

    pinMode(outputPin, OUTPUT);

    if (manualTriggerPin != 0) {
        pinMode(manualTriggerPin, INPUT_PULLUP);
    }

}

enum DimmingState cLight::getDimmingState() {
    return this->dimmingState;
}

bool cLight::isPoweredOn() {
    return this->dimmingState != POWERED_OFF;
}

void cLight::evaluate() {

    DEBUG_PRINTLN("\n###############");
    DEBUG_PRINTLN("#    START    #\n");

    if (this->dimmingState != WAITING_FOR_COMMAND) {


        // 1. Look at the current dimming state

        // 1.1. How much time has passed?
        unsigned long currentTime = millis();
        unsigned long timeDelta = currentTime - this->lastStateChange;

        DEBUG_PRINT("timeDelta: "); DEBUG_PRINTLN(timeDelta);

        unsigned long timeRisingFalling = timeDelta;

        DEBUG_PRINT("StateBefore: "); DEBUG_PRINTLN(this->dimmingState);

        // 1.2. Has a current state expired?
        switch (this->dimmingState) {
            case WAITING_FOR_DIM_TO_START:{
                if (timeDelta >= TIME_WAITING_FOR_START) {
                    // We're already RISING / FALLING
                    // -> Enter next state.
                    unsigned long timeOverdue = timeDelta - TIME_WAITING_FOR_START;

                    setDimmingState(
                        (this->dimmerWillRise) ? DIM_RISING : DIM_FALLING,
                        currentTime - timeOverdue
                    );
                    timeRisingFalling = timeOverdue;
                }
            }
            break;
            case DIM_RISING:{
                unsigned int timeToPeak = (1.0f - this->lastBrightnessWhenIdling) * TIME_DIM_RISING;
                if (timeDelta >= timeToPeak) {
                    // We've reached the peak
                    // -> Enter next state.
                    unsigned long timeOverdue = timeDelta - timeToPeak;

                    this->dimmerWillRise = false;
                    setDimmingState(
                        WAITING_AT_PEAK,
                        currentTime - timeOverdue
                    );
                    this->lastBrightnessWhenIdling = 1.0f;
                }
            }
            break;
            case WAITING_AT_PEAK:{
                if (timeDelta >= TIME_WAITING_AT_PEAK) {
                    // We're already FALLING
                    // -> Enter next state.
                    unsigned long timeOverdue = timeDelta - TIME_WAITING_AT_PEAK;

                    this->dimmerWillRise = false;
                    setDimmingState(
                        DIM_FALLING,
                        currentTime - timeOverdue
                    );
                    timeRisingFalling = timeOverdue;
                }
            }
            break;
            case DIM_FALLING:{
                unsigned int timeToLow = this->lastBrightnessWhenIdling * TIME_DIM_FALLING;
                if (timeDelta >= timeToLow) {
                    // We've reached the low
                    // -> Enter next state.
                    unsigned long timeOverdue = timeDelta - timeToLow;

                    this->dimmerWillRise = true;
                    setDimmingState(
                        WAITING_AT_LOW,
                        currentTime - timeOverdue
                    );

                    this->lastBrightnessWhenIdling = 0.0f;
                }
            }
            break;
            case WAITING_AT_LOW:{
                if (timeDelta >= TIME_WAITING_AT_LOW) {
                    // We're already RISING
                    // -> Enter next state.
                    unsigned long timeOverdue = timeDelta - TIME_WAITING_AT_LOW;

                    this->dimmerWillRise = true;
                    setDimmingState(
                        DIM_RISING,
                        currentTime - timeOverdue
                    );
                    timeRisingFalling = timeOverdue;
                }
            }
            break;

            default: break;
        }

       DEBUG_PRINT("StateAfter: "); DEBUG_PRINTLN(this->dimmingState);

        // 1.3. Calculate current brightness

        float deltaRising = (this->dimmingState == DIM_RISING)  ? (float) timeRisingFalling / TIME_DIM_RISING   : 0.0f;
        float deltaFalling = (this->dimmingState == DIM_FALLING)? (float) timeRisingFalling / TIME_DIM_FALLING  : 0.0f;

        this->brightness = this->lastBrightnessWhenIdling + deltaRising - deltaFalling;

        DEBUG_PRINT("brightness = "); DEBUG_PRINTLN(this->lastBrightnessWhenIdling);

        DEBUG_PRINT("             + "); DEBUG_PRINT(this->dimmingState); DEBUG_PRINT(" == "); DEBUG_PRINT(DIM_RISING); DEBUG_PRINT(" ? "); DEBUG_PRINT(timeRisingFalling); DEBUG_PRINT(" / "); DEBUG_PRINT(TIME_DIM_RISING); DEBUG_PRINTLN(" : 0");

        DEBUG_PRINT("             - "); DEBUG_PRINT(this->dimmingState); DEBUG_PRINT(" == "); DEBUG_PRINT(DIM_FALLING); DEBUG_PRINT(" ? "); DEBUG_PRINT(timeRisingFalling); DEBUG_PRINT(" / "); DEBUG_PRINT(TIME_DIM_FALLING); DEBUG_PRINTLN(" : 0");

        DEBUG_PRINTLN("");

        DEBUG_PRINT("           = "); DEBUG_PRINT(this->lastBrightnessWhenIdling); DEBUG_PRINT(" + "); DEBUG_PRINT(deltaRising); DEBUG_PRINT(" - "); DEBUG_PRINTLN(deltaFalling);

        DEBUG_PRINT("           = "); DEBUG_PRINTLN(this->brightness);

    }

    manualControl();
    applyBrightness();

    DEBUG_PRINTLN("\n#     ENDE    #");
    DEBUG_PRINTLN("###############\n");
}

void cLight::manualControl() {
    int newManualOverwrite = readInput();

    if (!this->manualOverwrite && newManualOverwrite) {
        DEBUG_PRINT("Manual Mode: ") DEBUG_PRINT(this->dimmingState); DEBUG_PRINTLN(" | Button Pressed.");
        // Rising Edge detected.
        // -> Button pressed.

        if (this->dimmingState == WAITING_FOR_COMMAND) {
            startDimming();
        }

        //TODO: Can't turn lamp on or off. Needs detection for short impulses!
        //TODO: Does not work if we're already dimming using auto mode...

    } else if (this->manualOverwrite && !newManualOverwrite) {
        DEBUG_PRINT("Manual Mode: ") DEBUG_PRINT(this->dimmingState); DEBUG_PRINTLN(" | Button released.");
        // Falling Edge detected.
        // -> Button released.

        switch (this->dimmingState) {
            case POWERED_OFF:
                turnOn();
            break;

            case WAITING_FOR_DIM_TO_START:
                // If dimming hasn't started yet, just turn the light off.
                turnOff();
            break;

            default:
                stopDimming();
            break;
        }
    }

    this->manualOverwrite = newManualOverwrite;
}

void cLight::setDimmingState(enum DimmingState newState, unsigned long timeChanged = 0) {
    this->dimmingState = newState;
    this->lastStateChange = (timeChanged != 0) ? timeChanged : millis();

    if (newState == WAITING_FOR_COMMAND) {
        this->lastBrightnessWhenIdling = this->brightness;
    }

    DEBUG_PRINT("setDimmingState -> ");
    DEBUG_PRINT(newState); DEBUG_PRINT(" | ");
    DEBUG_PRINTLN(this->lastStateChange);
}

void cLight::turnOn() {
    if (!isPoweredOn()) {
        DEBUG_PRINTLN("turnOn");

        // Reapply brightness, which has been active when turned off.
        this->brightness = (this->brightness > 0.1)
                            ? this->brightness
                            : 0.1;

        setBrightness(this->brightness);
        setDimmingState(WAITING_FOR_COMMAND);
    }
}

void cLight::turnOff() {
    if (isPoweredOn()) {
        DEBUG_PRINTLN("turnOff");
        setBrightness(0.0f);
    }
    setDimmingState(POWERED_OFF);
}

void cLight::startDimming() {
    // Check if dimming isn't already in progress.

    if (this->dimmingState == WAITING_FOR_COMMAND) {
        // It's not. -> Start.

        DEBUG_PRINT("startDimming at "); DEBUG_PRINTLN(this->brightness);

        setDimmingState(WAITING_FOR_DIM_TO_START);
    }
}

void cLight::stopDimming() {
    // Check if dimming isn't alreay halted.

    switch (this->dimmingState) {
        case WAITING_FOR_DIM_TO_START:
            // When Button is released to early, the light will turn off.
            turnOff();
        break;

        case DIM_RISING: case WAITING_AT_PEAK: case DIM_FALLING: case WAITING_AT_LOW:
            // It's not. -> Stop.

            setDimmingState(WAITING_FOR_COMMAND);

            if (this->invertsDimDirectionOnStop) {
                this->dimmerWillRise = !this->dimmerWillRise;
            }

            DEBUG_PRINT("stopDimming at "); DEBUG_PRINTLN(this->brightness);
        break;
        default: break;
    }
}

int cLight::readInput() {
    return (this->manualTriggerPin != 0 && !digitalRead(this->manualTriggerPin)) ? HIGH : LOW;
}

void cLight::setBrightness(float value) {
    DEBUG_PRINT(value);
    this->brightness = value;
    if (this->dimmingState == WAITING_FOR_COMMAND) {
        this->lastBrightnessWhenIdling = this->brightness;
    }
}

float cLight::getBrightness() {
    return isPoweredOn() ? this->brightness : 0.0f;
}

void cLight::applyBrightness() {
    float target = map(getBrightness(), 0.0f, 1.0f, 0, 255);
    analogWrite(this->outputPin, target);
}
