#pragma once

#include <stddef.h>
#include <stdint.h>

namespace web_server {

// Start the HTTP server (one server, one port).
bool begin();

// Hand a JPEG to the web page. The data is copied, so you can return the
// camera frame buffer right after calling this.
void publish(const uint8_t* jpeg, size_t len);

}  // namespace web_server