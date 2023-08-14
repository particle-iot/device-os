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
#include "module_info.h"

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
#if HAL_PLATFORM_MCP23S17
#include "mcp23s17.h"
#endif
#if HAL_PLATFORM_DEMUX
#include "demux.h"
#endif
using namespace particle;
#endif

namespace {

// When setting to GPIO output mode, all of the audio pins (PA[0] ~ PA[6] and PB[28] ~ PB[31])
// and one of the normal pins (PA[27]) do not support GPIO read function (always read as '0'),
// We'll cache the GPIO state for these pins.
#if PLATFORM_ID == PLATFORM_P2 || PLATFORM_ID == PLATFORM_TRACKERM
constexpr int CACHE_PIN_COUNT = 6;
hal_pin_t cachePins[CACHE_PIN_COUNT] = {D7, S4, S5, S6, BTN, ANTSW};
#elif PLATFORM_ID == PLATFORM_MSOM
constexpr int CACHE_PIN_COUNT = 10;
hal_pin_t cachePins[CACHE_PIN_COUNT] = {D20, D21, D26, BGPWR, BGRST, BGDTR, BGVINT, GNSS_ANT_PWR, UNUSED_PIN1, UNUSED_PIN2};
#endif
constexpr uint32_t CACHE_PIN_STATE_UNKNOWN = 0x3;
constexpr uint32_t CACHE_PIN_STATE_MASK = 0x3;
constexpr int CACHE_PIN_STATE_BITS = 2;
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
bool isSwdPin(hal_pin_t pin){
    if (pin == SWD_DAT || pin == SWD_CLK) {
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
    conf.drive_strength = HAL_GPIO_DRIVE_DEFAULT;
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

        if (isSwdPin(pin)) {
            Pinmux_Swdoff();
            // Configure the another SWD pin as input float if it is not configured yet.
            if (pin == SWD_CLK) {
                hal_pin_info_t* info = hal_pin_map() + SWD_DAT;
                if (info->pin_func == PF_NONE) {
                    Pinmux_Config(hal_pin_to_rtl_pin(SWD_DAT), PINMUX_FUNCTION_GPIO);
                    GPIOA_BASE->PORT[0].DDR &= (~(1 << 27));
                    PAD_PullCtrl(hal_pin_to_rtl_pin(SWD_DAT), GPIO_PuPd_NOPULL);
                }
            } else {
                hal_pin_info_t* info = hal_pin_map() + SWD_CLK;
                if (info->pin_func == PF_NONE) {
                    Pinmux_Config(hal_pin_to_rtl_pin(SWD_CLK), PINMUX_FUNCTION_GPIO);
                    GPIOB_BASE->PORT[0].DDR &= (~(1 << 3));
                    PAD_PullCtrl(hal_pin_to_rtl_pin(SWD_CLK), GPIO_PuPd_NOPULL);
                }
            }
        }

        // Set pin function may reset nordic gpio configuration, should be called before the re-configuration
        if (mode != PIN_MODE_NONE) {
            hal_pin_set_function(pin, PF_DIO);
        } else {
            hal_pin_set_function(pin, PF_NONE);
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
                if (isSwdPin(pin)) {
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

        if ((mode == OUTPUT_OPEN_DRAIN || mode == OUTPUT_OPEN_DRAIN_PULLUP) && conf->set_value && conf->value == 0) {
            GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
        }

        // The GPIO_Init() doesn't seam to set the pull ability when the pin is configured as output
        if (GPIO_InitStruct.GPIO_Mode == GPIO_Mode_OUT) {
            if (mode == OUTPUT_OPEN_DRAIN_PULLUP) {
                PAD_PullCtrl(rtlPin, GPIO_PuPd_UP);
            } else {
                PAD_PullCtrl(rtlPin, GPIO_PuPd_NOPULL);
            }
        }

        // Pre-set the output value if requested to avoid a glitch
        if (conf->set_value && (mode == OUTPUT || mode == OUTPUT_OPEN_DRAIN || mode == OUTPUT_OPEN_DRAIN_PULLUP)) {
            GPIO_WriteBit(rtlPin, conf->value);
        }

        GPIO_Init(&GPIO_InitStruct);
        pinInfo->pin_mode = mode;

        Pinmux_Config(hal_pin_to_rtl_pin(pin), PINMUX_FUNCTION_GPIO);
        hal_gpio_set_drive_strength(pin, static_cast<hal_gpio_drive_t>(conf->drive_strength));

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
        CHECK_TRUE(mode == INPUT || mode == INPUT_PULLUP || mode == OUTPUT, SYSTEM_ERROR_INVALID_ARGUMENT);

        // Set pin function may reset nordic gpio configuration, should be called before the re-configuration
        if (mode != PIN_MODE_NONE) {
            hal_pin_set_function(pin, PF_DIO);
        } else {
            hal_pin_set_function(pin, PF_NONE);
        }

        // Pre-set the output value if requested to avoid a glitch
        if (conf->set_value && mode == OUTPUT) {
            CHECK(Mcp23s17::getInstance().writePinValue(pinInfo->gpio_port, pinInfo->gpio_pin, conf->value));
        }

        CHECK(Mcp23s17::getInstance().setPinMode(pinInfo->gpio_port, pinInfo->gpio_pin, mode));
        pinInfo->pin_mode = mode;
    }
#endif // HAL_PLATFORM_MCP23S17
#if HAL_PLATFORM_DEMUX
    else if (pinInfo->type == HAL_PIN_TYPE_DEMUX) {
        CHECK_TRUE(mode == OUTPUT, SYSTEM_ERROR_INVALID_ARGUMENT);
        if (conf->set_value) {
            Demux::getInstance().write(pinInfo->gpio_pin, conf->value);
        }
        pinInfo->pin_mode = mode;
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
        Mcp23s17::getInstance().writePinValue(pinInfo->gpio_port, pinInfo->gpio_pin, value);
    }
#endif
#if HAL_PLATFORM_DEMUX
    else if (pinInfo->type == HAL_PIN_TYPE_DEMUX) {
        Demux::getInstance().write(pinInfo->gpio_pin, value);
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
        if ((pinInfo->pin_mode == INPUT) ||
            (pinInfo->pin_mode == INPUT_PULLUP) ||
            (pinInfo->pin_mode == OUTPUT)) {
            uint8_t value = 0x00;
            Mcp23s17::getInstance().readPinValue(pinInfo->gpio_port, pinInfo->gpio_pin, &value);
            return value;
        } else {
            return 0;
        }
    }
#endif
#if HAL_PLATFORM_DEMUX
    else if (pinInfo->type == HAL_PIN_TYPE_DEMUX) {
        return Demux::getInstance().read(pinInfo->gpio_pin);
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

int hal_gpio_get_drive_strength(hal_pin_t pin, hal_gpio_drive_t* drive) {
    uint32_t rtlPin = hal_pin_to_rtl_pin(pin);

    /* get PADCTR */
    uint32_t drvStrength = PINMUX->PADCTR[rtlPin];

    /* get Pin_Num drvStrength contrl */
    drvStrength &= PAD_BIT_MASK_DRIVING_STRENGTH << PAD_BIT_SHIFT_DRIVING_STRENGTH;
    drvStrength >>= PAD_BIT_SHIFT_DRIVING_STRENGTH;

    // Pad driving strength
    //   - PAD_DRV_STRENGTH_0: 4mA
    //   - PAD_DRV_STRENGTH_1: 8mA   (Severe overshoot, not recommended to use it)
    //   - PAD_DRV_STRENGTH_2: 12mA
    //   - PAD_DRV_STRENGTH_3: 16mA  (Severe overshoot, not recommended to use it)
    switch(drvStrength) {
        case PAD_DRV_STRENGTH_0: *drive = HAL_GPIO_DRIVE_STANDARD; break;
        case PAD_DRV_STRENGTH_2: *drive = HAL_GPIO_DRIVE_HIGH; break;
        // DVOS won't configure PAD_DRV_STRENGTH_1 and PAD_DRV_STRENGTH_3 due to the
        // severe overshoot, so it is the default setting
        default:
            *drive = HAL_GPIO_DRIVE_DEFAULT; break;
    }

    return SYSTEM_ERROR_NONE;
}

int hal_gpio_set_drive_strength(hal_pin_t pin, hal_gpio_drive_t drive) {
    uint32_t rtlPin = hal_pin_to_rtl_pin(pin);
    uint32_t drvStrength = PAD_DRV_STRENGTH_0;

    // Pad driving strength
    //   - PAD_DRV_STRENGTH_0: 4mA
    //   - PAD_DRV_STRENGTH_1: 8mA   (Severe overshoot, not recommended to use it)
    //   - PAD_DRV_STRENGTH_2: 12mA
    //   - PAD_DRV_STRENGTH_3: 16mA  (Severe overshoot, not recommended to use it)
    switch(drive) {
        case HAL_GPIO_DRIVE_STANDARD: drvStrength = PAD_DRV_STRENGTH_0; break;
        case HAL_GPIO_DRIVE_HIGH:     drvStrength = PAD_DRV_STRENGTH_2; break;
        case HAL_GPIO_DRIVE_DEFAULT:  drvStrength = PAD_DRV_STRENGTH_3; break;
        default:
            return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    uint32_t temp = 0;

    /* get PADCTR */
    temp = PINMUX->PADCTR[rtlPin];

    /* clear Pin_Num drvStrength contrl */
    temp &= ~(PAD_BIT_MASK_DRIVING_STRENGTH << PAD_BIT_SHIFT_DRIVING_STRENGTH);

    /* set needs drvStrength */
    temp |= (drvStrength & PAD_BIT_MASK_DRIVING_STRENGTH) << PAD_BIT_SHIFT_DRIVING_STRENGTH;

    /* set PADCTR register */
    PINMUX->PADCTR[rtlPin] = temp;

    return SYSTEM_ERROR_NONE;
}
