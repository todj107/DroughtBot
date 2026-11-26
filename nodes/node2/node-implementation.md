# requestCommand & requestParams Explained

## The hub sends

```cpp
struct RequestMessage {
    MessageType type;      
    uint32_t requestId;    // unique ID
    uint8_t commandLength; // bytes in the command string
    // actual data follows: [command string] + [2 bytes params]
};
```

## For example

Hub tells Node #2 to water

**RequestMessage header:**
- `requestId = 42` (this request is #42)
- `commandLength = 11`

**Followed by:**
- `requestCommand = "water_plant"`
- `requestParams = [50, 0]` (50ml of water)

The params are always 2 bytes. A command that needs less just ignores the extra bytes, and a command with no params gets [0, 0].

**requestCommand** = The operation name  
**requestParams** = The arguments for that operation

## In the node you set up a handler function

(I used AI to make parts of the example, I have edited it, but handle with care)

```cpp
void setup() {
    openCommunication();  // Start ESP-NOW radio
    
    // Register nodes own handler function
    setRequestHandler(myCommandHandler);
}

void myCommandHandler(uint32_t requestId, const char* command, const uint8_t* params) {
    if (strcmp(command, "water_plant") == 0) {

        uint16_t amountMl = params[1] * 256 + params[0];
        
        //do the watering
        pumpWater(amountMl);
        
        //send response back to hub
        sendOk(requestId);
    }
    else if (strcmp(command, "read_sensor") == 0) {
        int reading = readWeight();
        
        //send the sensor reading back to hub
        sendResponseValue(requestId, reading);
    }
    else {
        //unknown command
        sendError(requestId, 255);
    }
}

void loop() {
    handleRequest();  // Check if a request arrived and process it
    // Your other code
}
```

## Request Processing Flow

1. `handleRequest()` checks if `requestReceived` flag is set

2. If yes, it calls `processRequest()`

3. `processRequest()` calls your handler with the requestId, command and params

4. **Your handler must call a send function to return data:**
   - `sendOk(requestId)` - Send "ok"
   - `sendResponseValue(requestId, value)` - Send "ok:value"
   - `sendError(requestId, code)` - Send "error:code"
   - `sendResponseData(requestId, data, length)` - Send binary data

5. The handler has no return value. If it does not send a response, the hub waits until it times out

**Important:** The `requestId` is passed to your handler when a request arrives. Use it in the send functions to match the response with the original request.

**Example of sending different response types:**
```cpp
// if just acknowledge you got it
sendOk(requestId);

// if send a numeric value back
sendResponseValue(requestId, 500);

// if something went wrong, send an error code
sendError(requestId, 1);

// send binary data
uint8_t data[4] = {0x01, 0x02, 0x03, 0x04};
sendResponseData(requestId, data, 4);
```

sendResponseData() is used for sending images for example or many values.
sendOk() is used when we just want to ack the hub and say that we recieved, but we dont actually will get anything to replyu
and sendResponseValue() is used for returning a simple measurement like a weight.
sendError() is used when something went wrong, the code lets us separate different errors from eachother.
