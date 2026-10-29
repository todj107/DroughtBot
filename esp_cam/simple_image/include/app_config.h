#pragma once

#include <esp_camera.h>

// Tunable settings.
namespace app_config {

// Camera
constexpr int          kXclkHz            = 20'000'000;
constexpr framesize_t  kFrameSize         = FRAMESIZE_XGA;   // Sizes the frame buffers
constexpr int          kJpegQuality       = 12;               // 0-63, lower = better quality
constexpr int          kFrameBuffers      = 2;                // Needs PSRAM (Which it has)
}