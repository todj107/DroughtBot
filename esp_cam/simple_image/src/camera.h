#pragma once

namespace camera {
// Initialises the OV5640 and applies the default sensor settings.
// Returns false (and logs why) if the camera could not be started.
bool begin();
}  // namespace camera
