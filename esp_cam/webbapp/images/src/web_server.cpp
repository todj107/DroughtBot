#include "web_server.h"

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_http_server.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "app_config.h"

namespace web_server {

namespace {

httpd_handle_t   g_server = nullptr;
SemaphoreHandle_t g_lock  = nullptr;  // the HTTP server runs in its own task

uint8_t* g_jpeg    = nullptr;  // copy of the latest photo
size_t   g_jpegLen = 0;
size_t   g_jpegCap = 0;

const char kPage[] = R"HTML(<!doctype html>
<html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Camera demo</title>
<style>
 body{font-family:system-ui,sans-serif;background:#111;color:#eee;text-align:center;margin:0;padding:1rem}
 img{max-width:100%;height:auto;background:#000}
</style></head><body>
<h3>Latest photo</h3>
<img id="p" alt="">
<p id="s">Waiting for first photo...</p>
<script>
 const img=document.getElementById('p'), st=document.getElementById('s');
 const refresh=()=>{ img.src='/latest.jpg?t='+Date.now(); };
 img.onload =()=>{ st.textContent='Updated '+new Date().toLocaleTimeString(); setTimeout(refresh,1000); };
 img.onerror=()=>{ setTimeout(refresh,1000); };
 refresh();
</script></body></html>)HTML";

esp_err_t indexHandler(httpd_req_t* req) {
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, kPage, HTTPD_RESP_USE_STRLEN);
}

esp_err_t latestHandler(httpd_req_t* req) {
  xSemaphoreTake(g_lock, portMAX_DELAY);
  if (g_jpegLen == 0) {
    xSemaphoreGive(g_lock);
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "no photo yet");
    return ESP_OK;
  }
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  const esp_err_t res =
      httpd_resp_send(req, reinterpret_cast<const char*>(g_jpeg), g_jpegLen);
  xSemaphoreGive(g_lock);
  return res;
}

void registerUri(const char* uri, esp_err_t (*handler)(httpd_req_t*)) {
  const httpd_uri_t def = {
      .uri      = uri,
      .method   = HTTP_GET,
      .handler  = handler,
      .user_ctx = nullptr,
  };
  httpd_register_uri_handler(g_server, &def);
}

}  // namespace

bool begin() {
  g_lock = xSemaphoreCreateMutex();

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = app_config::kHttpPort;

  if (httpd_start(&g_server, &config) != ESP_OK) {
    Serial.println("[http] failed to start server");
    return false;
  }
  registerUri("/",           indexHandler);
  registerUri("/latest.jpg", latestHandler);
  return true;
}

void publish(const uint8_t* jpeg, size_t len) {
  if (g_lock == nullptr || jpeg == nullptr || len == 0) return;

  xSemaphoreTake(g_lock, portMAX_DELAY);

  if (len > g_jpegCap) {  // grow the buffer only when a bigger photo arrives
    free(g_jpeg);
    g_jpeg = static_cast<uint8_t*>(heap_caps_malloc(len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (g_jpeg == nullptr) {
      g_jpeg = static_cast<uint8_t*>(heap_caps_malloc(len, MALLOC_CAP_8BIT));
    }
    g_jpegCap = (g_jpeg != nullptr) ? len : 0;
    g_jpegLen = 0;
  }

  if (g_jpeg != nullptr) {
    memcpy(g_jpeg, jpeg, len);
    g_jpegLen = len;
  }

  xSemaphoreGive(g_lock);
}

}  // namespace web_server