
#include <Arduino.h>
#include "Display.h"
#include "Buttons.h"

Display display;
Buttons buttons;

void setup()
{
    Serial.begin(115200);

    // initialize buttons
    buttons.begin();

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
    display.set_header("Buttons");

    if (buttons.button1())
    {
        display.set_line(0, "Button 1 pressed");
    }

    if (buttons.button2())
    {
        display.set_line(1, "Button 2 pressed");
    }

    if (buttons.button3())
    {
        display.set_line(2, "Button 3 pressed");
    }

    delay(100);
}

