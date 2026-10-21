#include <Arduino.h>
#include "scale_controller.h"
#include "pump_controller.h"

ScaleController scale;
PumpController pump(2);

void setup() {
    Serial.begin(115200);
    scale.begin();
    pump.begin();

    delay(3000);
}

void loop() {
    Serial.print("Enter ml water to be added: ");
    float water = scale.readInput().toFloat();

    float value;
    scale.hx().tare(20);

    Serial.println("Running full speed");
    pump.runFullSpeed();
    do {
        delay(10);
        value = scale.hx().get_units(5);
        Serial.println(value);
    } while (value < water - 2);
    pump.runSlowSpeed();
    do {
        delay(10);
        value = scale.hx().get_units(5);
        Serial.println(value);
    } while (value < water);
    pump.stop();

    Serial.println("-----------------\n     STOP    \n---------------");
    Serial.print("\nStop value: ");
    Serial.println(value);

    Serial.print("Avg after: ");
    Serial.println(scale.hx().get_units(20));
    Serial.println();
}

