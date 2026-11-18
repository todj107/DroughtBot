#pragma once

#include <stddef.h>
#include <stdint.h>

bool openCommunication();
bool sendData(const uint8_t* data, size_t length);
bool closeCommunication();
uint8_t getNodeId();

typedef void (*RequestHandler)(uint32_t requestId, const char* command, const uint8_t* params);

void setRequestHandler(RequestHandler handler);
void handleRequest();
bool sendOk(uint32_t requestId);
bool sendResponseValue(uint32_t requestId, uint32_t value);
bool sendError(uint32_t requestId, uint8_t code);
bool sendResponseData(uint32_t requestId, const uint8_t* data, size_t length);


