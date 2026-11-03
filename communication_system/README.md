# Communication setyp for any node (ESP32)

Build and upload the node from this directory:

```text
pio run -e node -t upload
```

## Node functions

`node.h` declares these functions:

- `openCommunication()` discovers the hub and waits until communication opens.
  It retries while the hub is busy or unavailable.
- `sendData(data, length)` sends a byte buffer in ESP-NOW-sized chunks and waits
  for an acknowledgement for each chunk. Each chunk is retried up to five
  times. It returns `false` if there is no open session or a chunk is not
  acknowledged.
- `closeCommunication()` tells the hub that the node is done and releases the
  session.

Example:

```cpp
const uint8_t message[] = "hello";

if (openCommunication()) {
    sendData(message, sizeof(message));
    closeCommunication();
}
```

## Using the node code with existing firmware

The current `node.cpp` is a standalone firmware file: it defines
`setup()` and `loop()`. It cannot be added unchanged to firmware that already
defines those functions. `node.h` only declares the communication functions;
it does not contain their implementation. The node code needs to be refactored
before it can be used as a drop-in component in another firmware project.

TODO:

- Add a timeout to `openCommunication()` and discard the data if it expires.
- Make the node code a header file that can be included in other firmware.