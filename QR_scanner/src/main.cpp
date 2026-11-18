
#include "QR_scanner.h"

QR_scanner qr_scanner;

void setup() {
    Serial.begin(115200);
    qr_scanner.begin();

    Serial.println("Press 's' to scan QR code");
}

void loop() {
    if (Serial.available()) {
        if (Serial.read() == 's') {
            qr_scanner.start_scan();
            Serial.println("Scanning...");
        }
    }

    String code;

    if (qr_scanner.update(code)) {
        Serial.print("QR scanned: ");
        Serial.println(code);
    }  
}


