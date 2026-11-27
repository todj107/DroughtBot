#include "Display.h"

// Waveshare ESP32-S3-LCD-1.9
#define LCD_BL   14
#define LCD_DC   11
#define LCD_CS   12
#define LCD_SCK  10
#define LCD_MOSI 13
#define LCD_RST   9

Display::Display()
{
    bus = new Arduino_HWSPI(
        LCD_DC,
        LCD_CS,
        LCD_SCK,
        LCD_MOSI
    );

    gfx = new Arduino_ST7789(
        bus,
        LCD_RST,
        1,          // Rotation
        true,       // IPS
        170, 320,   // Resolution
        35, 0, 35, 0
    );
}

void Display::begin()
{
    if (!gfx->begin())
    {
        Serial.println("LCD initialization failed!");
        return;
    }

    // Enable display backlight
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, LOW);

    gfx->fillScreen(BLACK);
    gfx->setTextWrap(false);

    test_colors();

    //gfx->fillScreen(BLACK);

    // Startup screen
    gfx->setTextSize(3);
    gfx->setTextColor(WHITE);
    gfx->setCursor(43, 45);
    gfx->print("DroughtBot v1");

    gfx->setTextSize(2);
    gfx->setCursor(94, 100);
    gfx->print("Every drop,");

    gfx->setCursor(22, 125);
    gfx->print("intelligently delivered");

    delay(2000);

    gfx->fillScreen(BLACK);
}

void Display::set_header(String text)
{
    gfx->fillRect(0, 0, 320, 34, CYAN);

    gfx->setTextSize(3);
    gfx->setTextColor(BLACK);
    gfx->setCursor(8, 6);
    gfx->print(text);
}

void Display::set_line(int line, String text)
{
    if (line < 0 || line >= 4)
        return;

    int y = 38 + line * 33;

    // Clear previous text
    gfx->fillRect(0, y, 320, 33, BLACK);

    gfx->setTextSize(2);
    gfx->setTextColor(WHITE);
    gfx->setCursor(8, y + 6);
    gfx->print(text);
}

void Display::test_colors()
{
    gfx->fillRect(5,   5, 100, 30, RED);
    gfx->fillRect(110, 5, 100, 30, GREEN);
    gfx->fillRect(215, 5, 100, 30, BLUE);

    delay(200);
}