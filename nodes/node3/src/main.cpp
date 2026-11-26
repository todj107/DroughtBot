#include <Arduino.h>
#include "esp_system.h"
#include "espnow_protocol.h"
#include "node.h"
#include "Buttons.h"
#include "Display.h"


// INTERNAL BTNS
#define BTN1 0
#define BTN2 1
#define BTN3 2

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

    if (!espnowBegin()){
        Serial.println("Error: Espnow radio initilizing failed. Restart in 5 sec");
        delay(5000);
        esp_restart();
    }
    
    if (!openCommunication()){
        Serial.println("Error: Initilizing communication failed.");
        delay(5000);
        esp_restart();
    }

    // Register nodes own handler function
    setRequestHandler(commandHandler);
}

void loop() {    
    if (btn.pressed(BTN1)) {
        // Do something
        sendButton(EspNowProtocol::ButtonId::Start);
    }
    if (btn.pressed(BTN2)) {
        // Do something
        sendButton(EspNowProtocol::ButtonId::Series);
    }
    if (btn.pressed(BTN3)) {
        // Do something 
        sendButton(EspNowProtocol::ButtonId::Abort);
    }

    handleRequest();
    delay(10);
}

void commandHandler(uint32_t requestId, const char* command, const uint8_t* params) {
    if (strcmp(command, "abort") == 0) {
        sendOk(requestId);
        // Abort work (does nothing right now)
        Serial.println("Work aborted");
        return;
    }

    if (strcmp(command, "restart_esp") == 0) {
        sendOk(requestId);
        delay(200);
        esp_restart();
    }

    if (!display_is_working) {
        sendError(requestId, DISPLAY_INIT_FAILED);
        return;
    }

    if (strcmp(command, "display") == 0) {
        /**
         * Structrue: params[0]: Status code (e.g. error), params[1]: message code.
         * These are coupled to the codes defined in espnow_protocol.
         */
        if (!params[0]) {
            sendError(requestId, INCORRECT_PARAMS);
            return;
        }
        sendOk(requestId);

        if (params[0] == STATUS_ERROR) {
            // Display header and message
            disp.set_header("Error");
            disp.set_message(DisplayMessages::error(params[1]));
        }

        else if (params[0] == STATUS_READY) {
            disp.set_header("Ready");
            disp.set_message(DisplayMessages::ready(params[1]));
        }

        else if (params[0] == STATUS_RUNNING) {
            disp.set_header("Running");
            disp.set_message(DisplayMessages::running(params[1]));
        }

        return;
    }

    else if (strcmp(command, "series_info") == 0) {
        sendOk(requestId);

        disp.set_header("Series info");
        char message[100];
        snprintf(message, sizeof(message), "Series %u has %u plants. Press OK to start\n", params[0], params[1]);
        disp.set_message(message);

        return;
    }

    else {
        //unknown command
        sendError(requestId, UNKNOWN_COMMAND);
        return;
    }
}