#include <ArduinoJson.h>
#include "cLight.h"

// MARK: Global Variables

cLight* leselampe;
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

    leselampe = new cLight(2, true, 9, 52);
    leselampe->setBrightnessBounds(10, 100);
    //leselampe->debugMode = true;

    fluter = new cLight(1, true, 5, 50);
    fluter->setBrightnessBounds(10, 100);
    fluter->turnOn();
    fluter->setBrightness(50);
    // fluter->debugMode = true;
}

/**
 * This function gets called periodically.
 */
void loop() {

    // 1. Check, if we have a new command ready to parse
    if (commandReady) {

        leselampe->readJSON(command);
        fluter->readJSON(command);

        command = "";
        commandReady = false;
    }

    // 2. evaluate current status
    leselampe->evaluate();
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
