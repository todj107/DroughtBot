#include "scale_controller.h"

ScaleController::ScaleController(){
}

bool ScaleController::begin(uint8_t dout_pin, uint8_t sck_pin, float alpha, float scale) {
    _hx.begin(dout_pin, sck_pin);
    _hx.set_scale(scale);
    _hx.tare(10);
    // Save offset
    _start_offset = _hx.get_offset();

    _stabilizer.begin(alpha);

    return _hx.is_ready();
}

void ScaleController::tare(uint8_t n) {
    _hx.tare(n);
    _stabilizer.reset();
}

float ScaleController::get_stable_reading(uint8_t n){
    float v_raw = _hx.get_units(n);
    _stabilizer.add(v_raw);
    float v_stable = _stabilizer.getValue();

    return v_stable;
}

float ScaleController::get_raw_reading(uint8_t n) {
    return _hx.get_units(n);
}

void ScaleController::reset_to_start_offset(){
    _hx.set_offset(_start_offset);
    _stabilizer.reset();
}

bool ScaleController::is_ready() {
    return _hx.is_ready();
}
