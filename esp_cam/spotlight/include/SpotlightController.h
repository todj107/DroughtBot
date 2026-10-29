#pragma once
#include <Arduino.h>

/**
 * @class SpotlightController
 * @brief Handles the spotlight.
 */
class SpotlightController {
private:
    uint8_t _brightness;
    uint8_t _EN_PIN;
public:
    SpotlightController() = default;

    // Destructor (Turns off the spotlight)
    ~SpotlightController();

    static constexpr uint8_t DEFAULT_BRIGHTNESS = (uint8_t)60;
    /**
     * @brief Connect the pin and initilizes the spotlight to zero.
     * @param EN_PIN Logic enable pin number.
     */
    void begin(uint8_t EN_PIN);

    /**
     * @brief Changes the brightness used in on();.
     * @param percent Percentage of maximum brightness.
     * @note on(); needs to be called afterwards to make the change take effect.
     */
    void change_brightness(uint8_t percent);

    /**
     * @brief Turns on the spothlight.
     * If brightness is not changed it uses the defualt settings.
     */
    void on();

    /**
     * @brief Sets the brightness to zero, i.e. turns off the spotlight.
     */
    void off();
};