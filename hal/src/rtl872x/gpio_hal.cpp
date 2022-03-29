/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef __cplusplus
extern "C" {
#endif
#include "rtl8721d.h"
#ifdef __cplusplus
}
#endif
#include "gpio_hal.h"
#include "check.h"
#include "hw_ticks.h"
#include "timer_hal.h"

void hal_gpio_mode(hal_pin_t pin, PinMode mode) {
    hal_gpio_config_t conf = {};
    conf.size = sizeof(conf);
    conf.version = HAL_GPIO_VERSION;
    conf.mode = mode;
    hal_gpio_configure(pin, &conf, nullptr);
}

int hal_gpio_configure(hal_pin_t pin, const hal_gpio_config_t* conf, void* reserved) {
    CHECK_TRUE(hal_pin_is_valid(pin), SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(conf, SYSTEM_ERROR_INVALID_ARGUMENT);

    hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    PinMode mode = conf->mode;

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    if (pinInfo->type == HAL_PIN_TYPE_MCU) {
#endif
        // Just in case
#if defined (ARM_CORE_CM0)
        RCC_PeriphClockCmd(APBPeriph_GPIO, APBPeriph_GPIO_CLOCK, ENABLE);
#endif

        uint32_t rtlPin = hal_pin_to_rtl_pin(pin);

        if ((pinInfo->gpio_port == RTL_PORT_A && pinInfo->gpio_pin == 27) ||
                (pinInfo->gpio_port == RTL_PORT_B && pinInfo->gpio_pin == 3)) {
            Pinmux_Swdoff();
        }

        // Set pin function may reset nordic gpio configuration, should be called before the re-configuration
        if (mode != PIN_MODE_NONE) {
            hal_pin_set_function(pin, PF_DIO);
        } else {
            hal_pin_set_function(pin, PF_NONE);
        }

        // Pre-set the output value if requested to avoid a glitch
        if (conf->set_value && (mode == OUTPUT || mode == OUTPUT_OPEN_DRAIN || mode == OUTPUT_OPEN_DRAIN_PULLUP)) {
            GPIO_WriteBit(rtlPin, conf->value);
        }

        GPIO_InitTypeDef  GPIO_InitStruct = {};
        GPIO_InitStruct.GPIO_Pin = rtlPin;
        
        switch (mode) {
            case OUTPUT:
            case OUTPUT_OPEN_DRAIN: {
                GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
                GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
                break;
            }
            case INPUT: {
                GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
                GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
                break;
            }
            case INPUT_PULLUP: {
                GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
                GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
                break;
            }
            case INPUT_PULLDOWN: {
                GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
                GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
                break;
            }
            case OUTPUT_OPEN_DRAIN_PULLUP: {
                GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
                GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
                break;
            }
            case PIN_MODE_NONE: {
                GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
                GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
                break;
            }
            default: {
                // TODO
                return -1;
            }
        }

        GPIO_Init(&GPIO_InitStruct);
        pinInfo->pin_mode = mode;
#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    }
#if HAL_PLATFORM_MCP23S17
    else if (pinInfo->type == HAL_PIN_TYPE_IO_EXPANDER) {
        // TODO
    }
#endif // HAL_PLATFORM_MCP23S17
#if HAL_PLATFORM_DEMUX
    else if (pinInfo->type == HAL_PIN_TYPE_DEMUX) {
        // TODO
    }
#endif // HAL_PLATFORM_DEMUX
    else {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
#endif

    return 0;
}

PinMode hal_gpio_get_mode(hal_pin_t pin) {
    return (!hal_pin_is_valid(pin)) ? PIN_MODE_NONE : hal_pin_map()[pin].pin_mode;
}

void hal_gpio_write(hal_pin_t pin, uint8_t value) {
    if (!hal_pin_is_valid(pin)) {
        return;
    }

    hal_pin_info_t* pinInfo = hal_pin_map() + pin;

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    if (pinInfo->type == HAL_PIN_TYPE_MCU) {
#endif
        uint32_t rtlPin = hal_pin_to_rtl_pin(pin);
        // TODO: PWM have conflict with GPIO OUTPUT mode on Realtek
        if (pinInfo->pin_func == PF_PWM) {
            hal_gpio_mode(pin, OUTPUT);
        }
        GPIO_WriteBit(rtlPin, value);
#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    }
#if HAL_PLATFORM_MCP23S17
    else if (pinInfo->type == HAL_PIN_TYPE_IO_EXPANDER) {
        // TODO
    }
#endif
#if HAL_PLATFORM_DEMUX
    else if (pinInfo->type == HAL_PIN_TYPE_DEMUX) {
        // TODO
    }
#endif
#endif
}

int32_t hal_gpio_read(hal_pin_t pin) {
    if (!hal_pin_is_valid(pin)) {
        return 0;
    }

    hal_pin_info_t* pinInfo = hal_pin_map() + pin;

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    if (pinInfo->type == HAL_PIN_TYPE_MCU) {
#endif
        uint32_t rtlPin = hal_pin_to_rtl_pin(pin);

        // TODO: verify reading output
        if ((pinInfo->pin_mode == INPUT) ||
            (pinInfo->pin_mode == INPUT_PULLUP) ||
            (pinInfo->pin_mode == INPUT_PULLDOWN) ||
            (pinInfo->pin_mode == OUTPUT) ||
            (pinInfo->pin_mode == OUTPUT_OPEN_DRAIN) ||
            (pinInfo->pin_mode == OUTPUT_OPEN_DRAIN_PULLUP)) {
            return GPIO_ReadDataBit(rtlPin);
        } else {
            return 0;
        }
#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    }
#if HAL_PLATFORM_MCP23S17
    else if (pinInfo->type == HAL_PIN_TYPE_IO_EXPANDER) {
        // TODO
        return 0;
    }
#endif
#if HAL_PLATFORM_DEMUX
    else if (pinInfo->type == HAL_PIN_TYPE_DEMUX) {
        // TODO
        return 0
    }
#endif
    else {
        return 0;
    }
#endif
    return 0;
}

uint32_t hal_gpio_pulse_in(hal_pin_t pin, uint16_t value) {
    static const unsigned long THREE_SECONDS_IN_MICROSECONDS = 3000000;
    #define FAST_READ(pin) ((gpiobase->EXT_PORT[0] >> pin) & 1UL)

    // TODO: FIX DEBUG RETURN VALUES
    if (!hal_pin_is_valid(pin)) {
        return 1;
    }

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    if (PIN_MAP[pin].type == HAL_PIN_TYPE_MCU) {
#endif
        hal_pin_info_t* pin_info = hal_pin_map() + pin;
        uint32_t rtlPin = pin_info->gpio_pin;
        GPIO_TypeDef * gpiobase = ((pin_info->gpio_port == RTL_PORT_A) ? GPIOA_BASE : GPIOB_BASE);

        volatile uint64_t timeout_start = hal_timer_micros(nullptr);

        /* If already on the value we want to measure, wait for the next one.
        * Time out after 3 seconds so we don't block the background tasks
        */
        while (FAST_READ(rtlPin) == value) {
            if (hal_timer_micros(nullptr) - timeout_start > THREE_SECONDS_IN_MICROSECONDS) {
                return 2;
            }
        }

        /* Wait until the start of the pulse.
        * Time out after 3 seconds so we don't block the background tasks
        */
        while (FAST_READ(rtlPin) != value) {
            if (hal_timer_micros(nullptr) - timeout_start > THREE_SECONDS_IN_MICROSECONDS) {
                return 3;
            }
        }

        /* Wait until this value changes, this will be our elapsed pulse width.
        * Time out after 3 seconds so we don't block the background tasks
        */
        volatile uint32_t pulse_start = hal_timer_micros(nullptr);
        while (FAST_READ(rtlPin) == value) {
            if (hal_timer_micros(nullptr) - timeout_start > THREE_SECONDS_IN_MICROSECONDS) {
                return 4;
            }
        }

        return (hal_timer_micros(nullptr) - pulse_start);
#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    } else {
        return 5;
    }
#endif
}
