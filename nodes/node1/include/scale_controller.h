#pragma once

#include <Arduino.h>
#include <HX711.h>
#include "ScaleStabilizer.h"

class ScaleController {
private:
    HX711 _hx;
    ScaleStabilizer _stabilizer;
    float _start_offset;

public:
    /**
     * @brief Functions for the load cell.
     */
    ScaleController();

    static constexpr float DEFAULT_SCALE = -396.06430054f;
    /**
     * @brief Connect the pins, sets scale and tare (offset).
     * @param dout_pin Data output pin
     * @param sck_pin Clock input pin
     * @param alpha Sensitivity. 0.1: low. 0.5: high.
     * @param scale Scale of laod cell found from calibration.
     * @return true if scale is ready, otherwise false.
     */
    bool begin(uint8_t dout_pin = 6, uint8_t sck_pin = 5, float alpha = 0.3f, float scale = DEFAULT_SCALE);

    /**
     * @brief Set the OFFSET value for tare weight.
     * @param n How many times to read the tare value
     */
    void tare(uint8_t n = 10);

    /**
     * @brief Get the stable scale value
     * @param n How many times to read the (raw) scale value
     * @return Stablized value for the reading
     */
    float get_stable_reading(uint8_t n = 1);

    /**
     * @brief Get the raw scale value
     * @param n How many times to read the scale value
     * @return The average of n raw scale values.
     */
    float get_raw_reading(uint8_t n = 1);

    /**
     * @brief Resets the scale to _start_offset.
     */
    void reset_to_start_offset();
};