/*
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP started");
}

void loop() {
  Serial.println("Hello from esp");
  delay(1000);
}
*/


/*
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>


#define LED_PIN 21
#define NUM_LEDS 1

Adafruit_NeoPixel led(NUM_LEDS, LED_PIN, NEO_RGB + NEO_KHZ800);

void setup()
{
  Serial.begin(115200);

  // Initialize LED library
  led.begin();
  // Set brightness
  led.setBrightness(255);
  // Set all colors to off
  led.clear();
  //set new color
  led.show();
}

void loop()
{
  // Turn on LED
  led.setPixelColor(0, led.Color(255, 0, 0));
  led.show();

  delay(1e3);

  // Turn off
  led.clear();
  led.show();
  delay(1e3);
}
*/




#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

#define LED_PIN 21
#define NUM_LEDS 1

Adafruit_NeoPixel led(NUM_LEDS, LED_PIN, NEO_RGB + NEO_KHZ800);

// Animation setting
const unsigned long CYCLE_TIME = 500;  // 3 seconds
const float MAX_BRIGHTNESS = 0.35f;
const float PHASE_OFFSET = (2.0f * PI) / 3.0f;

// Smooth breathing
float breathe(float phase)
{
  float wave = 0.5f - 0.5f * cosf(phase);

  return powf(wave, 1.5f);
}

void setup()
{
    led.begin();
    led.clear();
    led.show();
}

void loop()
{

  // Current position in the animation cycle
  float progress = (millis() % CYCLE_TIME)
                   / (float)CYCLE_TIME;

  float phase = 2.0f * PI * progress;

  // RGB channels shifted by 2pi/3
  float red   = breathe(phase);
  float green = breathe(phase - PHASE_OFFSET);
  float blue  = breathe(phase - 2.0f * PHASE_OFFSET);

  // Convert to RGB values (0-255)
  uint8_t r = 255.0f * MAX_BRIGHTNESS * red;
  uint8_t g = 255.0f * MAX_BRIGHTNESS * green;
  uint8_t b = 255.0f * MAX_BRIGHTNESS * blue;

  led.setPixelColor(0, led.Color(r, g, b));
  led.show();

  delay(10);


}