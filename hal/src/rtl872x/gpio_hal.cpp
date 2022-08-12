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

namespace {

// When setting to GPIO output mode, all of the audio pins and one of the
// normal pins (PA[27]) do not support GPIO read function (always read as '0'),
// We'll cache the GPIO state for these pins.
constexpr uint32_t CACHE_PIN_STATE_UNKNOWN = 0x3;
constexpr uint32_t CACHE_PIN_STATE_MASK = 0x3;
constexpr int CACHE_PIN_STATE_BITS = 2;
constexpr int CACHE_PIN_COUNT = 6;
hal_pin_t cachePins[CACHE_PIN_COUNT] = {D7, S4, S5, S6, BTN, ANTSW};
// 2-bit state for each pin: | 00 | 00 | 00 | 00 | 00 | 00 |
uint32_t cachePinState = 0;

void setCachePinState(hal_pin_t pin, uint8_t value) {
    for (int i = 0; i < CACHE_PIN_COUNT; i++) {
        if (cachePins[i] == pin) {
            cachePinState &= ~(CACHE_PIN_STATE_MASK << (i * CACHE_PIN_STATE_BITS));
            cachePinState |= value << (i * CACHE_PIN_STATE_BITS);
            break;
        }
    }
}

uint8_t getCachePinState(hal_pin_t pin) {
    for (int i = 0; i < CACHE_PIN_COUNT; i++) {
        if (cachePins[i] == pin) {
            return (cachePinState >> (i * CACHE_PIN_STATE_BITS)) & CACHE_PIN_STATE_MASK;
        }
    }
    return CACHE_PIN_STATE_UNKNOWN;
}

bool isCachePin(hal_pin_t pin) {
    for (int i = 0; i < CACHE_PIN_COUNT; i++) {
        if (cachePins[i] == pin) {
            return true;
        }
    }
    return false;
}

bool isGpioPin(hal_pin_t pin) {
    hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    return(pinInfo->gpio_port != RTL_PORT_NONE) ? true : false;
}

bool isCachePinSetToOutput(hal_pin_t pin) {
    hal_pin_info_t* pinInfo = hal_pin_map() + pin;
    if ((pinInfo->pin_mode == OUTPUT) ||
        (pinInfo->pin_mode == OUTPUT_OPEN_DRAIN) ||
        (pinInfo->pin_mode == OUTPUT_OPEN_DRAIN_PULLUP)) {
        return true;
    }
    return false;
}

// RTL872XD has two SWD ports. Default is PA27, but PB3 can be configured as alternate SWD as well
bool isSwdPin(hal_pin_info_t* pinInfo){
    if ((pinInfo->gpio_port == RTL_PORT_A && pinInfo->gpio_pin == 27) ||
        (pinInfo->gpio_port == RTL_PORT_B && pinInfo->gpio_pin == 3)) {
        return true;
    }
    return false;
}

} // Anonymous


void hal_gpio_mode(hal_pin_t pin, PinMode mode) {
    hal_gpio_config_t conf = {};
    conf.size = sizeof(conf);
    conf.version = HAL_GPIO_VERSION;
    conf.mode = mode;
    hal_gpio_configure(pin, &conf, nullptr);
}

int hal_gpio_configure(hal_pin_t pin, const hal_gpio_config_t* conf, void* reserved) {
    CHECK_TRUE(hal_pin_is_valid(pin), SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(isGpioPin(pin), SYSTEM_ERROR_NOT_SUPPORTED);
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

        if (isSwdPin(pinInfo)) {
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
            case OUTPUT: {
                GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
                GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
                break;
            }
            case INPUT:
            case OUTPUT_OPEN_DRAIN: {
                GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
                GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
                break;
            }
            case INPUT_PULLUP:
            case OUTPUT_OPEN_DRAIN_PULLUP: {
                GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
                GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
                break;
            }
            case INPUT_PULLDOWN: {
                GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
                GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
                break;
            }
            case PIN_MODE_SWD: {
                if (isSwdPin(pinInfo)) {
                    //"Pinmux_Swdon"
                    u32 Temp = 0;
                    Temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SWD_PMUX_EN);
                    Temp |= (BIT_LSYS_SWD_PMUX_EN);
                    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_SWD_PMUX_EN, Temp);
                }
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

        if (isCachePin(pin) && isCachePinSetToOutput(pin)) {
            if (conf->set_value) {
                setCachePinState(pin, conf->value);
            } else {
                setCachePinState(pin, CACHE_PIN_STATE_UNKNOWN);
            }
        }

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
    if (!hal_pin_is_valid(pin) || !isGpioPin(pin)) {
        return;
    }

    hal_pin_info_t* pinInfo = hal_pin_map() + pin;

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    if (pinInfo->type == HAL_PIN_TYPE_MCU) {
#endif
        // TODO: PWM have conflict with GPIO OUTPUT mode on Realtek
        if (pinInfo->pin_func == PF_PWM) {
            hal_gpio_mode(pin, OUTPUT);
        }

        auto mode = hal_gpio_get_mode(pin);
        hal_pin_info_t* pin_info = hal_pin_map() + pin;
        GPIO_TypeDef * gpiobase = ((pin_info->gpio_port == RTL_PORT_A) ? GPIOA_BASE : GPIOB_BASE);
        // Dirty hack: As per the description of EXT_PORT register in the user manual,
        // "When Port A is configured as Input, then reading this location reads
        // the values on the signal. When the data direction of Port A is set as
        // Output, reading this location reads the data register for Port A."
        if (value) {
            if (mode == OUTPUT_OPEN_DRAIN || mode == OUTPUT_OPEN_DRAIN_PULLUP) {
                // Output 1, it should read back 1 if idle or 0 if it is pulled down by external signal
                // Configure it as input meets the requirements.
                gpiobase->PORT[0].DDR &= (~(1 << pin_info->gpio_pin));
            } else {
                gpiobase->PORT[0].DR |= (1 << pin_info->gpio_pin);
            }
        } else {
            // Output 0, it should always read back 0.
            // Configure it as output, despite of the output mode
            gpiobase->PORT[0].DR &= ~(1 << pin_info->gpio_pin);
            gpiobase->PORT[0].DDR |= (1 << pin_info->gpio_pin);
        }

        if (isCachePin(pin) && isCachePinSetToOutput(pin)) {
            setCachePinState(pin, value);
        }
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
    if (!hal_pin_is_valid(pin) || !isGpioPin(pin)) {
        return 0;
    }

    if (isCachePin(pin) && isCachePinSetToOutput(pin)) {
        uint8_t state = getCachePinState(pin);
        return (state == CACHE_PIN_STATE_UNKNOWN) ? 0 : state;
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
    const uint64_t THREE_SECONDS_IN_MICROSECONDS = 3000000;
    #define FAST_READ(pin) ((gpiobase->EXT_PORT[0] >> pin) & 1UL)

    if (!hal_pin_is_valid(pin) || !isGpioPin(pin)) {
        return 0;
    }

    hal_pin_info_t* pin_info = hal_pin_map() + pin;
    uint32_t rtlPin = pin_info->gpio_pin;
    GPIO_TypeDef * gpiobase = ((pin_info->gpio_port == RTL_PORT_A) ? GPIOA_BASE : GPIOB_BASE);

    volatile uint64_t timeout_start = hal_timer_micros(nullptr);

    /* If already on the value we want to measure, wait for the next one.
    * Time out after 3 seconds so we don't block the background tasks
    */
    while (FAST_READ(rtlPin) == value) {
        if (hal_timer_micros(nullptr) - timeout_start > THREE_SECONDS_IN_MICROSECONDS) {
            return 0;
        }
    }

    /* Wait until the start of the pulse.
    * Time out after 3 seconds so we don't block the background tasks
    */
    while (FAST_READ(rtlPin) != value) {
        if (hal_timer_micros(nullptr) - timeout_start > THREE_SECONDS_IN_MICROSECONDS) {
            return 0;
        }
    }

    /* Wait until this value changes, this will be our elapsed pulse width.
    * Time out after 3 seconds so we don't block the background tasks
    */
    volatile uint64_t pulse_start = hal_timer_micros(nullptr);
    while (FAST_READ(rtlPin) == value) {
        if (hal_timer_micros(nullptr) - timeout_start > THREE_SECONDS_IN_MICROSECONDS) {
            return 0;
        }
    }

    return (hal_timer_micros(nullptr) - pulse_start);
}
