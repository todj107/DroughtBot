#include "Display.h"
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

#define LCD_DC   12
#define LCD_CS   13
#define LCD_SCK  10
#define LCD_MOSI 11
#define LCD_RST  14

Display::Display()
{
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

void Display::begin()
{
    pinMode(LCD_RST, OUTPUT);

    digitalWrite(LCD_RST, HIGH);
    delay(100);

    digitalWrite(LCD_RST, LOW);
    delay(100);

    digitalWrite(LCD_RST, HIGH);
    delay(200);

    gfx->begin(1000000);
    
    gfx->fillScreen(BLACK);

    test_colors();

    gfx->setTextSize(2);
    gfx->setTextColor(WHITE);
    gfx->setCursor(3, 50);
    gfx->print("DroughtBot v1");

    gfx->setTextSize(1);
    gfx->setTextColor(WHITE);
    gfx->setCursor(5, 105);
    gfx->print("Every drop,");
    gfx->setCursor(5, 115);
    gfx->print("intelligently delivered");


	delay(2000);
    gfx->fillScreen(BLACK);
}

void Display::set_header(String text)
{
    gfx->fillRect(0, 0, 160, 25, CYAN);

    gfx->setFont(&FreeSansBold9pt7b);
    gfx->setTextSize(1);
    gfx->setTextColor(BLACK);
    gfx->setCursor(3, 20);
    gfx->print(text);
}

void Display::set_line(int line, String text)
{
    if (line < 0 || line >= 4)
        return;

    int y = 30 + line * 24;

    // Clear previous text
    gfx->fillRect(0, y, 160, 24, BLACK);

    gfx->setFont(&FreeSans9pt7b);
    gfx->setTextSize(1);
    gfx->setTextColor(WHITE);
    gfx->setCursor(3, y + 17);
    gfx->println(text);
}

void Display::test_colors()
{
    gfx->fillRect(0, 5, 50, 30, RED);
    gfx->fillRect(55, 5, 50, 30, GREEN);
    gfx->fillRect(110, 5, 50, 30, BLUE);
    delay(200);
}
