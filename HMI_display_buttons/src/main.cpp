
#include <Arduino.h>
#include "Display.h"

Display display;

void setup()
{
    Serial.begin(115200);

    // initalize display
    display.begin();

    // set header
    display.set_header("Scanner");

    // set line information 0-3
    display.set_line(0, "Ready");
    display.set_line(1, "Weight: 0g");
    display.set_line(2, "Water: 0ml");
    display.set_line(3, "Waiting...");
    delay(3000);
}

void loop()
{


}
