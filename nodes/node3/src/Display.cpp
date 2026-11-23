#include "Display.h"
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

#define LCD_DC   12
#define LCD_CS   13
#define LCD_SCK  10
#define LCD_MOSI 11
#define LCD_RST  14

#define INDENT 3
#define LINE_BREAK 10

/**
 * 
 * Number of characters pert line, with font size,
 *      - 1: 26 chars (size (6x8))
 *      - 2: 13 chars (size (12x16))
 * 
 * God line break size:
 *      - 1: ca 10px
 *      - 2: ca 15px
 */

// Need to change this class. 
// It should print given message on display, and wrap if the message is to long.
// Maybe some labels to the buttons. 
// Maybe some meaningfull header, like status or which process it is on.
// Most of the code right now is unusable.
// Want a clear_header(), clear_message(), set_button(1, 2 or 3). Or similar.

Display::Display(){
    bus = new Arduino_HWSPI(
        LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI
    );
    gfx = new Arduino_ST7735(
        bus, LCD_RST,
        1,              // Rotation
        false,          // IPS
        128, 160,       // Resolution
        0, 0, 0, 0,     // Offsets
        false           // BGR
    );
}

bool Display::begin() {
    if (!gfx->begin()) {
        return false;
    }

    gfx->fillScreen(BLACK);
    gfx->setTextSize(2);
    gfx->setTextColor(WHITE);
    gfx->setCursor(3, 50);
    gfx->print("DroughtBot v1");

    return true;
}

void Display::clear_header() {
    gfx->fillRect(0, _head_box_y[0], _width, _head_box_y[1], CYAN);
}

void Display::clear_message() {
    gfx->fillRect(0, _msg_box_y[0], _width, _msg_box_y[1], BLACK);
}

void Display::clear_button(int btn) {
    gfx->fillRect(_btn_cursor_x[btn], _btn_box_y[0], _btn_length[btn], _btn_box_y[1], BLACK);
}

void Display::set_header(char *text) {
    clear_header();

    gfx->setFont(&FreeSansBold9pt7b);
    gfx->setTextSize(1);
    gfx->setTextColor(BLACK);
    gfx->setCursor(INDENT, _head_cursor_y);
    gfx->print(text);
}

// void Display::set_line(int line, char *text)
// {
//     if (line < 0 || line >= 4)
//         return;

//     int y = 30 + line * 24;

//     // Clear previous text
//     gfx->fillRect(0, y, 160, 24, BLACK);

//     gfx->setFont(&FreeSans9pt7b);
//     gfx->setTextSize(1);
//     gfx->setTextColor(WHITE);
//     gfx->setCursor(3, y + 17);
//     gfx->println(text);
// }

void Display::set_message(char *text) {
    clear_message();

    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextSize(1);
    gfx->setTextColor(WHITE);

    gfx->setCursor(INDENT, _msg_cursor_y);
    gfx->print(text);    
}

void Display::set_button(int btn, char *text) {
    int pos_x = _btn_cursor_x[btn];
    int pos_y = _btn_cursor_y;

    clear_button(btn);
    _btn_length[btn] = sizeof(text)/sizeof(char); 

    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextSize(1);
    gfx->setTextColor(WHITE);

    gfx->setCursor(pos_x, pos_y);
    gfx->print(text);
}

// void Display::test_colors()
// {
//     gfx->fillRect(0, 5, 50, 30, RED);
//     gfx->fillRect(55, 5, 50, 30, GREEN);
//     gfx->fillRect(110, 5, 50, 30, BLUE);
//     delay(200);
// }
