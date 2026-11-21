#pragma once
#include <Arduino.h>

#define QR_MAX_LEN 300 // max characters in a QR code
#define QR_BUFFER_SIZE (QR_MAX_LEN + 1) // +1 for the '\0' terminator

class qrScanner {
private:
	HardwareSerial _serial{1};
	
	char _buffer[QR_BUFFER_SIZE];
	bool _scanning = false;
	uint8_t _len = 0;

	void reset_buffer();	

public:
	void begin(int8_t rxPin, int8_t txPin);
	void start_scan();
	bool get_qrcode(char *code);
};

