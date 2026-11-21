// Buttons.cpp
#include "Buttons.h"

static const uint32_t DEBOUNCE_MS = 40; // Worth testing different settings

static volatile bool pressedFlag[Buttons::COUNT];
static volatile uint32_t lastPress[Buttons::COUNT];

void IRAM_ATTR Buttons::onPress(void* arg) {
    uint8_t i = (uint8_t)(uintptr_t)arg;
    uint32_t now = millis();
    if (now - lastPress[i] > DEBOUNCE_MS) {
        pressedFlag[i] = true;
        lastPress[i] = now;
    }
}

void Buttons::begin(uint8_t pins[]) {
    for (uint8_t i = 0; i < COUNT; i++) {
        _pins[i] = pins[i];
        pinMode(_pins[i], INPUT_PULLUP);
        attachInterruptArg(_pins[i], onPress, (void*)(uintptr_t)i, FALLING);
    }
}

bool Buttons::pressed(uint8_t i) {
    if (pressedFlag[i]) { 
        pressedFlag[i] = false; 
        return true; 
    }
    return false;
}

bool Buttons::isDown(uint8_t i) {
    return digitalRead(_pins[i]) == LOW;
}