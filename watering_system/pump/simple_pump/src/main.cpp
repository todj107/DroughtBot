#include <Arduino.h>
#include "pump_controller.h"

// Create a PumpController object named "pump" connected to GIPO pin 9
PumpController pump(9);

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Initialize the pump
    pump.begin();
}

void loop() {
    Serial.println("Running full speed");
    pump.runFullSpeed();
    delay(2000);

    Serial.println("Running slow speed");
    pump.runSlowSpeed();
    delay(2000);

    Serial.println("Stopping");
    pump.stop();
    delay(5000);
}