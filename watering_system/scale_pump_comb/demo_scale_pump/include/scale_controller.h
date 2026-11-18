#pragma once

#include <Arduino.h>
#include <HX711.h>

class ScaleController {
private:
    HX711 _hx;
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
     * 
     * @param scale Scale of laod cell found from calibration.
     */
    void begin(float scale = DEFAULT_SCALE);

    /**
     * @brief Reads input from Serial.
     * 
     * @return String of Serial input.
     */
    String readInput();

    /**
     * @brief Use the pre-defined HX711 class.
     */
    HX711& hx() {
        return _hx;
    }

};