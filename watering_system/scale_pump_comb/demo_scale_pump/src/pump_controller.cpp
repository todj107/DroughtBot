#include "pump_controller.h"

// Runs when you create the pump object and saves the pin number
PumpController::PumpController(uint8_t pin){
    pumpPin = pin;
}


void PumpController::begin() {
    ESP32PWM::allocateTimer(0);
    pumpServo.setPeriodHertz(50);
    pumpServo.writeMicroseconds(1500); // Make sure pump starts in STOP state
    pumpServo.attach(pumpPin, 500, 2500);
    Serial.println("PUMP Initialized and ready.");
}

void PumpController::runFullSpeed() {
    pumpServo.writeMicroseconds(500);
}

void PumpController::runSlowSpeed() {
    pumpServo.writeMicroseconds(800);
}

void PumpController::stop() {
    pumpServo.writeMicroseconds(1500);
}
