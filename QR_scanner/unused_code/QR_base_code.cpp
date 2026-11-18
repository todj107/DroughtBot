

#include <Arduino.h>

// UART pins for QR reader
#define QR_RX_PIN 4
#define QR_TX_PIN 5

// Hardware UART for QR scanner
HardwareSerial qrScanner(1);

// Store recieved code
String qrData = "";

void setup()
{
  // Serial connection to computer
  Serial.begin(115200);

  // Serial connenction to QR scanner
  qrScanner.begin(
    9600,
    SERIAL_8N1,
    QR_RX_PIN,
    QR_TX_PIN
  );

  Serial.println("QR scanner is ready");
}

void loop()
{
  // Check werther the scanner has sent data
  while (qrScanner.available() > 0)
  {
    char c = qrScanner.read();

    if (c == '\r' || c == '\n')
    {
      if (qrData.length() > 0)
      {
        Serial.print("QR scanned: ");
        Serial.println(qrData);

        // Clear buffer for next scan
        qrData = "";

      }
    }
    else
    {
      // Append character to QR data
      qrData += c;
    }
  }
}
