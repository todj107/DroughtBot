#include <Arduino.h>

#include "node.h"
#include "Buttons.h"
#include "error_codes.h"
#include "Display.h"

// INTERNAL BTNS
#define BTN1 0
#define BTN2 1
#define BTN3 2

// EXTERNAL ERROR CODES
#define UNKNOWN_COMMAND 255

Buttons btn;
uint8_t button_pins[3] = {38, 39, 40};
Display disp;

bool display_is_working = true;

// Functions
void commandHandler(uint32_t requestId, const char* command, const uint8_t* params);

void setup() {
    Serial.begin(115200);
    delay(3000);

    btn.begin(button_pins);

    if (!disp.begin()) {
        Serial.println("Display initialization failed");
        display_is_working = false;
    }
    
    if (!openCommunication()){
        Serial.println("Error: Initilizing communication failed.");
        delay(5000);
        return;
    }

    // Register nodes own handler function
    setRequestHandler(commandHandler);
}

void loop() {    
    if (btn.pressed(BTN1)) {
        // Do something
    }
    if (btn.pressed(BTN2)) {
        // Do something
    }
    if (btn.pressed(BTN3)) {
        // Do something
    }

    handleRequest();
}

void commandHandler(uint32_t requestId, const char* command, const uint8_t* params) {
    Serial.println("Request arrived");                  // DEBUGG (REMOVE LATER)

    if (!display_is_working) {
        sendError(requestId, DISPLAY_INIT_FAILED);
        return;
    }

    if (strcmp(command, "display_error")) {
        /**
         * Structrue 1: params[0]: Error_code, params[1]: nothing
         * Alternative structure: params[0: error_code, params[1]: which node.
         */
        if (!params[0]) {
            sendError(requestId, INCORRECT_PARAMS);
        }
        sendOk(requestId);

        char *error_message = error_code::get_message(params[0]);

        // Need to create a clear function
    }
}