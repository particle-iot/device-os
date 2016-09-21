#include "timer_hal.h"
#include "wiringPi.h"

system_tick_t HAL_Timer_Get_Micro_Seconds(void)
{
    return microsPi();
}

system_tick_t HAL_Timer_Get_Milli_Seconds(void)
{
    return millisPi();
}

