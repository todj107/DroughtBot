#include <WiFi.h>

#include <esp_now.h>



uint8_t setupAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};



void onMessageReceived(const esp_now_recv_info_t *senderInfo, const uint8_t *message, int messageLength) {

  Serial.write(message, messageLength);

  Serial.println();

}



void setup() {

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  esp_now_init();

  esp_now_register_recv_cb(onMessageReceived);



  esp_now_peer_info_t broadcastPeer = {};

  memcpy(broadcastPeer.peer_addr, setupAddress, sizeof(setupAddress));

  esp_now_add_peer(&broadcastPeer);

}







void loop() {

  const char *message = "hello from node";

  esp_now_send(setupAddress, (const uint8_t *)message, strlen(message));

  delay(2000);

}