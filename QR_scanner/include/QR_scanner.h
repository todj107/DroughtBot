#pragma once
#include <Arduino.h>


class QR_scanner
{
private:
	HardwareSerial serial{1};
	
	String buffer = "";
	bool scanning = false;


public:
	void begin();
	void start_scan();
	bool update(String &code);
};

