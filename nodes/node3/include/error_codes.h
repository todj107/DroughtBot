// Define external error codes and map to a message
#pragma once
#include <Arduino.h>

// Node 1
#define QR_SCANNER_ERROR 1
#define PUMP_NOT_READY 2
#define SCALE_NOT_READY 3
#define NODE_BUSY 4
#define ABORTED_PROCESS 5

// Node 2
#define CAMERA_INIT_FAILED 6

// Node 3

// Common
#define UNKNOWN_COMMAND 7
#define INCORRECT_PARAMS 8


namespace error_code {
    char *get_message(uint8_t ERROR_CODE) {
        switch (ERROR_CODE) {
        case QR_SCANNER_ERROR:
            return "ERROR: Unable to obtain QR-code";

        case PUMP_NOT_READY:
            return "ERROR: Pump initialization failed";

        case SCALE_NOT_READY:
            return "ERROR: Scale initialization failed";

        case NODE_BUSY:
            return "WARNING: Node is busy, command ignored.";

        case ABORTED_PROCESS:
            return "WARNING: Process have been aborted.";

        case CAMERA_INIT_FAILED:
            return "ERROR: Camera initialization failed";
        
        case UNKNOWN_COMMAND:
            return "WARNING: Unknown command";

        case INCORRECT_PARAMS:
            return "ERROR: Incorrect params recieved";
        
        default:
            return "Invalid ERROR_CODE.";
        }
    }
} // namespace error_code

