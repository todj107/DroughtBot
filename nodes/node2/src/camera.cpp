#include "camera.h"

#include <Arduino.h>
#include <esp_camera.h>

#include "app_config.h"
#include "camera_pins.h"

namespace camera {

namespace {

camera_config_t makeConfig() {
  camera_config_t config = {};

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0 = camera_pins::Y2;
  config.pin_d1 = camera_pins::Y3;
  config.pin_d2 = camera_pins::Y4;
  config.pin_d3 = camera_pins::Y5;
  config.pin_d4 = camera_pins::Y6;
  config.pin_d5 = camera_pins::Y7;
  config.pin_d6 = camera_pins::Y8;
  config.pin_d7 = camera_pins::Y9;

  config.pin_xclk     = camera_pins::XCLK;
  config.pin_pclk     = camera_pins::PCLK;
  config.pin_vsync    = camera_pins::VSYNC;
  config.pin_href     = camera_pins::HREF;
  config.pin_sccb_sda = camera_pins::SIOD;
  config.pin_sccb_scl = camera_pins::SIOC;
  config.pin_pwdn     = camera_pins::PWDN;
  config.pin_reset    = camera_pins::RESET;

  config.xclk_freq_hz = app_config::kXclkHz;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode    = CAMERA_GRAB_LATEST;

  config.frame_size   = app_config::kFrameSize;
  config.jpeg_quality = app_config::kJpegQuality;
  config.fb_count     = app_config::kFrameBuffers;
  config.fb_location  = CAMERA_FB_IN_PSRAM;

  return config;
}

void applySensorDefaults() {
  sensor_t* s = esp_camera_sensor_get();
  if (s == nullptr) return;

  Serial.printf("[camera] sensor PID: 0x%04x\n", s->id.PID);
  if (s->id.PID != OV5640_PID) {
    Serial.println("[camera] warning: sensor is not an OV5640");
  }
  
  // Add per-sensor tweaks here, for example:
  // s->set_vflip(s, 1);
  // s->set_hmirror(s, 1);
  // s->set_brightness(s, 0);
  // s->set_saturation(s, 0);
}

}  // namespace

bool begin() {
  const camera_config_t config = makeConfig();

  const esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[camera] init failed: 0x%x\n", err);
    return false;
  }

  applySensorDefaults();
  Serial.println("[camera] ready");
  return true;
}

}  // namespace camera
