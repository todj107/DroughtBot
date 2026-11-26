#include "ScaleStabilizer.h"

ScaleStabilizer::ScaleStabilizer() {}

void ScaleStabilizer::begin(float alpha){
    _alpha = alpha;
}

void ScaleStabilizer::reset() {
    _filteredValue = 0.0f;
    _initialized = false;
}

void ScaleStabilizer::add(float reading) {
    if (!_initialized) {
        _filteredValue = reading;
        _initialized = true;
    } else {
        _filteredValue = _alpha * reading + (1.0f - _alpha) * _filteredValue;
    }
}

float ScaleStabilizer::getValue() {
    return _filteredValue;
}