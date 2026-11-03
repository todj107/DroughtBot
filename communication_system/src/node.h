#pragma once

#include <stddef.h>
#include <stdint.h>

bool openCommunication();
bool sendData(const uint8_t* data, size_t length);
bool closeCommunication();
