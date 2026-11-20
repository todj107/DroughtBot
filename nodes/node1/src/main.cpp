#include <Arduino.h>

#include "scale_controller.h"
#include "pump_controller.h"
#include "node.h"
#include "QR_scanner.h"

// Pins
#define PUMP_PIN (uint8_t)2
#define SCALE_PIN_DOUT (uint8_t)6
#define SCALE_PIN_SCK (uint8_t)5
#define QR_RX_PIN (uint8_t)4
#define QR_TX_PIN (uint8_t)5

// Error codes
// CHANGE ERROR CODES TO CORRECT ONE
#define INCORRECT_PARAMS 0
#define QR_SCANNER_ERROR 0
#define UNKNOWN_COMMAND 0

ScaleController scale;
PumpController pump(PUMP_PIN);
qrScanner qr;

// Functions
void commandHandler(uint32_t requestId, const char* command, const uint8_t* params);
float waterProcess(float target_weight);

void setup() {
    Serial.begin(115200);
    delay(3000);

    // Start qr-scanner
    qr.begin(QR_RX_PIN, QR_TX_PIN);
    // Start pump and scale
    if(!scale.begin(SCALE_PIN_DOUT, SCALE_PIN_SCK, 0.3f)) {
        Serial.println("Error: Initilizing Scale failed.");
        delay(5000);
        return;
    }
    if(!pump.begin()) {
        Serial.println("Error: Initilizing Pump failed.");
        delay(5000);
        return;
    }
    // Start communication with hub
    if (!openCommunication()) {
        Serial.println("Error: Initilizing communication failed.");
        // What to do if it fails?
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
    if (strcmp(command, "read_qr_code")) {
        Serial.println("Read QR-Code");                 // DEBUGG (REMOVE LATER)

        // Make scanner ready
        qr.start_scan();

        // Take scan measurements
        char qr_string[QR_BUFFER_SIZE];
        if (!qr.get_qrcode(qr_string)) {
            sendError(requestId, QR_SCANNER_ERROR);
        }

        if (!sendResponseString(requestId, qr_string)) {
            Serial.println("Error: Response string failed to send.");
        }
    }

    else if (strcmp(command, "read_scale") == 0) {
        Serial.println("Read Scale");                   // DEBUGG (REMOVE LATER)

        scale.reset_to_start_offset();
        delay(500);
        float reading = scale.get_raw_reading(10);
        // OBS: sendResponseValue expect uint32_t, however; reading is a float!
        if (!sendResponseValue(requestId, reading)) {
            Serial.println("Error: Response value failed to send.");
        }
    }

    else if(strcmp(command, "water_process") == 0) {
        Serial.println("Start Watering process");       // DEBUGG (REMOVE LATER)

        float target_weight = params[0];
        if (target_weight <= 0){
            Serial.println("Error: Target weight sent by hub is <= 0");
            sendError(requestId, INCORRECT_PARAMS);
            return;
        }
        float end_weight = waterProcess(target_weight);

        sendResponseValue(requestId, end_weight);
    }
    else {
        //unknown command
        sendError(requestId, UNKNOWN_COMMAND);
    }
}

/**
 * @brief Controlls the pump and calls run() to handle scale readings.
 * @param target_weight Amount of water in ml we want to add.
 * @return Stable final-weight. i.e. the accual amount of water added.
 */
float waterProcess(float target_weight) {
    // Zero the scale
    scale.tare();

    // Run the pump on full speed until 2 grams of goal weight.
    pump.runFullSpeed();
    run(target_weight - 2.0f);

    // 2 grams away from goal weight. Run the pump on slow mode.
    pump.runSlowSpeed();
    float stop_value = run(target_weight);

    // Stop the pump
    pump.stop();

    float final_value = scale.get_stable_reading(20);
    // Debugging
    Serial.print("\nStop value: ");
    Serial.println(final_value);
    Serial.print("Avg after: ");
    Serial.println(final_value);

    return final_value;
}

/**
 * @brief Pumps water until stop weight is reached.
 * @param stop Weight in grams.
 */
float run(float stop) {
    float value;
    do {
        delay(10);
        value = scale.get_stable_reading();
        Serial.println(value); // Remove after testing
    } while (value < stop);

    return value;
}
