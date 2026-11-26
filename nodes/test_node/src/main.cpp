#include <Arduino.h>
#include "node.h"
#include "espnow_protocol.h"
#include "test_image.h"

// Internal error codes
#define INTERNAL_PROCESS_ABORTED -10

static volatile bool abortRequested = false;
static bool busy = false;
const uint8_t button_start = 1;

// Functions
void commandHandler(uint32_t requestId, const char* command, const uint8_t* params);
float waterProcessSimulation(float target_weight);
bool sendImageFile(uint32_t requestId, const char* path);

void setup() {
    Serial.begin(115200);
    delay(3000);

    // Start things
    Serial.println("Things start here");
    delay(1000);

    if (!espnowBegin()){
        Serial.println("Error: Espnow radio initilizing failed. Restart in 5 sec");
        delay(5000);
        esp_restart();
    }

    // Start communication with hub
    if (!openCommunication()) {
        Serial.println("Error: Initilizing communication failed. Restart in 5 sec");
        delay(5000);
        esp_restart();
    }

    // Register nodes own handler function
    setRequestHandler(commandHandler);

    Serial.println("Simulate button press in 5s");
    delay(5000);
    // sendButton(EspNowProtocol::ButtonId::Start);
    sendButton(EspNowProtocol::ButtonId::Series);
}

void loop() {
    handleRequest();
    delay(10);
}


void commandHandler(uint32_t requestId, const char* command, const uint8_t* params) {
    Serial.printf("[Request] %s\n", command);
    if (strcmp(command, "abort") == 0) {
        sendOk(requestId);

        // Abort work
        abortRequested = true;
        Serial.println("Work aborted");

        return;
    }

    if (strcmp(command, "display") == 0) {
        Serial.println("[Display]");
        if (!params[0]) {
            Serial.println("Invalid params");
            sendError(requestId, INCORRECT_PARAMS);
            return;
        }
        sendOk(requestId);

        if (params[0] == STATUS_ERROR) {
            // Display header and message
            Serial.println("Error");
            Serial.println(DisplayMessages::error(params[1]));
        }

        else if (params[0] == STATUS_READY) {
            Serial.println("Ready");
            Serial.println(DisplayMessages::ready(params[1]));

            delay(2000);
            sendButton(EspNowProtocol::ButtonId::Start);
        }

        else if (params[0] == STATUS_RUNNING) {
            Serial.println("Running");
            Serial.println(DisplayMessages::running(params[1]));
        }

        return;
    }

    if (strcmp(command, "series_info") == 0) {
        sendOk(requestId);
        Serial.printf("Series %u has %u plants. Press OK to start\n", params[0], params[1]);
 
        // Simulate OK press
        delay(2000);
        sendButton(EspNowProtocol::ButtonId::Start);
        return;
    }

    else if (busy) { 
        // Reject other commands while pumping
        sendError(requestId, NODE_BUSY);
        Serial.println("Node is busy");
        return;
    }

    if (strcmp(command, "restart_esp") == 0) {
        sendOk(requestId);
        Serial.println("Restart esp in 2 seconds");
        delay(2000);
        esp_restart();
    }

    // Compare command
    if (strcmp(command, "read_qr_code") == 0) {
        sendOk(requestId);
        Serial.println("Read QR-Code");

        // Simulate qr-code
        delay(500);
        // New code every time, so each plant gets its own
        static unsigned qrCount = 0;
        char qr_string[32] = "this_is_a_qrcode_string";
        if (++qrCount > 1) {
            snprintf(qr_string, sizeof(qr_string), "test_qr_%u", qrCount);
        }

        if (!sendResponseString(requestId, qr_string)) {
            Serial.println("Error: Response string failed to send.");
        }
    }

    else if (strcmp(command, "read_scale") == 0) {
        sendOk(requestId);
        Serial.println("Read Scale");

        // Simulate scale reading
        delay(500);
        uint32_t reading = 23;
        if (!sendResponseValue(requestId, reading)) {
            Serial.println("Error: Response value failed to send.");
        }
    }

    else if(strcmp(command, "water_process") == 0) {
        sendOk(requestId);
        Serial.println("Start Watering process");

        float target_weight = params[0];
        if (target_weight <= 0){
            Serial.println("Error: Target weight sent by hub is <= 0");
            sendError(requestId, INCORRECT_PARAMS);
            return;
        }

        busy = true;
        abortRequested = false;
        float end_weight = waterProcessSimulation(target_weight);
        busy = false;
        if (end_weight == INTERNAL_PROCESS_ABORTED) {
            sendError(requestId, ABORTED_PROCESS);
            return;
        }
        

        if (!sendResponseValue(requestId, end_weight)) {
            Serial.println("Error: Response value failed to send.");
        }
    }

    else if (strcmp(command, "send_image") == 0) {
        sendOk(requestId);
        busy = true;
        
        Serial.println("Take and send image!");

        delay(500);
        Serial.println("Take image");
        delay(500);
        
        // Send the image
        Serial.println("Send image");
        if (!sendData(test_image_jpeg, test_image_jpeg_len)) {
            Serial.println("Error: Response value failed to send.");
        }
        busy = false;
    }

    else {
        //unknown command
        sendError(requestId, UNKNOWN_COMMAND);
    }
}

float waterProcessSimulation(float target_weight) {
    Serial.printf("Target weight: %f\n", target_weight);
    unsigned long now = millis();
    while (millis() - now < 10000) {
        handleRequest();
        if (abortRequested) {
            abortRequested = false;
            return INTERNAL_PROCESS_ABORTED;
        }
        
        delay(500);
    }

    float water_added = 20;
    return water_added;
}
