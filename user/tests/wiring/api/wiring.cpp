
#include "testapi.h"

test(api_wiring_pinMode) {

    PinMode mode = PIN_MODE_NONE;
    API_COMPILE(mode=getPinMode(D0));
    API_COMPILE(pinMode(D0, mode));

}

test(api_wiring_wire_setSpeed)
{
    API_COMPILE(Wire.setSpeed(CLOCK_SPEED_100KHZ));
}