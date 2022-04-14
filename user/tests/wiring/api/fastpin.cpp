
#include "testapi.h"


test(api_fastpin) {

    uint8_t value=0;
    API_COMPILE(digitalWriteFast(D0, HIGH));
    API_COMPILE(digitalWriteFast(A0, LOW));
    API_COMPILE(pinSetFast(A0));
    API_COMPILE(pinResetFast(A0));
    (void)value++;
}

