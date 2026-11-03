#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#define TYPE_DISCOVER 0
#define TYPE_DISCOVER_RESP 1
#define TYPE_DATA 2

bool discovered = false;
uint8_t peerMac[6] = {0};

void onReceive(const esp_now_recv_info_t *senderInfo, const uint8_t *data, int length) {
  if (length < 1) return;

  uint8_t type = data[0];

  if (type == TYPE_DISCOVER) {
    memcpy((uint8_t *)peerMac, senderInfo->src_addr, 6);
    uint8_t resp[1] = {TYPE_DISCOVER_RESP};
    esp_now_send(senderInfo->src_addr, resp, 1);
    Serial.println("[RX] DISCOVER - replied");
    
  } else if (type == TYPE_DISCOVER_RESP) {
    memcpy((uint8_t *)peerMac, senderInfo->src_addr, 6);
    discovered = true;
    Serial.println("[RX] DISCOVER_RESP");
    
  } else if (type == TYPE_DATA) {
    Serial.print("[RX] ");
    Serial.write(data + 1, length - 1);
    Serial.println();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ESP-NOW ===");
  
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  esp_now_init();
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peer = {};
  memset(peer.peer_addr, 0xFF, 6);
  peer.channel = 1;
  peer.ifidx = WIFI_IF_STA;
  esp_now_add_peer(&peer);
}

void loop() {
  if (!discovered) {
    uint8_t discover[1] = {TYPE_DISCOVER};
    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    Serial.println("[TX] DISCOVER");
    esp_now_send(broadcast, discover, 1);
    
    uint32_t start = millis();
    while (millis() - start < 2000 && !discovered) delay(50);
    
    if (!discovered) {
      Serial.println("[STATUS] Waiting...\n");
      delay(2000);
      return;
    }
    
    Serial.printf("[STATUS] Found: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
                  peerMac[0], peerMac[1], peerMac[2], peerMac[3], peerMac[4], peerMac[5]);
  }
  
  uint8_t msg[20] = {TYPE_DATA};
  memcpy(msg + 1, "hello", 5);
  Serial.println("[TX] hello");
  esp_now_send(peerMac, msg, 6);
  
  delay(2000);
}
