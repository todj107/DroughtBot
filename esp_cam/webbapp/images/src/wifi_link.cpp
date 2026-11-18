#include "wifi_link.h"

#include <WiFi.h>

#include "app_config.h"
#include "secrets.h"

namespace wifi_link {

bool connect() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);  // power-save adds latency to the video stream
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("[wifi] connecting");
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > app_config::kWifiTimeoutMs) {
      Serial.println("\n[wifi] timed out");
      return false;
    }
    delay(500);
    Serial.print('.');
  }

  Serial.printf("\n[wifi] connected, IP %s\n", WiFi.localIP().toString().c_str());
  return true;
}

}  // namespace wifi_link
