#include <ArduinoJson.h>
#include "cLight.h"

// MARK: Global Variables

cLight* light;
cLight* fluter;

String command = "";
bool commandReady = false;

/**
 * This function runs once when the reset button was pressed
 * or the board has been powered on.
 */
void setup() {
    // reserve the capacity
    command.reserve(200);

    Serial.begin(115200);
    Serial.setTimeout(50);

    while(!Serial);

    Serial.println("Arduino is ready!");

    light = new cLight(1, false, 9, 52);
    light->debugMode = false;

    light->turnOn();

    fluter = new cLight(2, false, 5, 50);
}

/**
 * This function gets called periodically.
 */
void loop() {

    // 1. Check, if we have a new command ready to parse
    if (commandReady) {

        char json[60];
        command.toCharArray(json, 60);
        light->readJSON(json);

        command = "";
        commandReady = false;
    }

    // 2. evaluate current status
    light->evaluate();
    fluter->evaluate();
}

/**
 * This function gets called when the serial buffer has new data available.
 */
void serialEvent() {

    while (Serial.available() && !commandReady) {
        char newCharacter = (char) Serial.read();

        if (newCharacter == '\n') {
            commandReady = true;
        } else {
            command += newCharacter;
        }
    }

}
