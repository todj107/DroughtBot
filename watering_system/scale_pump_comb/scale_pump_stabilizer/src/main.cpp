#include <Arduino.h>
#include "scale_controller.h"
#include "pump_controller.h"

ScaleController scale;
PumpController pump(2);

float run(float stop);

void setup() {
    Serial.begin(115200);
    scale.begin(6, 5, 0.3f);
    pump.begin();

    delay(3000);
}

void loop() {
    Serial.print("Enter ml water to be added: ");
    float goal = scale.readInput().toFloat();

    // Zero the scale
    scale.tare();

    // Run the pump on full speed until 2 grams of goal weight.
    Serial.println("Running full speed");
    pump.runFullSpeed();
    run(goal - 2.0f);

    // 2 grams away from goal weight. Run the pump on slow mode.
    Serial.println("Running low speed");
    pump.runSlowSpeed();
    float final_value = run(goal);

    // Stop the pump
    pump.stop();

    // Final prints
    Serial.println("-----------------\n     STOP    \n---------------");
    Serial.print("\nStop value: ");
    Serial.println(final_value);

    Serial.print("Avg after: ");
    Serial.println(scale.get_stable_reading(20));
    Serial.println();
}

/**
 * @brief Pumps water until stop weight is reached.
 * @param stop Weight in grams.
 */
float run(float stop) {
    float value;
    do {
        delay(10);
        value = scale.get_stable_reading();
        Serial.println(value);
    } while (value < stop);

    return value;
}

