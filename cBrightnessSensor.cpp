
#include "cBrightnessSensor.h"

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

#define fmin(x, y) (x < y ? x : y)
#define fmax(x, y) (x > y ? x : y)
#define fconstrain(x, y, z) (fmin(fmax(fmin(y, z), x), fmax(y, z)))
#define fabs(x)    (x < 0 ? -x : x)


cBrightnessSensor::cBrightnessSensor(
    int inputPin,
    int targetAmbientLightLevel,
    float rangeOfAcceptance
) {
    this->inputPin = inputPin;
    this->targetAmbientLightLevel = targetAmbientLightLevel;
    this->rangeOfAcceptance = rangeOfAcceptance;
}

void cBrightnessSensor::evaluate() {
    this->measuredAmbientLightLevel = this->readBrightness();
    // DEBUG_PRINT(" |  Ambient Light Level: "); DEBUG_PRINT(this->measuredAmbientLightLevel);
    this->recommendedBrightness = this->calculateRecommendedBrightness();
    // DEBUG_PRINT(" | Recommended Brightness: "); DEBUG_PRINTLN(this->recommendedBrightness);
}

int cBrightnessSensor::getRecommendedBrightness() {
    return this->recommendedBrightness;
}

int cBrightnessSensor::getMeasuredAmbientLightLevel() {
    return this->measuredAmbientLightLevel;
}


int cBrightnessSensor::calculateRecommendedBrightness() {
    float error = (float) (this->targetAmbientLightLevel - this->measuredAmbientLightLevel) / 100.0f;

    if (fabs(error) < this->rangeOfAcceptance) { error = 0.0f; }

    // proportional part
    float proportionalPart = this->Kp * (float) error;

    // integral part
    this->accumulatedDifference = fconstrain(
        this->accumulatedDifference + error,
        -1.0f / this->Ki, 1.0f / this->Ki
    );

    float integralPart = this->Ki * this->accumulatedDifference;

    // differential part
    unsigned long timeDelta = this->measurementTime - this->lastMeasurementTime;
    float slope = (float) (this->measuredAmbientLightLevel - this->lastMeasuredAmbientLightLevel) / (float) timeDelta;
    float differentialPart = this->Kd * slope;


    int result = (int) ((proportionalPart + integralPart + differentialPart) * 100.0f);
    DEBUG_PRINT("current: "); DEBUG_PRINT(this->measuredAmbientLightLevel);
    DEBUG_PRINT(" | error: "); DEBUG_PRINT((int) (error * 100));
    DEBUG_PRINT(" | PID: ");
    DEBUG_PRINT(result);
    DEBUG_PRINT(" = (prop) ") DEBUG_PRINT((int) (proportionalPart * 100));
    DEBUG_PRINT(" + (int) ") DEBUG_PRINT((int) (integralPart * 100));
    DEBUG_PRINT(" + (diff) ") DEBUG_PRINT((int) (differentialPart * 100));

    int boundedRes = constrain(
        map(result,
            -100, 100,
            0, 100
        ),
        0, 100
    );

    DEBUG_PRINT(" = (bounded) ") DEBUG_PRINTLN(boundedRes);

    return boundedRes;
}

int cBrightnessSensor::readBrightness() {
    this->lastMeasuredAmbientLightLevel = this->measuredAmbientLightLevel;
    this->lastMeasurementTime = this->measurementTime;

    int measuredValue = analogRead(this->inputPin);
    this->measurementTime = millis();

    // DEBUG_PRINT("Raw Value: "); DEBUG_PRINT(measuredValue);

    float maxPercentage = (float) PHOTO_RESISTOR_UPPER_BOUND / (float) (PHOTO_RESISTOR_UPPER_BOUND + SERIES_RESISTOR_VALUE);
    float minPercentage = (float) PHOTO_RESISTOR_LOWER_BOUND / (float) (PHOTO_RESISTOR_LOWER_BOUND + SERIES_RESISTOR_VALUE);

    int maxValue = (int) (1024.0f * maxPercentage); // less conducting -> low light
    int minValue = (int) (1024.0f * minPercentage); // more conducting -> bright light

    return constrain(map(
        measuredValue,
        minValue, maxValue,
        100, 0
    ), 0, 100);
}
