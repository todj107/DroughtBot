#include <Arduino.h>
#include "esp_system.h"


#include "node.h"
#include "app_config.h"
#include "camera.h"
#include "SpotlightController.h"


// PINS
#define LIGHT_EN_PIN 43

// EXTERNAL ERROR CODES
#define UNKNOWN_COMMAND 255
#define CAMERA_INIT_FAILED 0

SpotlightController light;

static bool camera_init_fail = false;

// Functions
void commandHandler(uint32_t requestId, const char* command, const uint8_t* params);
void warm_up(uint8_t n);

void setup() {
    Serial.begin(115200);
    delay(3000);

    light.begin(LIGHT_EN_PIN);

    if (!camera::begin()) {
        Serial.println("Camera init failed");
        camera_init_fail = true;
    }

    if (!openCommunication()) {
        Serial.println("Initilizing communication failed.");
        // Try again in 5s
        delay(5000);
        return;
    }

    // Register nodes own handler function
    setRequestHandler(commandHandler);
}

void loop() {
    handleRequest();
    delay(10);
}

void commandHandler(uint32_t requestId, const char* command, const uint8_t* params) {
    Serial.println("Request arrived");                  // DEBUGG (REMOVE LATER)

    if (camera_init_fail) {
        sendError(requestId, CAMERA_INIT_FAILED);
    }

    if (strcmp(command, "restart_esp") == 0) {
        sendOk(requestId);
        esp_camera_deinit();
        esp_restart();
    }
    else if (strcmp(command, "send_image") == 0) {
        sendOk(requestId);
        
        // Discard first images
        warm_up((uint8_t)3);

        // Flash on
        light.on();
        delay(500);
        // Take a picture
        camera_fb_t* fb = esp_camera_fb_get();
        // Flash off
        delay(500);
        light.off();
        
        // Send the image
        if (!sendResponseData(requestId, fb->buf, fb->len)) {
            Serial.println("Error: Sending image failed.");
            return;
        }

        // Return/remove picture
        // Needs to be done before taking another picture
        esp_camera_fb_return(fb);
    }
    else {
        //unknown command
        sendError(requestId, UNKNOWN_COMMAND);
    }
}

/**
 * @brief Takes n pictures so the camera camn calibrate.
 * @param n The amount of pictures that should be discarded.
 */
void warm_up(uint8_t n) {
    for (uint8_t i = 0; i < n; i++){
        camera_fb_t* fb = esp_camera_fb_get();
        if (fb){
            esp_camera_fb_return(fb);
        }
        delay(100);
    }
}