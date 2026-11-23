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

    int _width = 160;
    int _height = 128;

    int _head_cursor_y = 20;
    int _head_box_y[2] = {0, 25};

    int _msg_box_y[2] = {25, 100};
    int _msg_cursor_y = _msg_box_y[0] + 10;

    int _btn_box_y[2] = {100, 128};
    int _btn_cursor_y = 125;
    int _btn_cursor_x[3] = {3,40,130}; // CHANGE TO CORRECT POS
    int _btn_length[3] = {0,0,0};


public:
    /**
     * Create display object and configure SPI.
     */
    Display();

    /**
     * Initialize the display and show the startup screen.
     * Must be called in setup() before using other functions.
     */
    bool begin();

    /**
     * Set the header text at the top of the display.
     * Replaces the previous header.
     *
     * @param text Text to display in the header.
     */
    void set_header(char *text);

    /**
     * Write text to one of the four information lines.
     * Clears the previous text on the selected line.
     *
     * @param line Line number (0-3).
     * @param text Text to display.
     */
    // void set_line(int line, char *text);

    void set_message(char *text);

    void set_button(int btn, char *text);

    void clear_message();

    void clear_button(int btn);

    void clear_header();
};
