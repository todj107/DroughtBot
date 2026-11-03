#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

namespace EspNowProtocol {

constexpr uint8_t kChannel = 1;
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
