#ifndef _BRIGHTNESS_SENSOR_H_
#define _BRIGHTNESS_SENSOR_H_

#include "Arduino.h"
#include <ArduinoJson.h>

/** absolute darkness */
#define PHOTO_RESISTOR_UPPER_BOUND (1200000)

/** 150 Ohm using direct light from flashlight */
#define PHOTO_RESISTOR_LOWER_BOUND (150)

/** Measured: 47.5 kOhm */
#define SERIES_RESISTOR_VALUE (47500)


class cBrightnessSensor {

public:

    cBrightnessSensor(
        int inputPin,
        int targetAmbientLightLevel,
        float rangeOfAcceptance
    );

    void evaluate();

    int getRecommendedBrightness();
    int getMeasuredAmbientLightLevel();

    bool debugMode = false;

    float rangeOfAcceptance;

private:

    /**
     * The analog input pin of the photo resistor.
     */
    int inputPin;

    /**
     * The light level we want to reach.
     */
    int targetAmbientLightLevel;

    /**
     * the calculated light brightness which is neccessary
     * to reach the target brightness.
     */
    int recommendedBrightness = 0;

    /**
     * The ambient light level.
     */
    int measuredAmbientLightLevel = 0;
    int lastMeasuredAmbientLightLevel = 0;


    // MARK: - PID Controller Settings

    unsigned long lastMeasurementTime = 0;
    unsigned long measurementTime = 0;
    float accumulatedDifference = 0;

    float Kp = 0.05;
    float Ki = 0.05;
    float Kd = 0.01;

    int calculateRecommendedBrightness();
    int readBrightness();
};

#endif //_BRIGHTNESS_SENSOR_H_
