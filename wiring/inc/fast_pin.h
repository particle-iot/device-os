/*
 ******************************************************************************
 *  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
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
 ******************************************************************************
 */


#ifndef FAST_PIN_H
#define	FAST_PIN_H

#include "platforms.h"
#include "pinmap_hal.h"

#ifdef	__cplusplus
extern "C" {
#endif

__attribute__((always_inline)) inline const hal_pin_info_t* fastPinGetPinmap() {
    static const hal_pin_info_t* pinMap = hal_pin_map();
    return pinMap;
}

#if HAL_PLATFORM_NRF52840

#include "nrf_gpio.h"
#include "pinmap_impl.h"

inline void pinSetFast(hal_pin_t _pin) __attribute__((always_inline));
inline void pinResetFast(hal_pin_t _pin) __attribute__((always_inline));
inline int32_t pinReadFast(hal_pin_t _pin) __attribute__((always_inline));

inline void pinSetFast(hal_pin_t _pin)
{
    uint32_t nrf_pin = NRF_GPIO_PIN_MAP(fastPinGetPinmap()[_pin].gpio_port, fastPinGetPinmap()[_pin].gpio_pin);
    nrf_gpio_pin_set(nrf_pin);
}

inline void pinResetFast(hal_pin_t _pin)
{
    uint32_t nrf_pin = NRF_GPIO_PIN_MAP(fastPinGetPinmap()[_pin].gpio_port, fastPinGetPinmap()[_pin].gpio_pin);
    nrf_gpio_pin_clear(nrf_pin);
}

inline int32_t pinReadFast(hal_pin_t _pin)
{
    uint32_t nrf_pin = NRF_GPIO_PIN_MAP(fastPinGetPinmap()[_pin].gpio_port, fastPinGetPinmap()[_pin].gpio_pin);
    // Dummy read is needed because peripherals run at 16 MHz while the CPU runs at 64 MHz.
    (void)nrf_gpio_pin_read(nrf_pin);
    return nrf_gpio_pin_read(nrf_pin);
}

#elif HAL_PLATFORM_RTL872X

inline void pinSetFast(hal_pin_t _pin) __attribute__((always_inline));
inline void pinResetFast(hal_pin_t _pin) __attribute__((always_inline));
inline int32_t pinReadFast(hal_pin_t _pin) __attribute__((always_inline));

inline void pinSetFast(hal_pin_t _pin)
{
    hal_pin_info_t pin_info = fastPinGetPinmap()[_pin];

    GPIO_TypeDef* gpiobase = ((pin_info.gpio_port == RTL_PORT_A) ? GPIOA_BASE : GPIOB_BASE);
    if (pin_info.pin_mode == OUTPUT_OPEN_DRAIN || pin_info.pin_mode == OUTPUT_OPEN_DRAIN_PULLUP) {
        gpiobase->PORT[0].DDR &= (~(1 << pin_info.gpio_pin));
    } else {
        gpiobase->PORT[0].DR |= (1 << pin_info.gpio_pin);
        gpiobase->PORT[0].DDR |= (1 << pin_info.gpio_pin);
    }
}

inline void pinResetFast(hal_pin_t _pin)
{
    hal_pin_info_t pin_info = fastPinGetPinmap()[_pin];

    GPIO_TypeDef* gpiobase = ((pin_info.gpio_port == RTL_PORT_A) ? GPIOA_BASE : GPIOB_BASE);
    gpiobase->PORT[0].DR &= ~(1 << pin_info.gpio_pin);
    gpiobase->PORT[0].DDR |= (1 << pin_info.gpio_pin);
}

inline int32_t pinReadFast(hal_pin_t _pin)
{
    hal_pin_info_t pin_info = fastPinGetPinmap()[_pin];

    GPIO_TypeDef* gpiobase = ((pin_info.gpio_port == RTL_PORT_A) ? GPIOA_BASE : GPIOB_BASE);
    return ((gpiobase->EXT_PORT[0] >> pin_info.gpio_pin) & 1UL);
}

#elif PLATFORM_ID == PLATFORM_GCC

// make them unresolved symbols so attempted use will result in a linker error
void pinResetFast(hal_pin_t _pin);
void pinSetFast(hal_pin_t _pin);
void pinReadFast(hal_pin_t _pin);

#elif PLATFORM_ID == PLATFORM_NEWHAL

    // no need to generate a warning for newhal
    #define pinSetFast(pin) digitalWrite(pin, HIGH)
    #define pinResetFast(pin) digitalWrite(pin, LOW)

#else

    #warning "*** MCU architecture not supported by the fastPin library. ***"
    #define pinSetFast(pin) digitalWrite(pin, HIGH)
    #define pinResetFast(pin) digitalWrite(pin, LOW)

#endif

inline void digitalWriteFast(hal_pin_t pin, uint8_t value)
{
    if (value)
        pinSetFast(pin);
    else
        pinResetFast(pin);
}

#ifdef	__cplusplus
}
#endif

#endif	/* FAST_PIN_H */
