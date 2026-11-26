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

//every node that has handshaken keeps its id; connected nodes can be addressed by the pi
constexpr size_t kMaxNodes = 8;

struct NodeSlot {
    bool known;
    bool connected;
    uint8_t address[6];
    uint8_t id;
    bool transferActive;
    uint32_t activeTransferId;
    uint32_t activeChunkCount;
    uint32_t nextChunkIndex;
    bool hasLastChunk;
    uint32_t lastTransferId;
    uint32_t lastChunkIndex;
    uint32_t activeRequestId;   //latest request sent to this node, tags its chunked data
};

NodeSlot nodes[kMaxNodes] = {};
uint8_t nextNodeId = 1;

//usb serial frames between the pi and the hub
constexpr uint8_t kPiMagic = 0xA5;
constexpr uint8_t kPiKindNode = 0;      //hub -> pi: response or data from a node
constexpr uint8_t kPiKindButton = 1;    //hub -> pi: button press, request id is 0
constexpr uint8_t kPiKindRequest = 2;   //pi -> hub: request for a node
constexpr uint8_t kPiKindList = 3;      //pi -> hub: send an event for every connected node
constexpr uint8_t kPiKindEvent = 3;     //hub -> pi: body [1 = connected, 0 = disconnected][6 byte MAC]

#pragma pack(push, 1)
struct PiFrameHeader {
    uint8_t magic;
    uint8_t kind;
    uint8_t nodeId;         //0 when no node is involved
    uint32_t requestId;
    uint32_t transferId;    //0 for single message responses
    uint32_t chunkIndex;
    uint32_t chunkCount;
    uint16_t length;        //body bytes that follow
};
#pragma pack(pop)

//request from pi: [magic][kind][u8 nodeId][u32 requestId][u8 commandLength][command][kRequestParamBytes params]
//list from pi: [magic][kind]
constexpr size_t kPiRequestHeaderBytes = 8;
constexpr size_t kPiListBytes = 2;
constexpr size_t kPiRequestMaxBytes = kPiRequestHeaderBytes + 255 + kRequestParamBytes;
uint8_t piBuffer[kPiRequestMaxBytes];
size_t piBufferLength = 0;

NodeSlot* findNode(const uint8_t address[6]) {
    for (NodeSlot& node : nodes) {
        if (node.known && sameAddress(node.address, address)) {
            return &node;
        }
    }
    return nullptr;
}

NodeSlot* findConnectedNodeById(uint8_t id) {
    for (NodeSlot& node : nodes) {
        if (node.connected && node.id == id) {
            return &node;
        }
    }
    return nullptr;
}

NodeSlot* freeSlot() {
    for (NodeSlot& node : nodes) {
        if (!node.known) {
            return &node;
        }
    }
    return nullptr;
}

void resetTransfer(NodeSlot& node) {
    
    //forget the current transfer
    node.transferActive = false;
    node.activeTransferId = 0;
    node.activeChunkCount = 0;
    node.nextChunkIndex = 0;
    node.hasLastChunk = false;
}

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

void sendNodeAssignment(const uint8_t address[6], uint8_t nodeId) {
    const NodeAssignmentMessage msg = {MessageType::NodeAssignment, nodeId};
    const esp_err_t result = esp_now_send(address,
                                          reinterpret_cast<const uint8_t*>(&msg),
                                          sizeof(msg));
    if (result != ESP_OK) {
        Serial0.printf("Could not send node assignment: %s\n", esp_err_to_name(result));
    }
}

//writes one frame to the pi, returns false if usb serial did not take all bytes
bool writePiFrame(uint8_t kind, uint8_t nodeId, uint32_t requestId, uint32_t transferId,
                  uint32_t chunkIndex, uint32_t chunkCount,
                  const uint8_t* body, size_t length) {
    const PiFrameHeader header = {kPiMagic, kind, nodeId, requestId, transferId,
                                  chunkIndex, chunkCount,
                                  static_cast<uint16_t>(length)};
    
    if (Serial.write(reinterpret_cast<const uint8_t*>(&header), sizeof(header)) != sizeof(header)) {
        return false;
    }
    return length == 0 || Serial.write(body, length) == length;
}

void sendPiError(uint8_t nodeId, uint32_t requestId, const char* text) {
    writePiFrame(kPiKindNode, nodeId, requestId, 0, 0, 1,
                 reinterpret_cast<const uint8_t*>(text), strlen(text));
}

void sendNodeEvent(const NodeSlot& node, bool connected) {
    uint8_t body[7];
    body[0] = connected ? 1 : 0;
    memcpy(body + 1, node.address, sizeof(node.address));
    writePiFrame(kPiKindEvent, node.id, 0, 0, 0, 1, body, sizeof(body));
}

//params points to kRequestParamBytes bytes, or nullptr for a command without params
//does not wait: the node answers "ok" and later sends its data tagged with requestId
bool sendRequest(NodeSlot& node, uint32_t requestId, const uint8_t* command,
                 size_t commandLength, const uint8_t* params) {
    if (commandLength > 240 ||
        sizeof(RequestMessage) + commandLength + kRequestParamBytes > ESP_NOW_MAX_DATA_LEN) {
        return false;
    }

    uint8_t packet[ESP_NOW_MAX_DATA_LEN];
    
    RequestMessage* req = reinterpret_cast<RequestMessage*>(packet);
    req->type = MessageType::Request;
    req->requestId = requestId;
    req->commandLength = static_cast<uint8_t>(commandLength);
    
    size_t offset = sizeof(RequestMessage);
    
    if (commandLength > 0) {
        memcpy(packet + offset, command, commandLength);
        offset += commandLength;
    }
    
    //params are always sent, zeros if the command has none
    if (params != nullptr) {
        memcpy(packet + offset, params, kRequestParamBytes);
    } else {
        memset(packet + offset, 0, kRequestParamBytes);
    }
    offset += kRequestParamBytes;
    
    const esp_err_t result = esp_now_send(node.address, packet, offset);
    if (result != ESP_OK) {
        Serial0.printf("Could not send request: %s\n", esp_err_to_name(result));
        return false;
    }
    
    node.activeRequestId = requestId;
    return true;
}

//reads complete requests from the pi out of usb serial
void pollPi() {
    while (Serial.available() > 0 && piBufferLength < sizeof(piBuffer)) {
        piBuffer[piBufferLength++] = static_cast<uint8_t>(Serial.read());
    }

    while (true) {
        
        //resync on the next magic byte
        size_t skip = 0;
        while (skip < piBufferLength && piBuffer[skip] != kPiMagic) {
            ++skip;
        }
        if (skip > 0) {
            memmove(piBuffer, piBuffer + skip, piBufferLength - skip);
            piBufferLength -= skip;
        }
        
        if (piBufferLength < kPiListBytes) {
            return;
        }

        if (piBuffer[1] == kPiKindList) {
            
            //the pi may start after the nodes did, so it asks who is connected
            for (const NodeSlot& node : nodes) {
                if (node.connected) {
                    sendNodeEvent(node, true);
                }
            }
            memmove(piBuffer, piBuffer + kPiListBytes, piBufferLength - kPiListBytes);
            piBufferLength -= kPiListBytes;
            continue;
        }

        if (piBuffer[1] != kPiKindRequest) {
            memmove(piBuffer, piBuffer + 1, --piBufferLength);
            continue;
        }

        if (piBufferLength < kPiRequestHeaderBytes) {
            return;
        }

        const size_t commandLength = piBuffer[7];
        const size_t total = kPiRequestHeaderBytes + commandLength + kRequestParamBytes;
        if (piBufferLength < total) {
            return;
        }

        const uint8_t nodeId = piBuffer[2];
        uint32_t requestId;
        memcpy(&requestId, piBuffer + 3, sizeof(requestId));
        
        NodeSlot* node = findConnectedNodeById(nodeId);
        if (node == nullptr) {
            sendPiError(nodeId, requestId, "error:no_node");
        } else if (!sendRequest(*node, requestId, piBuffer + kPiRequestHeaderBytes, commandLength,
                                piBuffer + kPiRequestHeaderBytes + commandLength)) {
            sendPiError(nodeId, requestId, "error:hub_send_failed");
        }

        memmove(piBuffer, piBuffer + total, piBufferLength - total);
        piBufferLength -= total;
    }
}

void processData(const ReceivedMessage& message) {

    //only a connected node can send data
    NodeSlot* node = findNode(message.senderAddress);
    if (node == nullptr || !node->connected) {
        sendTargetedControl(message.senderAddress, MessageType::Busy);
        return;
    }

    if (message.length <= sizeof(DataHeader)) {
        Serial0.println("Dropped empty or faulty data chunk");
        return;
    }

    DataHeader header = {};
    memcpy(&header, message.bytes, sizeof(header));
    if (header.type != MessageType::Data || header.chunkCount == 0 ||
        header.chunkIndex >= header.chunkCount) {
        Serial0.println("Dropped malformed data chunk");
        return;
    }

    if (node->hasLastChunk && header.transferId == node->lastTransferId &&
        header.chunkIndex == node->lastChunkIndex) {
        
        //the ack may have been lost, so ack the retry without writing twice
        sendAck(message.senderAddress, header.transferId, header.chunkIndex);
        return;
    }

    if (node->transferActive && header.chunkIndex == 0 &&
        header.transferId != node->activeTransferId) {
        resetTransfer(*node);
    }

    if (!node->transferActive) {
        
        //a transfer always starts with chunk zero
        if (header.chunkIndex != 0) {
            Serial0.println("Dropped out-of-order data chunk");
            return;
        }
        node->transferActive = true;
        node->activeTransferId = header.transferId;
        node->activeChunkCount = header.chunkCount;
        node->nextChunkIndex = 0;
    }

    if (header.transferId != node->activeTransferId ||
        header.chunkCount != node->activeChunkCount ||
        header.chunkIndex != node->nextChunkIndex) {
        Serial0.println("Dropped out-of-order data chunk");
        return;
    }

    const uint8_t* payload = message.bytes + sizeof(DataHeader);
    const size_t payloadLength = message.length - sizeof(DataHeader);
    if (!writePiFrame(kPiKindNode, node->id, node->activeRequestId, header.transferId,
                      header.chunkIndex, header.chunkCount, payload, payloadLength)) {
        Serial0.println("Could not write complete chunk to USB serial");
        return;
    }

    node->hasLastChunk = true;
    node->lastTransferId = header.transferId;
    node->lastChunkIndex = header.chunkIndex;
    
    //ack only after the bytes made it to usb serial
    sendAck(message.senderAddress, header.transferId, header.chunkIndex);

    ++node->nextChunkIndex;
    if (node->nextChunkIndex == node->activeChunkCount) {
        node->transferActive = false;
        node->activeTransferId = 0;
        node->activeChunkCount = 0;
        node->nextChunkIndex = 0;
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
        NodeSlot* node = findNode(message.senderAddress);
        
        if (node != nullptr && node->connected) {
            
            //a node that is already in just gets its id again
            sendControl(message.senderAddress, MessageType::Ready);
            sendNodeAssignment(message.senderAddress, node->id);
            return;
        }

        //a returning node keeps its id, a new one needs a free slot
        if (node == nullptr) {
            node = freeSlot();
        }
        if (node == nullptr || !addPeer(message.senderAddress)) {
            sendTargetedControl(message.senderAddress, MessageType::Busy);
            return;
        }

        if (!node->known) {
            memcpy(node->address, message.senderAddress, sizeof(node->address));
            node->id = nextNodeId++;
            node->known = true;
        }
        node->connected = true;
        resetTransfer(*node);
        Serial0.printf("Node connected: %02X:%02X:%02X:%02X:%02X:%02X (ID: %u)\n",
                       node->address[0], node->address[1], node->address[2],
                       node->address[3], node->address[4], node->address[5],
                       node->id);
        sendControl(message.senderAddress, MessageType::Ready);
        sendNodeAssignment(message.senderAddress, node->id);
        sendNodeEvent(*node, true);
        return;
    }

    if (type == MessageType::Close && message.length == 1) {
        
        //forget the connection but keep the id. TODO: handle timeout for inactive nodes
        NodeSlot* node = findNode(message.senderAddress);
        if (node != nullptr && node->connected) {
            node->connected = false;
            resetTransfer(*node);
            Serial0.printf("Node disconnected (ID: %u)\n", node->id);
            sendNodeEvent(*node, false);
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

    //responses and small measurements go straight to the pi, no transfer needed
    if (type == MessageType::RequestResponse && 
        message.length >= sizeof(ResponseMessage) - sizeof(ResponseMessage::response)) {
        const ResponseMessage* resp = reinterpret_cast<const ResponseMessage*>(message.bytes);
        const size_t headerBytes = sizeof(ResponseMessage) - sizeof(resp->response);
        
        const NodeSlot* node = findNode(message.senderAddress);
        if (node == nullptr || !node->connected) {
            Serial0.println("Dropped response from a node that is not connected");
            return;
        }
        
        if (resp->responseLength > sizeof(resp->response) ||
            message.length < headerBytes + resp->responseLength) {
            Serial0.println("Dropped malformed response");
            return;
        }
        
        if (!writePiFrame(kPiKindNode, node->id, resp->requestId, 0, 0, 1,
                          resp->response, resp->responseLength)) {
            Serial0.println("Could not write response to USB serial");
        }
        return;
    }

    //button presses have no request id and only concern the pi
    if (type == MessageType::Button && message.length > 1) {
        const NodeSlot* node = findNode(message.senderAddress);
        if (!writePiFrame(kPiKindButton, node ? node->id : 0, 0, 0, 0, 1,
                          message.bytes + 1, message.length - 1)) {
            Serial0.println("Could not write button press to USB serial");
        }
        return;
    }

    if (type == MessageType::Data) {
        processData(message);
    }
}
} 

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

    //read requests from the pi
    pollPi();

    //handle the next queued message
    ReceivedMessage message = {};
    if (xQueueReceive(receiveQueue, &message, pdMS_TO_TICKS(10)) == pdTRUE) {
        processMessage(message);
    }
}