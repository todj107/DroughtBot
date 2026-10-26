#pragma once

#include <Arduino.h>
#include <HX711.h>
#include "ScaleStabilizer.h"

class ScaleController {
private:
    HX711 _hx;
    ScaleStabilizer _stabilizer;
    uint8_t _dout_pin;
    uint8_t _sck_pin;

public:
    /**
     * @brief Functions for the load cell.
     * 
     * @param dout_pin Data output pin
     * @param sck_pin Clock input pin
     */
    ScaleController(uint8_t dout_pin = 6, uint8_t sck_pin = 5);

    static constexpr float DEFAULT_SCALE = -396.06430054f;
    /**
     * @brief Connect the pins, sets scale and tare (offset).
     * @param window_size Size of the moving window
     * @param weightThreshold Weight/noise threshold - e.g. 0.1g
     * @param scale Scale of laod cell found from calibration.
     */
    void begin(int window_size = 10, float weight_threshold = 0.05, float scale = DEFAULT_SCALE);

    /**
     * @brief Set the OFFSET value for tare weight.
     * @param n How many times to read the tare value
     */
    void tare(uint8_t n = 10);

    /**
     * 
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
     * @brief Reads input from Serial.
     * 
     * @return String of Serial input.
     */
    String readInput();
};