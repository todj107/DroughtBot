#pragma once

#include <esp_camera.h>

// Tunable settings.
namespace app_config {

// Camera
constexpr int          kXclkHz            = 20'000'000;
constexpr framesize_t  kMaxFrameSize      = FRAMESIZE_UXGA;   // Sizes the frame buffers
constexpr framesize_t  kInitialFrameSize  = FRAMESIZE_SVGA;   // What you see on first load
constexpr int          kJpegQuality       = 12;               // 0-63, lower = better quality
constexpr int          kFrameBuffers      = 2;                // Needs PSRAM (Which it has)

// Network
constexpr uint32_t     kWifiTimeoutMs     = 20'000;
constexpr uint16_t     kHttpPort          = 80;
constexpr uint16_t     kStreamPort        = 81;

}