
#include "gpio_hal.h"


PinFunction HAL_Validate_Pin_Function(pin_t pin, PinFunction pinFunction)
{
    return PF_DIO;
}

void HAL_Pin_Mode(pin_t pin, PinMode mode)
{
}

PinMode HAL_Get_Pin_Mode(pin_t pin)
{
    return PIN_MODE_NONE;
}

void HAL_GPIO_Write(pin_t pin, uint8_t value)
{
}

int32_t HAL_GPIO_Read(pin_t pin)
{
    return 0;
}

uint32_t HAL_Pulse_In(pin_t pin, uint16_t value)
{
	return 0;
}

