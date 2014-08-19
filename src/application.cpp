#include "application.h"

void setup() {
    RGB.control(true);
    RGB.brightness(8);
    RGB.color(255,255,255);
    RGB.control(false);
}