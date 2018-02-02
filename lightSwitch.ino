#include "cLight.h"

#define OUT (8)
#define IN (52)

cLight* light;

// the setup function runs once when you press reset or power the board
void setup() {
    Serial.begin(115200);
    Serial.setTimeout(50);

    light = new cLight(OUT, IN);
    light->debugMode = false;
}

float brightness = 0.0f;
unsigned long lastLogged = 0;

bool capturedInput = false;

void loop() {

    // 1. Read target value from rc stream
    //    and set it as target
    if (Serial && Serial.available() > 0) {
        int rawReading = Serial.parseInt();
        int bounded = constrain(rawReading, 0, 100);
        brightness = (float) bounded / 100.0f;
        Serial.println(rawReading);


        // Set value as target
        light->setTargetBrightness(brightness);
    }


    // 2. evaluate current status
    light->evaluate();

    // 3. print current brightness
    if (Serial && millis() - lastLogged > 100) {
        String target = String(light->targetBrightness);
        String approx = String(light->approximateBrightness);
        String turnedOn = String(light->getDimmingState());
        Serial.println("Brightness: " + approx + " -> " + target + " | on: " + turnedOn);
        lastLogged = millis();
    }
}
