#pragma once

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

/**
 * Controls the ST7789 display.
 * Provides a header and four text lines
 * for displaying information.
 */
class Display
{
private:
    Arduino_DataBus *bus;
    Arduino_GFX *gfx;

public:
    Display();

    void begin();

    void set_header(String text);

    void set_line(int line, String text);

    void test_colors();
};