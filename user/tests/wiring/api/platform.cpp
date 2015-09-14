
#include "testapi.h"

#if defined(STM32F2XX)
extern "C" {
#include "stm32f2xx_tim.h"
}

test(api_platform_stm32f2) {


    API_COMPILE(TIM_InternalClockConfig(NULL));



}

#endif


test(system_ticks)
{
    uint32_t value;
    API_COMPILE(value=System.ticks());
    API_COMPILE(value=System.ticksPerMicrosecond());
    API_COMPILE(System.ticksDelay(30));
    (void)value;
}