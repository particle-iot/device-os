
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
void D0_callback()
{    
}

test(api_wiring_interrupt) {
        
    API_COMPILE(interrupts());
    API_COMPILE(noInterrupts());
    
    API_COMPILE(attachInterrupt(D0, D0_callback, RISING));
    API_COMPILE(detachInterrupt(D0));
   
}

void TIM3_callback()
{    
}

test(api_wiring_system_interrupt) {
            
    API_COMPILE(attachSystemInterrupt(SysInterrupt_TIM3, TIM3_callback));
    API_COMPILE(detachSystemInterrupt(SysInterrupt_TIM3));
   
}