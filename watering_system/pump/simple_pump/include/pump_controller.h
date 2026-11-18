#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>

class PumpController {
private:
    // Hidden internal variables that only PumpController can touch
    Servo pumpServo;
    uint8_t pumpPin;

public:
    // Pass the GIPO pin your pump is connected to (ex. PumpController pump(9);)
    PumpController(uint8_t pin);

    // Public function, commands that the main.cpp is allowed to use
    // Call this once inside setup() to cofigure the PMW timer and attach the servo 
    void begin();

    // Runs the pump at full speed clockwise
    void runFullSpeed();

    // Runs the pump at slower speed (use this for the last few ml to avoid overshooting)
    void runSlowSpeed();

    // Stops pump
    void stop();
};