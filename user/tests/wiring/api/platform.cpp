
#include "testapi.h"

#if defined(STM32F2XX)
extern "C" {
#include "stm32f2xx_tim.h"
}

test(api_platform_stm32f2) {


    API_COMPILE(TIM_InternalClockConfig(NULL));



}

#endif