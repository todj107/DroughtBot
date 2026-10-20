#include <Arduino.h>
#include <HX711.h>

// HX711 circuit wiring
const int DOUT_PIN = 6;
const int SCK_PIN = 5;

// Predifined scale
const float scale = -396.06430054;

HX711 hx;

String readInput();

void setup() {
    Serial.begin(115200);
    hx.begin(DOUT_PIN, SCK_PIN);

    hx.set_scale(scale);
    delay(3000);
}

void loop() {
    Serial.print("Enter ml water to be added: ");
    float water = readInput().toFloat();

    float value;
    hx.tare(20);
    do {
        delay(10);
        value = hx.get_units(5);
        Serial.println(value);
    } while (value < water);
    Serial.println("-----------------\n     STOP    \n---------------");
    
    Serial.print("\nStop value: ");
    Serial.println(value);

    Serial.print("Avg after: ");
    Serial.println(hx.get_units(20));
    Serial.println();
}

/**
 * @brief Reads input from Serial.
 * 
 * @return String of Serial input.
 */
String readInput() {
    while (true) {
        if (Serial.available()) {
            String input = Serial.readString();
            input.trim();

            return input;
        }
        delay(100);
    }
}