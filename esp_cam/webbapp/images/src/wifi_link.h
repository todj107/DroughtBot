#pragma once

#include <Arduino.h>

namespace wifi_link {

// Connects in station mode using the credentials in include/secrets.h.
// Returns true once an IP address has been obtained.
bool connect();

}  // namespace wifi_link
