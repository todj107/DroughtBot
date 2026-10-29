#pragma once

// Pin map for the Waveshare ESP32-S3-CAM-OV5640.
namespace camera_pins {

constexpr int PWDN  = -1;   // not connected
constexpr int RESET = -1;   // not connected
constexpr int XCLK  = 38;

// SCCB (I2C-style control bus)
constexpr int SIOD  = 8;
constexpr int SIOC  = 7;

// Parallel data bus (Y2..Y9 = D0..D7)
constexpr int Y2    = 45;
constexpr int Y3    = 47;
constexpr int Y4    = 48;
constexpr int Y5    = 46;
constexpr int Y6    = 42;
constexpr int Y7    = 40;
constexpr int Y8    = 39;
constexpr int Y9    = 21;

constexpr int VSYNC = 17;
constexpr int HREF  = 18;
constexpr int PCLK  = 41;

}  // namespace camera_pins
