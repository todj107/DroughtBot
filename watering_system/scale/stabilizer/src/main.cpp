#include <Arduino.h>
#include "scale_controller.h"

ScaleController scale;

void setup() {
    Serial.begin(115200);
    scale.begin();
    delay(3000);
}

void loop() {
    Serial.print("Enter ml water to be added: ");
    float water = scale.readInput().toFloat();

    scale.tare(20);

    float value;
    do {
        delay(10);
        Serial.print("Raw: ");
        Serial.print(scale.get_raw_reading());
        
        value = scale.get_stable_reading();
        Serial.print("  |  Stable: ");
        Serial.println(value);
    } while (value < water);

    Serial.println("-----------------\n     STOP    \n---------------");
    Serial.print("\nStop value: ");
    Serial.println(value);

    Serial.print("Avg after: ");
    Serial.println(scale.get_raw_reading(20));
    Serial.println();
}
