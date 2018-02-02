#ifndef _LIGHT_H_
#define _LIGHT_H_

#include "Arduino.h"

#define TIME_WAITING_FOR_START  (1000)
#define TIME_DIM_RISING         (4000)
#define TIME_WAITING_AT_PEAK    (200)
#define TIME_DIM_FALLING        (4000)
#define TIME_WAITING_AT_LOW     (200)


enum DimmingState {
    POWERED_OFF,
    WAITING_FOR_COMMAND,
    WAITING_FOR_DIM_TO_START,
    DIM_RISING,
    WAITING_AT_PEAK,
    DIM_FALLING,
    WAITING_AT_LOW
};

/**
 * An implementation of a light source controlled by a dimmer.
 * @param outputPin        The output pin used to control the dimmer.
 * @param manualTriggerPin The input pin which registers the physical
 *                         button presses. If not available, pass 0.
 */
class cLight {

public:

    cLight(int outputPin);
    cLight(int outputPin, int manualTriggerPin);

    /**
     * Sets the target brightness of this lamp.
     * @param target brightness in percent.
     */
    void setTargetBrightness(float target);

    /**
     * Evaluates the current states and takes the appropriate actions
     * to reach the targeted brightness.
     */
    void evaluate();

    /**
     * Returns the current state of this light's dimmer.
     * @return the dimming state
     */
    enum DimmingState getDimmingState();

    /**
     * Returns whether the lamp is turned on.
     * @return lamp powered state.
     */
    bool isPoweredOn();

    /**
     * The targeted brightness for this lamp.
     */
    float targetBrightness = 0.0f;

    /**
     * The current brightness roughly approximated.
     */
    float approximateBrightness = 0.0f;

    /**
     * Flag, indicating whether the controller is in debug mode.
     * This leads to debug prints via a serial connection.
     *
     * The lower the value, the more verbose it is.
     */
    bool debugMode = false;

private:

    //MARK: Input / Output

    /**
     * The output pin used to control the dimmer.
     */
    int outputPin;

    /**
     * The input pin which registers the physical button presses.
     */
    int manualTriggerPin;

    /**
     * Flag, indicating whether control is being overwritten manually.
     */
    bool manualOverwrite = false;

    //MARK: DIMMING

    /**
     * An epsilon value, indicating when the target brightness
     * should be considered as reached.
     */
    float brightnessEpsilon = 0.01f;

    /**
     * The brightness of the light at the point in time
     * when the dimmer was idling.
     */
    float lastBrightnessWhenIdling = 0.0f;


    /**
     * The state, the dimmer is in.
     */
    enum DimmingState dimmingState = POWERED_OFF;

    /**
     * the timestamp (milliseconds since startup) of the last state change.
     */
    unsigned long lastStateChange = 0;

    /**
     * Sets the new dimming state an adjusts the lastStateChange time.
     * @param newState the new state.
     */
    void setDimmingState(enum DimmingState newState, unsigned long timeChanged);

    bool dimmerWillRise = true;

    //MARK: Take Actions

    /**
     * Controls the light using the attached button.
     */
    void manualControl();

    /**
     * Controls the light automatically evaluating the targetBrightness property.
     */
    void automaticControl();


    /**
     * Sends a short impulse to toggle the light's state.
     */
    void toggleLight();

    void turnOn();
    void turnOff();

    void startDimming();
    void stopDimming();


    int readInput();
    void setOutput(int value);


};

#endif
