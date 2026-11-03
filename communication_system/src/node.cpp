#include "espnow_protocol.h"
#include "node.h"

namespace {
using namespace EspNowProtocol;

constexpr uint32_t kResponseTimeoutMs = 1000;
constexpr uint32_t kAckTimeoutMs = 500;
constexpr uint32_t kInitialBackoffWindowMs = 1000;
constexpr uint32_t kMaxBackoffWindowMs = 30000;
constexpr uint8_t kMaxChunkAttempts = 5;

volatile bool hubDiscovered = false;
uint8_t hubAddress[6] = {};
uint8_t nodeAddress[6] = {};
volatile uint8_t controlResponse = 0;
volatile bool ackReceived = false;
volatile uint32_t ackTransferId = 0;
volatile uint32_t ackChunkIndex = 0;
bool radioReady = false;
bool sessionOpen = false;
bool reconnectEnabled = true;
uint32_t nextTransferId = 1;

void onReceive(const esp_now_recv_info_t* senderInfo,
               const uint8_t* data, int length) {
    
    //message comes in
    if (senderInfo == nullptr || senderInfo->src_addr == nullptr ||
        data == nullptr || length < 1) {
        return;
    }

    const MessageType type = static_cast<MessageType>(data[0]);
    
    if (type == MessageType::DiscoverResponse && length == 1) {
        if (addPeer(senderInfo->src_addr)) {
            memcpy(hubAddress, senderInfo->src_addr, sizeof(hubAddress));
            hubDiscovered = true;
        }
        return;
    }

    //ignore replies from anyone but our hub
    if (!hubDiscovered || !sameAddress(senderInfo->src_addr, hubAddress)) {
        return;
    }

    if ((type == MessageType::Busy || type == MessageType::Closed) &&
        length == sizeof(TargetedControlMessage)) {
        TargetedControlMessage message = {};
        memcpy(&message, data, sizeof(message));
        if (sameAddress(message.targetAddress, nodeAddress)) {
            controlResponse = data[0];
        }
        return;
    }

    if ((type == MessageType::Ready || type == MessageType::Busy ||
         type == MessageType::Closed) && length == 1) {
        controlResponse = data[0];
        return;
    }

    if (type == MessageType::Ack && length == sizeof(AckMessage)) {
        AckMessage ack = {};
        memcpy(&ack, data, sizeof(ack));
        //only count an ack for the chunk we are waiting on
        if (ack.transferId == ackTransferId &&
            ack.chunkIndex == ackChunkIndex) {
            ackReceived = true;
        }
    }
}

bool waitForControlResponse() {
    const uint32_t start = millis();
    while (millis() - start < kResponseTimeoutMs && controlResponse == 0) {
        delay(10);
    }
    return controlResponse != 0;
}

uint32_t nextBackoffWindow(uint32_t window) {
    return window >= kMaxBackoffWindowMs / 2
        ? kMaxBackoffWindowMs
        : (window * 2) - 1;
}

void waitBeforeRetry(uint32_t& window) {
    
    //pick a random wait in window
    const uint32_t wait = esp_random() % window;
    Serial.printf("[STATUS] Retrying in %lu ms (window: %lu ms)\n",
                  static_cast<unsigned long>(wait),
                  static_cast<unsigned long>(window));
    delay(wait);
    window = nextBackoffWindow(window);
}
}//namespace

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\nESP-NOW NODE");

    //set up the radio and listen for hub replies
    radioReady = initializeRadio();
    if (!radioReady) {
        Serial.println("ESP-NOW radio initialization failed");
        return;
    }

    const esp_err_t result = esp_now_register_recv_cb(onReceive);
    if (result != ESP_OK) {
        radioReady = false;
        Serial.printf("Receive callback registration failed: %s\n",
                      esp_err_to_name(result));
        return;
    }

    WiFi.macAddress(nodeAddress);
    Serial.printf("Node ready; MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
                  nodeAddress[0], nodeAddress[1], nodeAddress[2],
                  nodeAddress[3], nodeAddress[4], nodeAddress[5]);
}

bool openCommunication() {
    if (!radioReady) {
        return false;
    }

    //keep trying until the hub lets us in
    reconnectEnabled = true;
    uint32_t backoffWindow = kInitialBackoffWindowMs;
    while (reconnectEnabled && !sessionOpen) {
        
        //first we find the hub
        if (!hubDiscovered) {
            const uint8_t discover = static_cast<uint8_t>(MessageType::Discover);
            esp_now_send(kBroadcastAddress, &discover, sizeof(discover));

            const uint32_t start = millis();
            while (millis() - start < kResponseTimeoutMs && !hubDiscovered) {
                delay(10);
            }

            //if we no find, we wait and try again
            if (!hubDiscovered) {
                Serial.println("[STATUS] Hub not found");
                waitBeforeRetry(backoffWindow);
                continue;
            }

            Serial.printf("[STATUS] Found hub %02X:%02X:%02X:%02X:%02X:%02X\n",
                          hubAddress[0], hubAddress[1], hubAddress[2],
                          hubAddress[3], hubAddress[4], hubAddress[5]);
        }

        controlResponse = 0;
        const uint8_t handshake = static_cast<uint8_t>(MessageType::Handshake);
        esp_now_send(hubAddress, &handshake, sizeof(handshake));

        //wait for the hub to accept
        if (waitForControlResponse() &&
            controlResponse == static_cast<uint8_t>(MessageType::Ready)) {
            sessionOpen = true;
            Serial.println("[STATUS] Communication open");
            return sessionOpen;
        }

        if (controlResponse == static_cast<uint8_t>(MessageType::Busy)) {
            Serial.println("[STATUS] Hub is busy");
        } else {
            Serial.println("[STATUS] Handshake timed out");
            hubDiscovered = false;
        }
        waitBeforeRetry(backoffWindow);
    }

    return true;
}

bool sendData(const uint8_t* data, size_t length) {
    if (!sessionOpen || (data == nullptr && length > 0)) {
        return false;
    }
    if (length == 0) {
        return true;
    }

    //how many packets
    const uint32_t chunkCount =
        static_cast<uint32_t>(1 + (length - 1) / kMaxChunkSize);

    const uint32_t transferId = nextTransferId++;
    size_t offset = 0;

    //send one chunk at a time
    for (uint32_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex) {
        const size_t payloadLength =
            length - offset < kMaxChunkSize ? length - offset : kMaxChunkSize;
        
        uint8_t packet[ESP_NOW_MAX_DATA_LEN];
        
        const DataHeader header = {
            MessageType::Data,
            transferId,
            chunkIndex,
            chunkCount,
        };

        memcpy(packet, &header, sizeof(header));
        memcpy(packet + sizeof(header), data + offset, payloadLength);

        bool chunkAcknowledged = false;

        //retry if the ack doesnt arrive
        for (uint8_t attempt = 0; attempt < kMaxChunkAttempts; ++attempt) {
            ackTransferId = transferId;
            ackChunkIndex = chunkIndex;
            ackReceived = false;

            const size_t packetLength = sizeof(header) + payloadLength;
            const esp_err_t result = esp_now_send(hubAddress, packet, packetLength);
            if (result == ESP_OK) {
                const uint32_t start = millis();
                while (millis() - start < kAckTimeoutMs && !ackReceived) {
                    delay(5);
                }
                if (ackReceived) {
                    chunkAcknowledged = true;
                    break;
                }
            }
        }

        if (!chunkAcknowledged) {
            Serial.printf("[ERROR] No ACK for chunk %u\n", chunkIndex);
            return false;
        }

        //next chunk after the hub confirms this chunk
        offset += payloadLength;
    }

    return true;
}

bool closeCommunication() {
    if (!sessionOpen) {
        reconnectEnabled = false;
        return true;
    }

    //tell the hub we're done
    const uint8_t close = static_cast<uint8_t>(MessageType::Close);
    
    //retry in case the close reply gets lost
    for (uint8_t attempt = 0; attempt < kMaxChunkAttempts; ++attempt) {
        controlResponse = 0;
        if (esp_now_send(hubAddress, &close, sizeof(close)) == ESP_OK &&
            waitForControlResponse() &&
            controlResponse == static_cast<uint8_t>(MessageType::Closed)) {
            sessionOpen = false;
            reconnectEnabled = false;
            Serial.println("[STATUS] Communication closed");
            return true;
        }
    }

    Serial.println("[ERROR] Hub did not confirm session close");
    return false;
}

void loop() {
    if (!radioReady) {
        delay(1000);
        return;
    }

    if (reconnectEnabled && !sessionOpen) {
        openCommunication();
    } else {
        delay(100);
    }
}
