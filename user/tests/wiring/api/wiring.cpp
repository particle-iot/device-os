
#include "testapi.h"

extern "C" {
#include "stm32f2xx_tim.h"
}
    
test(api_wiring_pinMode) {
        
    PinMode mode = PIN_MODE_NONE;
    API_COMPILE(mode=getPinMode(D0));    
    API_COMPILE(pinMode(D0, mode));
    
}