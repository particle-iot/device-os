#include "application.h"

retained int previous = -1;

void setup()
{
    int now = rand();
    if (previous!=-1) {
        RGB.control(true);
        if (previous==now) {
           RGB.color(255,0,0);
           Serial.println("random seed not set");
        }
	else {
           RGB.color(0,255,0);
           Serial.println("random seet set");
        }
    }
    else {
        previous = now;
        System.reset();
    }
}

