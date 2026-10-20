#include "scale_controller.h"

ScaleController::ScaleController(uint8_t dout_pin, uint8_t sck_pin){
    _dout_pin = dout_pin;
    _sck_pin = sck_pin;
}

void ScaleController::begin(float scale) {
    _hx.begin(_dout_pin, _sck_pin);
    _hx.set_scale(scale);
    _hx.tare(10);
}

String ScaleController::readInput() {
    while (true) {
        if (Serial.available()) {
            String input = Serial.readString();
            input.trim();

            return input;
        }
        delay(100);
    }
}