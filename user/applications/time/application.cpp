#include "application.h"

void setup()
{
    pinMode(D7, OUTPUT);
}

void loop()
{
    static bool state = false;

    // Use literals like 150ms, 1.5s or 1h in delay
    delay(1.5s);

    digitalWrite(D7, state);
    state = !state;
}
