#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

enum StatusCode : uint8_t {
    STATUS_ERROR = 1,
    STATUS_RUNNING = 2,
    STATUS_READY = 3,
};

enum ErrorCode : uint8_t {
    QR_SCANNER_ERROR = 1,
    PUMP_NOT_READY = 2,
    SCALE_NOT_READY = 3,
    NODE_BUSY = 4,
    ABORTED_PROCESS = 5,
    CAMERA_INIT_FAILED = 6,
    UNKNOWN_COMMAND = 7,
    INCORRECT_PARAMS = 8,
    DISPLAY_INIT_FAILED = 9,
    IMAGE_CAPTURE_FAILED = 10,
}; // enum ErrorCode

enum ReadyCode : uint8_t {
    START_PROCESS = 1,
    START_WATERING = 2,
    START_IMAGE = 3,
    NEXT_PLANT = 4,
    SERIES_DONE = 5,
    RETRY_QR = 6,
}; 

enum RunningCode : uint8_t {
    RUNNING_WATERING = 1,
    RUNNING_QR_SCALE = 2,
    RUNNING_IMAGE = 3,
};

namespace DisplayMessages {
// Error code messages
inline char *error(uint8_t ERROR_CODE) {
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

    case IMAGE_CAPTURE_FAILED:
        return "ERROR: Failed to capture an image";

    case DISPLAY_INIT_FAILED:
        return "ERROR: Display initialization failed";

    case UNKNOWN_COMMAND:
        return "WARNING: Unknown command";

    case INCORRECT_PARAMS:
        return "ERROR: Incorrect params recieved";

    default:
        return "Invalid ERROR_CODE.";
    }
}

inline char *ready(uint8_t READY_CODE) {
    switch (READY_CODE) {
    case START_PROCESS:
        return "Place the plant on the scale and press OK when ready";
 
    case START_WATERING:
        return "Put the pump-handle inside the soil and press OK when ready";
 
    case START_IMAGE:
        return "Remove the pump-handle and press OK when ready to take image of the plant";
 
    case NEXT_PLANT:
        return "QR-code saved. Show the next plant and press OK";
 
    case SERIES_DONE:
        return "All plants in the series are registered";
 
    case RETRY_QR:
        return "QR-code not saved. Check the plant and press OK to retry";
 
    default:
        return "Invalid ready code";
    }
}

inline char *running(uint8_t RUNNING_CODE) {
    switch (RUNNING_CODE) {
    case RUNNING_WATERING:
        return "Watering process";
    
    case RUNNING_QR_SCALE:
        return "Reading QR-code and taking initial weight";

    case RUNNING_IMAGE:
        return "Taking and sending image";

    default:
        return "Invalid running code";
    }
}
}// namespace DisplayMessages

namespace EspNowProtocol {

constexpr uint8_t kChannel = 1;
constexpr size_t kRequestParamBytes = 2;
constexpr uint8_t kBroadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

enum class MessageType : uint8_t {
    Discover = 1,
    DiscoverResponse,
    Handshake,
    Ready,
    Busy,
    Close,
    Closed,
    Data,
    Ack,
    Request = 10,
    RequestResponse = 11,
    NodeAssignment = 12,
    Button = 13,    // [type][body...], needs no session and no request id
};

enum class ButtonId : uint8_t {
    Abort = 1,   // avoid 0 so a zeroed buffer can't look like a press
    Start = 2,
    Series = 3,
};

#pragma pack(push, 1)
struct DataHeader {
    MessageType type;
    uint32_t transferId;
    uint32_t chunkIndex;
    uint32_t chunkCount;
};

struct AckMessage {
    MessageType type;
    uint32_t transferId;
    uint32_t chunkIndex;
};

struct TargetedControlMessage {
    MessageType type;
    uint8_t targetAddress[6];
};

struct RequestMessage {
    MessageType type;
    uint32_t requestId;
    uint8_t commandLength;
    // followed by [command string][kRequestParamBytes bytes of params]
};

struct ResponseMessage {
    MessageType type;
    uint32_t requestId;
    uint8_t responseLength;
    uint8_t response[240];
};

struct NodeAssignmentMessage {
    MessageType type;
    uint8_t nodeId;
};
#pragma pack(pop)

constexpr size_t kMaxChunkSize = ESP_NOW_MAX_DATA_LEN - sizeof(DataHeader);

inline bool addPeer(const uint8_t address[6]) {
    if (esp_now_is_peer_exist(address)) {
        return true;
    }

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, address, sizeof(peer.peer_addr));
    peer.channel = kChannel;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;

    const esp_err_t result = esp_now_add_peer(&peer);
    return result == ESP_OK || result == ESP_ERR_ESPNOW_EXIST;
}

inline bool initializeRadio() {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);

    if (esp_wifi_set_channel(kChannel, WIFI_SECOND_CHAN_NONE) != ESP_OK ||
        esp_now_init() != ESP_OK) {
        return false;
    }

    return addPeer(kBroadcastAddress);
}

inline bool sameAddress(const uint8_t first[6], const uint8_t second[6]) {
    return memcmp(first, second, 6) == 0;
}

}  // namespace EspNowProtocol
