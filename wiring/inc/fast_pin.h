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

/* Disabling USE_BIT_BAND since bitbanding is much slower! as per comment
 * by @pkourany on PR: https://github.com/spark/firmware/pull/556 */
#define USE_BIT_BAND 0

__attribute__((always_inline)) inline const hal_pin_info_t* fastPinGetPinmap() {
    static const hal_pin_info_t* pinMap = hal_pin_map();
    return pinMap;
}

#if USE_BIT_BAND
/* Use CortexM3 Bit-Band access to perform GPIO atomic read-modify-write */

//Below is defined in stm32fxxx.h
//PERIPH_BASE     = 0x40000000 /* Peripheral base address in the alias region */
//PERIPH_BB_BASE  = 0x42000000 /* Peripheral base address in the bit-band region */

/* A mapping formula shows how to reference each word in the alias region to a
   corresponding bit in the bit-band region. The mapping formula is:
   bit_word_addr = bit_band_base + (byte_offset x 32) + (bit_number + 4)

where:
   - bit_word_addr: is the address of the word in the alias memory region that
                    maps to the targeted bit.
   - bit_band_base is the starting address of the alias region
   - byte_offset is the number of the byte in the bit-band region that contains
     the targeted bit
   - bit_number is the bit position of the targeted bit
*/

#define  GPIO_ResetBit_BB(Addr, Bit)    \
          (*(__IO uint32_t *) (PERIPH_BB_BASE | ((Addr - PERIPH_BASE) << 5) | ((Bit) << 2)) = 0)

#define GPIO_SetBit_BB(Addr, Bit)       \
          (*(__IO uint32_t *) (PERIPH_BB_BASE | ((Addr - PERIPH_BASE) << 5) | ((Bit) << 2)) = 1)

#define GPIO_GetBit_BB(Addr, Bit)       \
          (*(__IO uint32_t *) (PERIPH_BB_BASE | ((Addr - PERIPH_BASE) << 5) | ((Bit) << 2)))


inline void pinSetFast(hal_pin_t _pin) __attribute__((always_inline));
inline void pinResetFast(hal_pin_t _pin) __attribute__((always_inline));
inline int32_t pinReadFast(hal_pin_t _pin) __attribute__((always_inline));

inline void pinSetFast(hal_pin_t _pin)
{
    GPIO_SetBit_BB((__IO uint32_t)&fastPinGetPinmap()[_pin].gpio_peripheral->ODR, fastPinGetPinmap()[_pin].gpio_pin_source);
}

inline void pinResetFast(hal_pin_t _pin)
{
    GPIO_ResetBit_BB((__IO uint32_t)&fastPinGetPinmap()[_pin].gpio_peripheral->ODR, fastPinGetPinmap()[_pin].gpio_pin_source);
}

inline int32_t pinReadFast(hal_pin_t _pin)
{
    return GPIO_GetBit_BB((__IO uint32_t)&fastPinGetPinmap()[_pin].gpio_peripheral->IDR, fastPinGetPinmap()[_pin].gpio_pin_source);
}

#else

#ifndef STM32F10X
    #if defined(STM32F10X_MD) || defined(STM32F10X_HD)
        #define STM32F10X
    #endif
#endif

#ifdef STM32F10X

inline void pinSetFast(hal_pin_t _pin) __attribute__((always_inline));
inline void pinResetFast(hal_pin_t _pin) __attribute__((always_inline));
inline int32_t pinReadFast(hal_pin_t _pin) __attribute__((always_inline));

inline void pinSetFast(hal_pin_t _pin)
{
    fastPinGetPinmap()[_pin].gpio_peripheral->BSRR = fastPinGetPinmap()[_pin].gpio_pin;
}

inline void pinResetFast(hal_pin_t _pin)
{
    fastPinGetPinmap()[_pin].gpio_peripheral->BRR = fastPinGetPinmap()[_pin].gpio_pin;
}

inline int32_t pinReadFast(hal_pin_t _pin)
{
    return ((fastPinGetPinmap()[_pin].gpio_peripheral->IDR & fastPinGetPinmap()[_pin].gpio_pin) == 0 ? LOW : HIGH);
}
#elif defined(STM32F2XX)

inline void pinSetFast(hal_pin_t _pin) __attribute__((always_inline));
inline void pinResetFast(hal_pin_t _pin) __attribute__((always_inline));
inline int32_t pinReadFast(hal_pin_t _pin) __attribute__((always_inline));

inline void pinSetFast(hal_pin_t _pin)
{
    fastPinGetPinmap()[_pin].gpio_peripheral->BSRRL = fastPinGetPinmap()[_pin].gpio_pin;
}

inline void pinResetFast(hal_pin_t _pin)
{
    fastPinGetPinmap()[_pin].gpio_peripheral->BSRRH = fastPinGetPinmap()[_pin].gpio_pin;
}

inline int32_t pinReadFast(hal_pin_t _pin)
{
	return ((fastPinGetPinmap()[_pin].gpio_peripheral->IDR & fastPinGetPinmap()[_pin].gpio_pin) == 0 ? LOW : HIGH);
}
#elif HAL_PLATFORM_NRF52840

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

inline void pinConfigure(hal_pin_info_t _pin){
    int padMuxIndex = (32 * _pin.gpio_port) + _pin.gpio_pin;
    uint32_t Temp = PINMUX->PADCTR[padMuxIndex];

    Temp &= ~PAD_BIT_MASK_FUNCTION_ID;
    Temp |= (PINMUX_FUNCTION_GPIO & PAD_BIT_MASK_FUNCTION_ID); 
    Temp &= ~PAD_BIT_SHUT_DWON;
     
    PINMUX->PADCTR[padMuxIndex] = Temp; 
}

inline void pinSetFast(hal_pin_t _pin)
{
    hal_pin_info_t pin_info = fastPinGetPinmap()[_pin];
    pinConfigure(pin_info);

    GPIO_TypeDef* gpiobase = ((pin_info.gpio_port == RTL_PORT_A) ? GPIOA_BASE : GPIOB_BASE);
    gpiobase->PORT[0].DDR |= (1 << pin_info.gpio_pin);
    gpiobase->PORT[0].DR |= (1 << pin_info.gpio_pin);
}

inline void pinResetFast(hal_pin_t _pin)
{
    hal_pin_info_t pin_info = fastPinGetPinmap()[_pin];
    pinConfigure(pin_info);

    GPIO_TypeDef* gpiobase = ((pin_info.gpio_port == RTL_PORT_A) ? GPIOA_BASE : GPIOB_BASE);
    gpiobase->PORT[0].DDR |= (1 << pin_info.gpio_pin);
    gpiobase->PORT[0].DR &= (0 << pin_info.gpio_pin);
}

inline int32_t pinReadFast(hal_pin_t _pin)
{
    hal_pin_info_t pin_info = fastPinGetPinmap()[_pin];
    pinConfigure(pin_info);
    
    GPIO_TypeDef* gpiobase = ((pin_info.gpio_port == RTL_PORT_A) ? GPIOA_BASE : GPIOB_BASE);
    return ((gpiobase->EXT_PORT[0] >> pin_info.gpio_pin) & 1UL);
}

#elif PLATFORM_ID==3 || PLATFORM_ID == 20

// make them unresolved symbols so attempted use will result in a linker error
void pinResetFast(hal_pin_t _pin);
void pinSetFast(hal_pin_t _pin);
void pinReadFast(hal_pin_t _pin);
#elif PLATFORM_ID==PLATFORM_NEWHAL
    // no need to generate a warning for newhal
    #define pinSetFast(pin) digitalWrite(pin, HIGH)
    #define pinResetFast(pin) digitalWrite(pin, LOW)
#else
    #warning "*** MCU architecture not supported by the fastPin library. ***"
    #define pinSetFast(pin) digitalWrite(pin, HIGH)
    #define pinResetFast(pin) digitalWrite(pin, LOW)
#endif

#endif //USE_BIT_BAND

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

