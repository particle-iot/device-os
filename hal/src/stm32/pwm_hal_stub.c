#include "pwm_hal.h"
#include "gpio_hal.h"
#include "pinmap_impl.h"

void HAL_PWM_Write_Ext(uint16_t pin, uint32_t value)
{
}

void HAL_PWM_Write_With_Frequency_Ext(uint16_t pin, uint32_t value, uint32_t pwm_frequency)
{
}

uint32_t HAL_PWM_Get_Frequency_Ext(uint16_t pin)
{
    return 0;
}

uint32_t HAL_PWM_Get_AnalogValue_Ext(uint16_t pin)
{
    return 0;
}

uint32_t HAL_PWM_Get_Max_Frequency(uint16_t pin)
{
    return 0;
}

uint8_t HAL_PWM_Get_Resolution(uint16_t pin)
{
    return 0;
}

void HAL_PWM_Set_Resolution(uint16_t pin, uint8_t resolution)
{
}
