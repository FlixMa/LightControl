
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

cLight::cLight(int identifier, bool invertsDimDirectionOnStop, int outputPin, int manualTriggerPin = 0) {
    if (identifier == 0) {
        TRANSMITLN("WARNING: Usage of the broadcast adress (0) as an identifier for a light. This may lead to unexpected behaviour!");
    }

    this->identifier = identifier;
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

    //DEBUG_PRINTLN("\n###############");
    //DEBUG_PRINTLN("#    START    #\n");

    if (
        isPoweredOn()
        && this->dimmingState != WAITING_FOR_COMMAND
        && this->dimmingState != AUTOMATIC_MODE
    ) {
        int brightnessStroke = (this->maximumBrightness - this->minimumBrightness);

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
                unsigned int timeToPeak = ((float) (this->maximumBrightness - this->lastBrightnessWhenIdling)) / brightnessStroke * TIME_DIM_RISING;
                if (timeDelta >= timeToPeak) {
                    // We've reached the peak
                    // -> Enter next state.
                    unsigned long timeOverdue = timeDelta - timeToPeak;

                    this->dimmerWillRise = false;
                    setDimmingState(
                        WAITING_AT_PEAK,
                        currentTime - timeOverdue
                    );
                    this->lastBrightnessWhenIdling = this->maximumBrightness;
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
                unsigned int timeToLow = ((float) (this->lastBrightnessWhenIdling - this->minimumBrightness)) / brightnessStroke * TIME_DIM_FALLING;
                if (timeDelta >= timeToLow) {
                    // We've reached the low
                    // -> Enter next state.
                    unsigned long timeOverdue = timeDelta - timeToLow;

                    this->dimmerWillRise = true;
                    setDimmingState(
                        WAITING_AT_LOW,
                        currentTime - timeOverdue
                    );

                    this->lastBrightnessWhenIdling = this->minimumBrightness;
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
            case WAITING_FOR_AUTOMATIC_MODE:{
                if (timeDelta >= TIME_WAITING_FOR_AUTOMATIC_MODE_AFTER_POWER_OFF) {
                    // We've been waiting long enough for the user to enter automatic mode.
                    // -> Enter POWERED_OFF.
                    turnOff();
                }
            break;
            default: break;
        }

       DEBUG_PRINT("StateAfter: "); DEBUG_PRINTLN(this->dimmingState);

       // Do not overwrite brightness when prepapring to turn off.
       if (this->dimmingState != WAITING_FOR_AUTOMATIC_MODE) {

            // 1.3. Calculate current brightness

            int deltaRising = (this->dimmingState == DIM_RISING)  ? brightnessStroke * timeRisingFalling / TIME_DIM_RISING   : 0;
            int deltaFalling = (this->dimmingState == DIM_FALLING)? brightnessStroke * timeRisingFalling / TIME_DIM_FALLING  : 0;

            int calculatedBrightness = this->lastBrightnessWhenIdling + deltaRising - deltaFalling;

            DEBUG_PRINT("brightness = "); /*DEBUG_PRINTLN(this->lastBrightnessWhenIdling);

            DEBUG_PRINT("             + "); DEBUG_PRINT(this->dimmingState); DEBUG_PRINT(" == "); DEBUG_PRINT(DIM_RISING); DEBUG_PRINT(" ? "); DEBUG_PRINT(brightnessStroke); DEBUG_PRINT(" * "); DEBUG_PRINT(timeRisingFalling); DEBUG_PRINT(" / "); DEBUG_PRINT(TIME_DIM_RISING); DEBUG_PRINTLN(" : 0");

            DEBUG_PRINT("             - "); DEBUG_PRINT(this->dimmingState); DEBUG_PRINT(" == "); DEBUG_PRINT(DIM_FALLING); DEBUG_PRINT(" ? "); DEBUG_PRINT(brightnessStroke); DEBUG_PRINT(" * "); DEBUG_PRINT(timeRisingFalling); DEBUG_PRINT(" / "); DEBUG_PRINT(TIME_DIM_FALLING); DEBUG_PRINTLN(" : 0");

            DEBUG_PRINTLN("");

            DEBUG_PRINT("           = "); DEBUG_PRINT(this->lastBrightnessWhenIdling); DEBUG_PRINT(" + "); DEBUG_PRINT(deltaRising); DEBUG_PRINT(" - "); DEBUG_PRINTLN(deltaFalling);

            DEBUG_PRINT("           = ");*/ DEBUG_PRINTLN(calculatedBrightness);

            setBrightness(calculatedBrightness);
        }
    }

    manualControl();
    applyBrightness();

    //DEBUG_PRINTLN("\n#     ENDE    #");
    //DEBUG_PRINTLN("###############\n");
}

void cLight::manualControl() {
    bool buttonIsPressed = readInput();

    // Debounce buttons.
    bool isDebouncing = (millis() - this->manuallyTriggeredTime) < TIME_DEBOUNCE_BUTTON;

    // wenn steigende flanke und nicht mehr am debouncen
    // -> starte debounce und halte state auf HIGH

    // wenn fallende flanke und nicht mehr am debouncen
    // -> starte debounce und halte state auf low


    if (!this->buttonWasPressed && buttonIsPressed && !isDebouncing) {
        DEBUG_PRINT("Manual Mode: ") DEBUG_PRINT(this->dimmingState); DEBUG_PRINTLN(" | Button Pressed.");
        // Rising Edge detected.
        // -> Button pressed.

        // Start debouncing and therefore lock it in this state.
        this->manuallyTriggeredTime = millis();
        this->buttonWasPressed = true;

        switch (this->dimmingState) {
            case WAITING_FOR_AUTOMATIC_MODE:
                // Enable automatic mode
                enterAutomaticMode();
            break;

            case AUTOMATIC_MODE:
                exitAutomaticMode();
                //NOTE: This one is intended to fall trough!

            case WAITING_FOR_COMMAND:
                startDimming();
            break;

            case POWERED_OFF:
                turnOn();
            break;

            default:
            break;
        }


    } else if (this->buttonWasPressed && !buttonIsPressed && !isDebouncing) {
        DEBUG_PRINT("Manual Mode: ") DEBUG_PRINT(this->dimmingState); DEBUG_PRINTLN(" | Button released.");
        // Falling Edge detected.
        // -> Button released.

        // Start debouncing and therefore lock it in this state.
        this->manuallyTriggeredTime = millis();
        this->buttonWasPressed = false;

        switch (this->dimmingState) {
            case WAITING_FOR_COMMAND:
                // We just turned on
            break;

            case AUTOMATIC_MODE:
                // We entered automatic mode (there is no
                // other possibility to reach this case here).
                //
                // -> Leave all as it is.
            break;

            case WAITING_FOR_DIM_TO_START:
                // If dimming hasn't started yet, just turn the light off.
                prepareToTurnOff();
            break;

            default:
                stopDimming();
            break;
        }

    }

}

void cLight::setDimmingState(enum DimmingState newState, unsigned long timeChanged = 0) {

    if (newState == WAITING_FOR_COMMAND && this->dimmingState != POWERED_OFF) {
        // save brightness for next startup
        DEBUG_PRINT("set lastBrightnessWhenIdling to "); DEBUG_PRINTLN(this->lastBrightnessWhenIdling);
        this->lastBrightnessWhenIdling = this->brightness;
    }

    this->dimmingState = newState;
    this->lastStateChange = (timeChanged != 0) ? timeChanged : millis();


    DEBUG_PRINT("setDimmingState -> ");
    DEBUG_PRINT(newState); DEBUG_PRINT(" | ");
    DEBUG_PRINTLN(this->lastStateChange);
}


//MARK: - Power On / Off

void cLight::turnOn() {
    DEBUG_PRINTLN("turning light on");
    if (!isPoweredOn()) {
        setDimmingState(WAITING_FOR_COMMAND);

        DEBUG_PRINT("lastBrightnessWhenIdling is ") DEBUG_PRINTLN(this->lastBrightnessWhenIdling);

        // Reapply brightness, which has been active when light was turned off.
        setBrightness(constrain(
            this->lastBrightnessWhenIdling,
            max(this->minimumBrightness, 10), min(this->maximumBrightness, 75)
            //TODO: Maybe remove this min and max
        ));

    }
}

void cLight::prepareToTurnOff() {
    if (isPoweredOn()) {
        DEBUG_PRINTLN("preparing to turn the light off");
        setDimmingState(WAITING_FOR_AUTOMATIC_MODE);
        setBrightness(0);
    }
}

void cLight::turnOff() {
    if (isPoweredOn()) {
        DEBUG_PRINTLN("turning light off");
        setDimmingState(POWERED_OFF);
        setBrightness(0);
    }
}


//MARK: - Automatic Mode

void cLight::enterAutomaticMode() {
    if (!isInAutomaticMode()) {
        turnOn();
        setDimmingState(AUTOMATIC_MODE);
    }
}

void cLight::exitAutomaticMode() {
    if (isInAutomaticMode()) {
        setDimmingState(WAITING_FOR_COMMAND);
    }
}

bool cLight::isInAutomaticMode() {
    return this->dimmingState == AUTOMATIC_MODE;
}


//MARK: - Dimming

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
            prepareToTurnOff();
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


//MARK: - Brightness Getter/Setter

void cLight::setBrightness(int value) {
    this->brightness = constrain(value, 0, 100);

    if (
        this->dimmingState == WAITING_FOR_COMMAND
        || (this->dimmingState == POWERED_OFF && this->brightness > 0)
    ) {
        this->lastBrightnessWhenIdling = this->brightness;
        DEBUG_PRINT("set lastBrightnessWhenIdling to "); DEBUG_PRINTLN(this->lastBrightnessWhenIdling);
    }

    DEBUG_PRINT("set brightness to "); DEBUG_PRINTLN(this->brightness);
}

int cLight::getBrightness() {
    return isPoweredOn() ? this->brightness : 0;
}

void cLight::setBrightnessBounds(int minimum, int maximum) {
    this->minimumBrightness = constrain(minimum, 0, 100);
    this->maximumBrightness = constrain(maximum, 0, 100);
}


//MARK: - Input / Output

int cLight::readInput() {
    return (this->manualTriggerPin != 0 && !digitalRead(this->manualTriggerPin)) ? HIGH : LOW;
}

void cLight::applyBrightness() {
    int target = getBrightness();
    if (target != this->lastAppliedBrightness) {
        // We have to adjust the output.

        int diff = abs(target - this->lastAppliedBrightness);

        float smoothedTarget =
                diff > 2 && target != 0
            ?   (float) this->lastAppliedBrightness * (1.0f - this->smoothingFactor)
                + (float) target * this->smoothingFactor
            :   target;

        // map value to the 8bit pwm duty cycle and write it out
        int pwm = (int) (smoothedTarget * 255.0f / 100.0f);
        analogWrite(this->outputPin, pwm);
        this->lastAppliedBrightness = (int) smoothedTarget;

        // Let's notify the connected peer about the change.
        notifyPeer();
    }
}


//MARK: - JSON Analysis

void cLight::readJSON(String command) {
    char json[60];
    command.toCharArray(json, 60);

    StaticJsonBuffer<60>  buffer;
    JsonObject& root = buffer.parseObject(json);

    if (!root.success()) {
        TRANSMITLN("IT DOESNT WORK!");
        return;
    }

    JsonVariant lightIdentifier = root["identifier"];
    JsonVariant desiredBrightness = root["brightness"];
    int desiredPowerState = root["power"];

    if (
        lightIdentifier.success()
        && (lightIdentifier.as<int>() == this->identifier
        || lightIdentifier.as<int>() == 0)
    ) {
        DEBUG_PRINT("JSON for light: "); DEBUG_PRINTLN(lightIdentifier.as<int>());

        // set brightness if key is available
        if (desiredBrightness.success()) {
            int val = desiredBrightness.as<int>();

            DEBUG_PRINT("JSON brightness: "); DEBUG_PRINTLN(val);
            if (val > 0) {
                setBrightness(val);
                turnOn();
            } else {
                turnOff();
            }

        } else {

            // NOTE: we dont need to check the key here
            // -> if its not we get a 0 which triggers the default case and therfore does nothing
            DEBUG_PRINT("JSON power cmd: "); DEBUG_PRINTLN(desiredPowerState);
            switch (desiredPowerState) {
            case 1:
                turnOn();
                break;
            case -1:
                turnOff();
                break;
            default:
                break;
            }

        }
    }
}

void cLight::notifyPeer() {
    if (Serial.availableForWrite() > 60) {
        // the message is at most 60 characters long:
        // {"identifier":1,"brightness":39,"isPoweredOn":true}
        StaticJsonBuffer<60> buffer;

        JsonObject& root = buffer.createObject();

        root["identifier"] = this->identifier;
        root["brightness"] = getBrightness();
        root["isPoweredOn"] = isPoweredOn();

        root.printTo(Serial);
        Serial.println();
    }
}
