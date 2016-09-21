
#include "gpio_hal.h"
#include "pinmap_impl.h"
#define NAMESPACE_WPI_PINMODE
#include "wiringPi.h"

inline bool is_valid_pin(pin_t pin) __attribute__((always_inline));
inline bool is_valid_pin(pin_t pin)
{
    return pin<TOTAL_PINS;
}

PinFunction HAL_Validate_Pin_Function(pin_t pin, PinFunction pinFunction)
{
    // Only digital pins supported by the Raspberry Pi
    return (!is_valid_pin(pin)) ? PF_NONE : PF_DIO;
}

void HAL_Pin_Mode(pin_t pin, PinMode mode)
{
    if (!is_valid_pin(pin)) {
	return;
    }

    switch (mode) {
	case INPUT:
	case INPUT_PULLUP:
	case INPUT_PULLDOWN:
	    pinModePi(pin, WPI_INPUT);
	    break;
	case OUTPUT:
	case AF_OUTPUT_PUSHPULL:
	case AF_OUTPUT_DRAIN:
	    pinModePi(pin, WPI_OUTPUT);
	    break;
	default:
	    return;
    }

    switch (mode) {
	case INPUT:
	    pullUpDnControl(pin, PUD_OFF);
	    break;
	case INPUT_PULLDOWN:
	    pullUpDnControl(pin, PUD_DOWN);
	    break;
	case INPUT_PULLUP:
	    pullUpDnControl(pin, PUD_UP);
	    break;
    }

    RPi_Pin_Info* PIN_MAP = HAL_Pin_Map();
    PIN_MAP[pin].pin_mode = mode;
}

PinMode HAL_Get_Pin_Mode(pin_t pin)
{
    return (!is_valid_pin(pin)) ? PIN_MODE_NONE : HAL_Pin_Map()[pin].pin_mode;
}

void HAL_GPIO_Write(pin_t pin, uint8_t value)
{
    if (!is_valid_pin(pin)) {
	return;
    }

    digitalWritePi(pin, value);
}

int32_t HAL_GPIO_Read(pin_t pin)
{
    if (!is_valid_pin(pin)) {
	return 0;
    }

    return digitalReadPi(pin);
}

uint32_t HAL_Pulse_In(pin_t pin, uint16_t value)
{
    // TODO
    return 0;
}

