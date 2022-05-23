
#include "testapi.h"

test(pwm_hal_backwards_compatibility)
{
    // These APIs are exposed to user application.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    // These APIs are known deprecated APIs, we don't need to see this warning in tests
    API_COMPILE(HAL_PWM_Write(0, 0));
    API_COMPILE(HAL_PWM_Write_Ext(0, 0));
    API_COMPILE(HAL_PWM_Write_With_Frequency(0, 0, 0));
    API_COMPILE(HAL_PWM_Write_With_Frequency_Ext(0, 0, 0));
    API_COMPILE(HAL_PWM_Get_Frequency(0));
    API_COMPILE(HAL_PWM_Get_Frequency_Ext(0));
    API_COMPILE(HAL_PWM_Get_AnalogValue(0));
    API_COMPILE(HAL_PWM_Get_AnalogValue_Ext(0));
    API_COMPILE(HAL_PWM_Get_Max_Frequency(0));
    API_COMPILE(HAL_PWM_Get_Resolution(0));
    API_COMPILE(HAL_PWM_Set_Resolution(0, 0));
#pragma GCC diagnostic pop
}
