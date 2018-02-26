#ifndef _LIGHT_H_
#define _LIGHT_H_

#include "Arduino.h"
#include <ArduinoJson.h>

#define TIME_DEBOUNCE_BUTTON    (100)

#define TIME_WAITING_FOR_START  (500)
#define TIME_DIM_RISING         (3000)
#define TIME_WAITING_AT_PEAK    (200)
#define TIME_DIM_FALLING        (3000)
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

    cLight(int identifier, bool invertsDimDirectionOnStop, int outputPin);
    cLight(int identifier, bool invertsDimDirectionOnStop, int outputPin, int manualTriggerPin);

    /**
     * Sets the brightness of this light.
     * @param brightness in percent.
     */
     void setBrightness(int value);
    /**
     * Returns the current brightness.
     * @return brightness in percent.
     */
     int getBrightness();

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
     * Flag, indicating whether the controller is in debug mode.
     * This leads to debug prints via a serial connection.
     *
     * The lower the value, the more verbose it is.
     */
    bool debugMode = false;

    void turnOn();
    void turnOff();

    /**
     * Reads the desired State from a JSON string.
     * It should look like this:
     *
     * @param json The string to read. It should be in this format:
     *      {
     *          "identifier": 1,
     *          "brightness": 63,
     *          "power":-1
     *      }
     */
    void readJSON(String command);


    /**
     * Sets the boundaries of the brightness value for this light.
     * The brightness, which is set by the dimmer/serial communication
     * will then be interpolated in this interval
     *
     * i.e. 100% maps to the maximum and 0% brightness maps to the minimum.
     *
     * @param minimum the maximum brightness of this light
     * @param maximum the minimum brightness of this light
     */
    void setBrightnessBounds(int minimum, int maximum);

private:

    /**
     * A unique identification of this light.
     */
    int identifier;

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
     * Flag, indicating whether the button button was pressed
     * during last cycle (including the debounce time).
     */
    bool buttonWasPressed = false;

    /**
     * The first time the button issued a rising edge.
     * This is used in conjunction with TIME_DEBOUNCE_BUTTON to debounce button inputs.
     */
    unsigned long manuallyTriggeredTime = 0;


    //MARK: Brightness

    /**
     * The current brightness.
     */
    int brightness = 0;

    /**
     * The brightness of the light at the point in time
     * when the dimmer was idling.
     */
    int lastBrightnessWhenIdling = 0;

    /**
     * The last brightness to which the PWM signal has been adjusted to.
     */
    int lastAppliedBrightness = -1;


    /**
     * The upper bound of brightness for this light. See setBrightnessBounds().
     */
    int maximumBrightness = 100;

    /**
     * The lower bound of brightness for this light. See setBrightnessBounds().
     */
    int minimumBrightness = 0;



    //MARK: DIMMING

    bool invertsDimDirectionOnStop = false;

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

    void startDimming();
    void stopDimming();

    int readInput();
    void applyBrightness();

    /**
     * Writes the current state of the light to the peer connected via Serial.
     *
     * A JSON String is sent in the following format:
     *  {
     *      "identifier"    : 1,
     *      "brightness"    : 75,
     *      "isPoweredOn"   : 1
     *  }
     */
    void notifyPeer();
};

#endif
