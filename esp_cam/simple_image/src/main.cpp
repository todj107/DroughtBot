#include <Arduino.h>
#include "app_config.h"
#include "camera.h"

/**
 * @brief Something is wrong that a retry might fix.
 * Used for debugging.
 * @param what Description of what went wrong.
 * @note [[noretrun]] specifies that the program will never leave the function.
 */
[[noreturn]] void restart_after_error(const char* what) {
  Serial.printf("[main] %s - restarting in 5 s\n", what);
  delay(5000);
  ESP.restart();
  while (true) {}
}

/**
 * @brief Takes n pictures so the camera camn calibrate.
 * @param n The amount of pictures that should be discarded.
 */
void warm_up(uint8_t n){
  for (uint8_t i = 0; i < n; i++){
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb){
      esp_camera_fb_return(fb);
    }
    delay(100);
  }
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("\nESP32-S3 OV5640 camera server");

  if (!camera::begin()) {
    restart_after_error("Camera init failed");
  }

  // Discard first images
  warm_up((uint8_t)3);

  // Take a picture (with flash?)
  camera_fb_t* fb = esp_camera_fb_get();
  
  // Send the image
  // ADD CODE

  // Wait for received verification
  // ADD CODE

  // Return/ remove picture. (Needs to be done before taking another picture)
  esp_camera_fb_return(fb);

  // // Go to sleep
  // esp_camera_deinit(); // Sleep the camera
  // // Set some wake-up timer
  // esp_deep_sleep_start();  // Sleep the esp. Wake-up is like a reboot.
}

// Unused (if deep_sleep is used)
void loop() {
  // All work happens in the HTTP server tasks.
  delay(10000);
}
