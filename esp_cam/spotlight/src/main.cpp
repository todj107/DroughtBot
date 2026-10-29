#include <Arduino.h>
#include "SpotlightController.h"

// Chnage the pin to match the setup
constexpr uint8_t EN_PIN = 4;

void setup() {
    Serial.begin(115200);

    SpotlightController led;
    led.begin(EN_PIN);
    delay(3000);

    // Spotlight on
    Serial.println("Spotlight on!");
    led.on();
    delay(2000);

    // Spotlight off
    Serial.println("Spotlight off!");
    led.off();
    delay(2000);

    // Change spotlight brightness
    Serial.println("Change Spotlight brightness.");
    for (uint8_t i = 25; i <= 100; i+=25) {
        Serial.printf("Brightness: %d%\n", i);
        led.change_brightness(i);
        led.on();
        delay(2000);
    }
} // ~SpotlightController calls here

void loop() {
    delay(10000);
}