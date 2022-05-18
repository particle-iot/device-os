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

__attribute__((always_inline)) inline const Hal_Pin_Info* fastPinGetPinmap() {
    static const Hal_Pin_Info* pinMap = HAL_Pin_Map();
    return pinMap;
}

#if HAL_PLATFORM_NRF52840

#include "nrf_gpio.h"
#include "pinmap_impl.h"

inline void pinSetFast(pin_t _pin) __attribute__((always_inline));
inline void pinResetFast(pin_t _pin) __attribute__((always_inline));
inline int32_t pinReadFast(pin_t _pin) __attribute__((always_inline));

inline void pinSetFast(pin_t _pin)
{
    uint32_t nrf_pin = NRF_GPIO_PIN_MAP(fastPinGetPinmap()[_pin].gpio_port, fastPinGetPinmap()[_pin].gpio_pin);
    nrf_gpio_pin_set(nrf_pin);
}

inline void pinResetFast(pin_t _pin)
{
    uint32_t nrf_pin = NRF_GPIO_PIN_MAP(fastPinGetPinmap()[_pin].gpio_port, fastPinGetPinmap()[_pin].gpio_pin);
    nrf_gpio_pin_clear(nrf_pin);
}

inline int32_t pinReadFast(pin_t _pin)
{
    uint32_t nrf_pin = NRF_GPIO_PIN_MAP(fastPinGetPinmap()[_pin].gpio_port, fastPinGetPinmap()[_pin].gpio_pin);
    // Dummy read is needed because peripherals run at 16 MHz while the CPU runs at 64 MHz.
    (void)nrf_gpio_pin_read(nrf_pin);
    return nrf_gpio_pin_read(nrf_pin);
}

#elif PLATFORM_ID == PLATFORM_GCC

// make them unresolved symbols so attempted use will result in a linker error
void pinResetFast(pin_t _pin);
void pinSetFast(pin_t _pin);
void pinReadFast(pin_t _pin);

#elif PLATFORM_ID == PLATFORM_NEWHAL

    // no need to generate a warning for newhal
    #define pinSetFast(pin) digitalWrite(pin, HIGH)
    #define pinResetFast(pin) digitalWrite(pin, LOW)

#else

    #warning "*** MCU architecture not supported by the fastPin library. ***"
    #define pinSetFast(pin) digitalWrite(pin, HIGH)
    #define pinResetFast(pin) digitalWrite(pin, LOW)

#endif // HAL_PLATFORM_NRF52840

inline void digitalWriteFast(pin_t pin, uint8_t value)
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
