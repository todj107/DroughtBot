#include <Arduino.h>
#include "esp_system.h"
#include "scale_controller.h"
#include "pump_controller.h"
#include "node.h"
#include "QR_scanner.h"
#include "espnow_protocol.h"

// Pins
#define PUMP_PIN (uint8_t)2
#define SCALE_PIN_DOUT (uint8_t)6
#define SCALE_PIN_SCK (uint8_t)5
#define QR_RX_PIN (uint8_t)4
#define QR_TX_PIN (uint8_t)5

// Internal error codes
#define PROCESS_ABORTED -10

ScaleController scale;
PumpController pump(PUMP_PIN);
qrScanner qr;

static volatile bool abortRequested = false;
static bool busy = false;

// Functions
void commandHandler(uint32_t requestId, const char* command, const uint8_t* params);
float waterProcess(float target_weight);
float run(float stop);

void setup() {
    Serial.begin(115200);
    delay(3000);

    // Start qr-scanner
    qr.begin(QR_RX_PIN, QR_TX_PIN);
    // Start pump and scale
    if(!scale.begin(SCALE_PIN_DOUT, SCALE_PIN_SCK, 0.3f)) {
        Serial.println("Error: Initilizing Scale failed.");
    }
    if(!pump.begin()) {
        Serial.println("Error: Initializing Pump failed.");
    }
    if (!espnowBegin()) {
        Serial.println("Error: Initializing espnow radio failed.");
        delay(5000);
        esp_restart();
    }
    // Start communication with hub
    if (!openCommunication()) {
        Serial.println("Error: Initializing communication failed.");
        delay(5000);
        esp_restart();
    }

    // Register nodes own handler function
    setRequestHandler(commandHandler);
}


void loop() {
    handleRequest();
    delay(10);
}

/**
 * @brief When a request is registered, the following structure should be followed:
 * 1) Decode commande to a known task,
 * 2) Send confirmation to hub that task have been received,
 * 3) Preform the task,
 * 4) Send task data to the hub.
 * 
 * @param requestId The id of the request. Used by hub to seperate different tasks.
 * @param command Contains the task that should be preformed.
 * @param params Structure: [uint8_t, uint8_t]. Can contain data needed.
 */
void commandHandler(uint32_t requestId, const char* command, const uint8_t* params) {
    Serial.println("Request arrived");                  // DEBUGG (REMOVE LATER)

    if (strcmp(command, "abort") == 0) {
        sendOk(requestId);
        // Stop the pump right away
        if (busy) {
            abortRequested = true;
            pump.stop();     
        }         
        return;
    }
    if (busy) { 
        // Reject other commands while pumping                 
        sendError(requestId, NODE_BUSY);
        return;
    }
    if (strcmp(command, "restart_esp") == 0) {
        sendOk(requestId);
        delay(200);
        esp_restart();
    }

    // Check that pump and scale works
    if (!pump.isReady()) {
        sendError(requestId, PUMP_NOT_READY);
        return;
    }
    if (!scale.is_ready()) {
        sendError(requestId, SCALE_NOT_READY);
        return;
    }

    // Compare command
    if (strcmp(command, "read_qr_code") == 0) {
        sendOk(requestId);
        Serial.println("Read QR-Code");                 // DEBUGG (REMOVE LATER)

        // Make scanner ready
        qr.start_scan();

        // Take scan measurements
        char qr_string[QR_BUFFER_SIZE];
        if (!qr.get_qrcode(qr_string)) {
            sendError(requestId, QR_SCANNER_ERROR);
            return;
        }

        if (!sendResponseString(requestId, qr_string)) {
            Serial.println("Error: Response string failed to send.");
        }
    }

    else if (strcmp(command, "read_scale") == 0) {
        sendOk(requestId);
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
        sendOk(requestId);
        Serial.println("Start Watering process");       // DEBUGG (REMOVE LATER)

        float target_weight = params[0];
        if (target_weight <= 0){
            Serial.println("Error: Target weight sent by hub is <= 0");
            sendError(requestId, INCORRECT_PARAMS);
            return;
        }

        busy = true;
        float end_weight = waterProcess(target_weight);
        if (end_weight == PROCESS_ABORTED) {
            sendError(requestId, ABORTED_PROCESS);
            return;
        }
        busy = false;

        if (!sendResponseValue(requestId, end_weight)) {
            Serial.println("Error: Response value failed to send.");
        }
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
 * PROCESS_ABORTED if an abort-request was registered during the process.
 */
float waterProcess(float target_weight) {
    // Zero the scale
    scale.tare();

    // Run the pump on full speed until 2 grams of goal weight.
    pump.runFullSpeed();
    if (run(target_weight - 2.0f) == PROCESS_ABORTED) {
        return PROCESS_ABORTED;
    }

    // 2 grams away from goal weight. Run the pump on slow mode.
    pump.runSlowSpeed();
    float stop_value = run(target_weight);
    if (stop_value == PROCESS_ABORTED) {
        return PROCESS_ABORTED;
    }

    // Stop the pump
    pump.stop();

    float final_value = scale.get_stable_reading(20);
    // Debugging
    Serial.print("\nStop value: ");
    Serial.println(stop_value);
    Serial.print("Avg after: ");
    Serial.println(final_value);

    return final_value;
}

/**
 * @brief Pumps water until stop weight is reached.
 * @param stop Weight in grams.
 * @return The end weight on success, 
 * PROCESS_ABORTED if an abort-request was registered during the process.
 */
float run(float stop) {
    float value;
    do {
        // delay(10);
        handleRequest();
        if (abortRequested){
            abortRequested = false;
            return PROCESS_ABORTED;
        }
        
        value = scale.get_stable_reading();
        Serial.println(value);                  // Remove after testing
    } while (value < stop);

    return value;
}
