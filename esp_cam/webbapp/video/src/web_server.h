#pragma once

namespace web_server {

// Starts two HTTP servers:
//   :80  /          control page
//        /capture   single JPEG
//        /status    sensor settings as JSON
//        /control   ?var=<name>&val=<int> to change a setting
//   :81  /stream    MJPEG stream (separate server so it cannot block the others)
bool begin();

}  // namespace web_server
