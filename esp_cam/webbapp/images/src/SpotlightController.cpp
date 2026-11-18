#include <Arduino.h>
#include "SpotlightController.h"

SpotlightController::~SpotlightController() {
    off();
}

void SpotlightController::begin(uint8_t EN_PIN) {
    _EN_PIN = EN_PIN;
    pinMode(_EN_PIN, OUTPUT);
    analogWrite(_EN_PIN, 0);
    _brightness = DEFAULT_BRIGHTNESS;
}

void SpotlightController::change_brightness(uint8_t percent){
    if (percent < 0) {
        percent = 0;
    }
    else if (percent > 100) {
        percent = 100;
    }
    _brightness = map(percent, 0, 100, 0, 255);
}

void SpotlightController::on() {
    analogWrite(_EN_PIN, _brightness);
}

void SpotlightController::off() {
    analogWrite(_EN_PIN, 0);
}
