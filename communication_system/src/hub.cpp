#include "espnow_protocol.h"

namespace {
using namespace EspNowProtocol;

constexpr UBaseType_t kReceiveQueueLength = 8;

struct ReceivedMessage {
    uint8_t senderAddress[6];
    uint16_t length;
    uint8_t bytes[ESP_NOW_MAX_DATA_LEN];
};

//vars
QueueHandle_t receiveQueue = nullptr;
portMUX_TYPE droppedMessagesMux = portMUX_INITIALIZER_UNLOCKED;
uint32_t droppedMessages = 0;
bool hubReady = false;
bool nodeConnected = false;
uint8_t connectedNode[6] = {};
bool transferActive = false;
uint32_t activeTransferId = 0;
uint32_t activeChunkCount = 0;
uint32_t nextChunkIndex = 0;
bool hasLastChunk = false;
uint32_t lastTransferId = 0;
uint32_t lastChunkIndex = 0;

void onReceive(const esp_now_recv_info_t* senderInfo,
               const uint8_t* data, int length) {
    
    //copy messages here so the main loop can handle them
    if (senderInfo == nullptr || senderInfo->src_addr == nullptr ||
        data == nullptr || length <= 0 ||
        length > ESP_NOW_MAX_DATA_LEN || receiveQueue == nullptr) {
        return;
    }

    ReceivedMessage message = {};
    
    memcpy(message.senderAddress, senderInfo->src_addr, sizeof(message.senderAddress));
    message.length = static_cast<uint16_t>(length);
    memcpy(message.bytes, data, message.length);

    if (xQueueSend(receiveQueue, &message, 0) != pdTRUE) {
        
        //keep track if the queue is full
        portENTER_CRITICAL(&droppedMessagesMux);
        
        ++droppedMessages;
        portEXIT_CRITICAL(&droppedMessagesMux);
    }
}

bool sendControl(const uint8_t address[6], MessageType type) {
    const uint8_t message = static_cast<uint8_t>(type);
    const esp_err_t result = esp_now_send(address, &message, sizeof(message));
    if (result != ESP_OK) {
        Serial0.printf("Could not send control message: %s\n", esp_err_to_name(result));
        return false;
    }
    return true;
}

void sendTargetedControl(const uint8_t target[6], MessageType type) {
    
    //broadcast reply
    TargetedControlMessage message = {type, {0}};
    memcpy(message.targetAddress, target, sizeof(message.targetAddress));
    const esp_err_t result = esp_now_send(kBroadcastAddress,
                                          reinterpret_cast<const uint8_t*>(&message),
                                          sizeof(message));
    if (result != ESP_OK) {
        Serial0.printf("Could not send control message: %s\n", esp_err_to_name(result));
    }
}

void sendAck(const uint8_t address[6], uint32_t transferId,
             uint32_t chunkIndex) {
    
    const AckMessage ack = {MessageType::Ack, transferId, chunkIndex};
    const esp_err_t result = esp_now_send(address,
                                          reinterpret_cast<const uint8_t*>(&ack),
                                          sizeof(ack));
    if (result != ESP_OK) {
        Serial0.printf("Could not send ACK: %s\n", esp_err_to_name(result));
    }
}

void resetTransfer() {
    
    //forget the current transfer
    transferActive = false;
    activeTransferId = 0;
    activeChunkCount = 0;
    nextChunkIndex = 0;
}

void processData(const ReceivedMessage& message) {
    //only the connected node can send data
    if (!nodeConnected || !sameAddress(message.senderAddress, connectedNode)) {
        sendTargetedControl(message.senderAddress, MessageType::Busy);
        return;
    }

    if (message.length <= sizeof(DataHeader)) {
        Serial0.println("Dropped empty or malformed data chunk");
        return;
    }

    DataHeader header = {};
    memcpy(&header, message.bytes, sizeof(header));
    if (header.type != MessageType::Data || header.chunkCount == 0 ||
        header.chunkIndex >= header.chunkCount) {
        Serial0.println("Dropped malformed data chunk");
        return;
    }

    if (hasLastChunk && header.transferId == lastTransferId &&
        header.chunkIndex == lastChunkIndex) {
        //the ack may have been lost, so ack the retry without writing twice
        sendAck(message.senderAddress, header.transferId, header.chunkIndex);
        return;
    }

    if (transferActive && header.chunkIndex == 0 &&
        header.transferId != activeTransferId) {
        resetTransfer();
    }

    if (!transferActive) {
        //a transfer always starts with chunk zero
        if (header.chunkIndex != 0) {
            Serial0.println("Dropped out-of-order data chunk");
            return;
        }
        transferActive = true;
        activeTransferId = header.transferId;
        activeChunkCount = header.chunkCount;
        nextChunkIndex = 0;
    }

    if (header.transferId != activeTransferId ||
        header.chunkCount != activeChunkCount ||
        header.chunkIndex != nextChunkIndex) {
        Serial0.println("Dropped out-of-order data chunk");
        return;
    }

    const uint8_t* payload = message.bytes + sizeof(DataHeader);
    const size_t payloadLength = message.length - sizeof(DataHeader);
    if (Serial.write(payload, payloadLength) != payloadLength) {
        Serial0.println("Could not write complete chunk to USB serial");
        return;
    }

    hasLastChunk = true;
    lastTransferId = header.transferId;
    lastChunkIndex = header.chunkIndex;
    
    //ack only after the bytes made it to usb serial
    sendAck(message.senderAddress, header.transferId, header.chunkIndex);

    ++nextChunkIndex;
    if (nextChunkIndex == activeChunkCount) {
        resetTransfer();
    }
}

void processMessage(const ReceivedMessage& message) {
    const MessageType type = static_cast<MessageType>(message.bytes[0]);
    
    //any node can find the hub
    if (type == MessageType::Discover && message.length == 1) {
        sendControl(kBroadcastAddress, MessageType::DiscoverResponse);
        return;
    }

    if (type == MessageType::Handshake && message.length == 1) {
        
        //let the current node back in, or accept a new one if we're free
        if (nodeConnected && sameAddress(message.senderAddress, connectedNode)) {
            sendControl(message.senderAddress, MessageType::Ready);

        } else if (!nodeConnected && addPeer(message.senderAddress)) {
            memcpy(connectedNode, message.senderAddress, sizeof(connectedNode));
            nodeConnected = true;
            resetTransfer();
            hasLastChunk = false;
            Serial0.printf("Node connected: %02X:%02X:%02X:%02X:%02X:%02X\n",
                           connectedNode[0], connectedNode[1], connectedNode[2],
                           connectedNode[3], connectedNode[4], connectedNode[5]);
            sendControl(message.senderAddress, MessageType::Ready);

        } else {
            sendTargetedControl(message.senderAddress, MessageType::Busy);
        }
        return;
    }

    if (type == MessageType::Close && message.length == 1) {
        
        //free the hub when the connected node is done. TODO: handle timeout for inactive nodes
        if (nodeConnected && sameAddress(message.senderAddress, connectedNode)) {
            nodeConnected = false;
            resetTransfer();
            hasLastChunk = false;
            Serial0.println("Node disconnected");
            sendTargetedControl(message.senderAddress, MessageType::Closed);
            if (esp_now_is_peer_exist(message.senderAddress) &&
                esp_now_del_peer(message.senderAddress) != ESP_OK) {
                Serial0.println("Could not remove disconnected node peer");
            }
        } else {
            sendTargetedControl(message.senderAddress, MessageType::Closed);
        }
        return;
    }

    if (type == MessageType::Data) {
        processData(message);
    }
}
}  // namespace

void setup() {
    Serial.begin(115200);
    Serial0.begin(115200);

    //make a queue for messages from the radio callback
    receiveQueue = xQueueCreate(kReceiveQueueLength, sizeof(ReceivedMessage));
    if (receiveQueue == nullptr) {
        Serial0.println("Failed to create ESP-NOW receive queue");
        return;
    }

    //start the radio and listen for incoming messages
    if (!initializeRadio()) {
        Serial0.println("ESP-NOW radio initialization failed");
        return;
    }

    const esp_err_t result = esp_now_register_recv_cb(onReceive);
    if (result != ESP_OK) {
        Serial0.printf("Receive callback registration failed: %s\n",
                       esp_err_to_name(result));
        return;
    }

    hubReady = true;
    Serial0.printf("Hub ready; MAC %s; ESP-NOW channel %u\n",
                   WiFi.macAddress().c_str(), kChannel);
}

void loop() {
    if (!hubReady || receiveQueue == nullptr) {
        delay(100);
        return;
    }

    //read and reset the counter safely
    portENTER_CRITICAL(&droppedMessagesMux);
    
    const uint32_t dropped = droppedMessages;
    
    droppedMessages = 0;
    
    portEXIT_CRITICAL(&droppedMessagesMux);
    
    if (dropped > 0) {
        Serial0.printf("Dropped %lu message(s): receive queue full\n",
                       static_cast<unsigned long>(dropped));
    }

    //handle the next queued message
    ReceivedMessage message = {};
    if (xQueueReceive(receiveQueue, &message, pdMS_TO_TICKS(100)) == pdTRUE) {
        processMessage(message);
    }
}
