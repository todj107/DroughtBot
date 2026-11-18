#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>


/**
 * Controls the ST7735S display.
 * Provides a header and four text lines for displaying information.
 */
class Display
{
private:
    Arduino_DataBus *bus;
    Arduino_GFX *gfx;

public:
    /**
     * Create display object and configure SPI.
     */
    Display();

    /**
     * Initialize the display and show the startup screen.
     * Must be called in setup() before using other functions.
     */
    void begin();

    /**
     * Set the header text at the top of the display.
     * Replaces the previous header.
     *
     * @param text Text to display in the header.
     */
    void set_header(String text);

    /**
     * Write text to one of the four information lines.
     * Clears the previous text on the selected line.
     *
     * @param line Line number (0-3).
     * @param text Text to display.
     */
    void set_line(int line, String text);

    /**
     * Display rectangles for color testing.
     */
    void test_colors();
};
