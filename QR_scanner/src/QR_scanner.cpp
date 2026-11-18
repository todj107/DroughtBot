#include "QR_scanner.h"

#define QR_RX_PIN 4
#define QR_TX_PIN 5

void QR_scanner::begin() {
	serial.begin(9600, SERIAL_8N1, QR_RX_PIN, QR_TX_PIN);
};

void QR_scanner::start_scan() {
	buffer = "";

	// Discard old data
	while (Serial.available()) {
		serial.read();
	}
	scanning = true;
}

bool QR_scanner::update(String &code) {

	while (serial.available() > 0) {
		char c = serial.read();

		// Ignore data if scanning diabeled
		if (!scanning) {
			continue;
		}

		// End of QR code
		if (c == '\r' || c == '\n') {
			if (buffer.length() > 0) {
				code = buffer;
				buffer = "";
				scanning = false;
				return true;
			}
		}
		else {
			buffer += c;
		}
	}
	return false;
}