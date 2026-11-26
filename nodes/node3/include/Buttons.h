// Buttons.h
#pragma once
#include <Arduino.h>

class Buttons {
private:
    static void IRAM_ATTR onPress(void* arg);
    uint8_t _pins[3];

public:
    static constexpr uint8_t COUNT = 3;
    void begin(uint8_t pins[]);
    bool pressed(uint8_t i);   // true once per press, then clears
    bool isDown(uint8_t i);    // current physical state
};