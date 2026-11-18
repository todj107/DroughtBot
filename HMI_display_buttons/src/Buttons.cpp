#include "Buttons.h"

#define BTN_1 38
#define BTN_2 39
#define BTN_3 40

void Buttons::begin()
{
    pinMode(BTN_1, INPUT_PULLUP);
    pinMode(BTN_2, INPUT_PULLUP);
    pinMode(BTN_3, INPUT_PULLUP);
}

bool Buttons::button1()
{
    return digitalRead(BTN_1) == LOW;
}

bool Buttons::button2()
{
    return digitalRead(BTN_2) == LOW;
}

bool Buttons::button3()
{
    return digitalRead(BTN_3) == LOW;
}