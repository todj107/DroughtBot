#include "scale_controller.h"

ScaleController::ScaleController(uint8_t dout_pin, uint8_t sck_pin){
    _dout_pin = dout_pin;
    _sck_pin = sck_pin;
}

void ScaleController::begin(int window_size, float weight_threshold, float scale) {
    _hx.begin(_dout_pin, _sck_pin);
    _hx.set_scale(scale);
    _hx.tare(10);

    _stabilizer.begin(window_size, weight_threshold);
}

void ScaleController::tare(uint8_t n) {
    _hx.tare(n);
}

float ScaleController::get_stable_reading(uint8_t n){
    float v_raw = _hx.get_units(n);
    _stabilizer.add(v_raw);
    float v_stable = (float)_stabilizer.getStablilizedReading();

    return v_stable;
}

float ScaleController::get_raw_reading(uint8_t n) {
    return _hx.get_units(n);
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