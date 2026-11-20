#include <Arduino.h>

#include "node.h"
#include "app_config.h"
#include "camera.h"
#include "SpotlightController.h"

#define LIGHT_EN_PIN 43
#define UNKNOWN_COMMAND 255

SpotlightController light;

// Functions
void commandHandler(uint32_t requestId, const char* command, const uint8_t* params);
void warm_up(uint8_t n);

void setup() {
    Serial.begin(115200);
    delay(3000);

    light.begin(LIGHT_EN_PIN);

    if (!camera::begin()) {
        Serial.println("Camera init failed");
        delay(5000);
        return;
    }

    if (!openCommunication()) {
        Serial.println("Initilizing communication failed.");
        delay(5000);
        return;
    }
}

void loop() {
    handleRequest();
    delay(10);
}

void commandHandler(uint32_t requestId, const char* command, const uint8_t* params) {
    Serial.println("Request arrived");                  // DEBUGG (REMOVE LATER)
    if (strcmp(command, "send_image")) {
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
void warm_up(uint8_t n){
    for (uint8_t i = 0; i < n; i++){
        camera_fb_t* fb = esp_camera_fb_get();
        if (fb){
            esp_camera_fb_return(fb);
        }
        delay(100);
    }
}