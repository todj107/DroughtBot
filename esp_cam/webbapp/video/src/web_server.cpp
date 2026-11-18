#include "web_server.h"

#include <Arduino.h>
#include <esp_camera.h>
#include <esp_http_server.h>

#include "app_config.h"

namespace web_server {

namespace {

httpd_handle_t g_controlServer = nullptr;
httpd_handle_t g_streamServer  = nullptr;

constexpr const char* kStreamContentType = "multipart/x-mixed-replace;boundary=frame";
constexpr const char* kStreamBoundary    = "\r\n--frame\r\n";
constexpr const char* kStreamPartFormat  =
    "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %ld.%06ld\r\n\r\n";

// ---------------------------------------------------------------- index page

const char kPageHead[] = R"HTML(<!doctype html>
<html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32-S3 Cam</title>
<style>
 body{font-family:system-ui,sans-serif;background:#111;color:#eee;margin:0;padding:1rem}
 .row{display:flex;gap:.75rem;align-items:center;margin:.4rem 0}
 label{width:7rem} input[type=range]{flex:1;max-width:16rem}
 img{max-width:100%;height:auto;margin-top:1rem;background:#000}
 a,button{color:#8cf}
</style></head><body>
<h2>ESP32-S3 Cam</h2>
<div class="row"><label>Resolution</label><select id="framesize">)HTML";

const char kPageTail[] = R"HTML(</select></div>
<div class="row"><label>Quality</label><input id="quality" type="range" min="4" max="63"></div>
<div class="row"><label>Brightness</label><input id="brightness" type="range" min="-2" max="2"></div>
<div class="row"><label>Contrast</label><input id="contrast" type="range" min="-2" max="2"></div>
<div class="row"><label>Saturation</label><input id="saturation" type="range" min="-2" max="2"></div>
<div class="row"><label>H-mirror</label><input id="hmirror" type="checkbox"></div>
<div class="row"><label>V-flip</label><input id="vflip" type="checkbox"></div>
<div class="row"><a href="/capture" target="_blank">Capture still</a></div>
<img id="stream" alt="stream">
<script>
 const ids=['framesize','quality','brightness','contrast','saturation','hmirror','vflip'];
 const send=(v,x)=>fetch('/control?var='+v+'&val='+x);
 ids.forEach(id=>{
   const el=document.getElementById(id);
   el.onchange=()=>send(id, el.type==='checkbox' ? (el.checked?1:0) : el.value);
 });
 fetch('/status').then(r=>r.json()).then(s=>ids.forEach(id=>{
   const el=document.getElementById(id);
   if(el.type==='checkbox') el.checked=!!s[id]; else el.value=s[id];
 }));
 document.getElementById('stream').src='http://'+location.hostname+':)HTML";

const char kPageEnd[] = R"HTML(/stream';
</script></body></html>)HTML";

void addSizeOption(String& page, framesize_t size, const char* label) {
  page += "<option value='";
  page += String(static_cast<int>(size));
  page += "'>";
  page += label;
  page += "</option>";
}

esp_err_t indexHandler(httpd_req_t* req) {
  String page(kPageHead);
  addSizeOption(page, FRAMESIZE_QVGA, "QVGA 320x240");
  addSizeOption(page, FRAMESIZE_VGA,  "VGA 640x480");
  addSizeOption(page, FRAMESIZE_SVGA, "SVGA 800x600");
  addSizeOption(page, FRAMESIZE_XGA,  "XGA 1024x768");
  addSizeOption(page, FRAMESIZE_SXGA, "SXGA 1280x1024");
  addSizeOption(page, FRAMESIZE_UXGA, "UXGA 1600x1200");
  page += kPageTail;
  page += String(app_config::kStreamPort);
  page += kPageEnd;

  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, page.c_str(), page.length());
}

// ------------------------------------------------------------------- capture

esp_err_t captureHandler(httpd_req_t* req) {
  camera_fb_t* fb = esp_camera_fb_get();
  if (fb == nullptr) {
    Serial.println("[http] capture failed");
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  const esp_err_t res =
      httpd_resp_send(req, reinterpret_cast<const char*>(fb->buf), fb->len);
  esp_camera_fb_return(fb);
  return res;
}

// -------------------------------------------------------------------- stream

esp_err_t streamHandler(httpd_req_t* req) {
  esp_err_t res = httpd_resp_set_type(req, kStreamContentType);
  if (res != ESP_OK) return res;
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  char partHeader[128];
  while (true) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb == nullptr) {
      Serial.println("[http] stream: frame grab failed");
      res = ESP_FAIL;
      break;
    }

    res = httpd_resp_send_chunk(req, kStreamBoundary, strlen(kStreamBoundary));
    if (res == ESP_OK) {
      const size_t n = snprintf(partHeader, sizeof(partHeader), kStreamPartFormat,
                                static_cast<unsigned>(fb->len),
                                static_cast<long>(fb->timestamp.tv_sec),
                                static_cast<long>(fb->timestamp.tv_usec));
      res = httpd_resp_send_chunk(req, partHeader, n);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, reinterpret_cast<const char*>(fb->buf), fb->len);
    }
    esp_camera_fb_return(fb);

    if (res != ESP_OK) break;  // client went away
  }
  return res;
}

// ------------------------------------------------------------ status/control

esp_err_t statusHandler(httpd_req_t* req) {
  sensor_t* s = esp_camera_sensor_get();
  if (s == nullptr) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  char json[192];
  const int n = snprintf(json, sizeof(json),
                         "{\"framesize\":%d,\"quality\":%d,\"brightness\":%d,"
                         "\"contrast\":%d,\"saturation\":%d,\"hmirror\":%d,\"vflip\":%d}",
                         s->status.framesize, s->status.quality, s->status.brightness,
                         s->status.contrast, s->status.saturation, s->status.hmirror,
                         s->status.vflip);

  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, json, n);
}

esp_err_t controlHandler(httpd_req_t* req) {
  char query[64];
  char var[24];
  char val[16];

  if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
      httpd_query_key_value(query, "var", var, sizeof(var)) != ESP_OK ||
      httpd_query_key_value(query, "val", val, sizeof(val)) != ESP_OK) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "expected ?var=<name>&val=<int>");
    return ESP_FAIL;
  }

  sensor_t* s = esp_camera_sensor_get();
  if (s == nullptr) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  const int v = atoi(val);
  int rc = -1;
  if      (!strcmp(var, "framesize"))  rc = s->set_framesize(s, static_cast<framesize_t>(v));
  else if (!strcmp(var, "quality"))    rc = s->set_quality(s, v);
  else if (!strcmp(var, "brightness")) rc = s->set_brightness(s, v);
  else if (!strcmp(var, "contrast"))   rc = s->set_contrast(s, v);
  else if (!strcmp(var, "saturation")) rc = s->set_saturation(s, v);
  else if (!strcmp(var, "hmirror"))    rc = s->set_hmirror(s, v);
  else if (!strcmp(var, "vflip"))      rc = s->set_vflip(s, v);
  else {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "unknown variable");
    return ESP_FAIL;
  }

  if (rc != 0) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, nullptr, 0);
}

// ------------------------------------------------------------------- startup

void registerUri(httpd_handle_t server, const char* uri, esp_err_t (*handler)(httpd_req_t*)) {
  const httpd_uri_t def = {
      .uri      = uri,
      .method   = HTTP_GET,
      .handler  = handler,
      .user_ctx = nullptr,
  };
  httpd_register_uri_handler(server, &def);
}

}  // namespace

bool begin() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 8;
  config.lru_purge_enable = true;

  config.server_port = app_config::kHttpPort;
  if (httpd_start(&g_controlServer, &config) != ESP_OK) {
    Serial.println("[http] failed to start control server");
    return false;
  }
  registerUri(g_controlServer, "/",        indexHandler);
  registerUri(g_controlServer, "/capture", captureHandler);
  registerUri(g_controlServer, "/status",  statusHandler);
  registerUri(g_controlServer, "/control", controlHandler);

  // The stream handler never returns while a client is connected, so it gets
  // its own server (and its own control port) to keep the rest responsive.
  config.server_port = app_config::kStreamPort;
  config.ctrl_port += 1;
  if (httpd_start(&g_streamServer, &config) != ESP_OK) {
    Serial.println("[http] failed to start stream server");
    return false;
  }
  registerUri(g_streamServer, "/stream", streamHandler);

  return true;
}

}  // namespace web_server
