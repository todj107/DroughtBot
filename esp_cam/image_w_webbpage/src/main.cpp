#include <Arduino.h>
#include <WiFi.h>
#include "app_config.h"
#include "camera.h"
#include "web_server.h"
#include "wifi_link.h"

/**
 * @brief Something is wrong that a retry might fix 
 * (bad ribbon seating, WiFi down...).
 * @param what Description of what went wrong.
 * @note [[noretrun]] specifies that the program will never leave the function.
 */
[[noreturn]] void restartAfterError(const char* what) {
  Serial.printf("[main] %s - restarting in 5 s\n", what);
  delay(5000);
  ESP.restart();
  while (true) {}
}

void take_picture(){
  camera_fb_t* fb = esp_camera_fb_get();
  if (fb != nullptr) {
    web_server::publish(fb->buf, fb->len);  // show it on the web page
    esp_camera_fb_return(fb);
  }
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("\nESP32-S3 OV5640 camera server");

  if (!camera::begin()) {
    restartAfterError("Camera init failed");
  }
  if (!wifi_link::connect()) {
    restartAfterError("WiFi connection failed");
  }
  if (!web_server::begin()) {
    restartAfterError("Web server failed to start");
  }

  const String ip = WiFi.localIP().toString();
  Serial.printf("Open  http://%s/  in your browser\n", ip.c_str());
  // Serial.printf("Stream: http://%s:%u/stream\n", ip.c_str(), app_config::kStreamPort);

  // Discard first image
  camera_fb_t* fb = esp_camera_fb_get();
  if (fb){
    esp_camera_fb_return(fb);
  }
}

void loop() {
  // All work happens in the HTTP server tasks.
  take_picture();
  delay(10000);
}
