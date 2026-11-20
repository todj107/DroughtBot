#include "QR_scanner.h"

// Private
void qrScanner::reset_buffer() {
	_buffer[0] = '\0';
	_len = 0;
}

// Public
void qrScanner::begin(int8_t rxPin, int8_t txPin) {
	_serial.begin(9600, SERIAL_8N1, rxPin, txPin);
};

void qrScanner::start_scan() {
	// Discard old data
	while (_serial.available() > 0) {
		_serial.read();
	}

	reset_buffer();

	_scanning = true;
}

bool qrScanner::get_qrcode(char *code) {
	if (!_scanning){
		// Start scan has not been called
		return false;
	}

	while (_serial.available() > 0 && _len <= QR_MAX_LEN) {
		char c = _serial.read();
		// End of QR code
		if (c == '\r' || c == '\n') {
			if (_len == 0)
				continue;
			
			// Add end char
			_buffer[_len] = '\0';
			_len += 1;

			memcpy(code, _buffer, _len);
			reset_buffer();
			_scanning = false;

			return true;
		}
		else {
			_buffer[_len] = c;
			_len += 1;
		}
	}

	return false;
}