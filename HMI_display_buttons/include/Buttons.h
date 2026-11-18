#pragma once
#include <Arduino.h>

/**
 * Handles three pushbuttons connected to GPIO 38, 39 and 40.
 * Returns true while a button is pressed.
 */
class Buttons
{
public:
    void begin();

    bool button1();
    bool button2();
    bool button3();
};


