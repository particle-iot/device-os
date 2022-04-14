/**
 * \file
 *
 * \brief GPIO driver.
 *
 * Copyright (c) 2012 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */
#ifndef GPIO_H_INCLUDED
#define GPIO_H_INCLUDED

#include "compiler.h"
#include "ioport.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup group_sam_drivers_gpio GPIO - General-Purpose Input/Output
 *
 * This is a driver for advanced GPIO functions of on-chip GPIO controller:
 * interrupt and event support.
 *
 * \section dependencies Dependencies
 *
 * The GPIO module depends on the following modules:
 * - \ref sysclk_group for clock control.
 * - \ref interrupt_group for enabling or disabling interrupts.
 * - \ref ioport_group for basic GPIO functions.
 * @{
 */

/**
 * \brief Interrupt callback function type for a GPIO pin.
 *
 * The interrupt handler can be configured to do a function callback,
 * the callback function must match the gpio_pin_callback_t type.
 *
 */
typedef void (*gpio_pin_callback_t)(void);

/**
 * \name Interrupt Support
 *
 * The GPIO can be configured to generate an interrupt when it detects a
 * change on a GPIO pin.
 *
 * @{
 */

bool gpio_set_pin_callback(ioport_pin_t pin, gpio_pin_callback_t callback,
		uint8_t irq_level);

/**
 * \brief Enable the interrupt of a pin.
 *
 * \param pin The pin number.
 */
static inline void gpio_enable_pin_interrupt(ioport_pin_t pin)
{
	GpioPort *gpio_port = &(GPIO->GPIO_PORT[ioport_pin_to_port_id(pin)]);
	gpio_port->GPIO_IERS = ioport_pin_to_mask(pin);
}

/**
 * \brief Disable the interrupt of a pin.
 *
 * \param pin The pin number.
 */
static inline void gpio_disable_pin_interrupt(ioport_pin_t pin)
{
	GpioPort *gpio_port = &(GPIO->GPIO_PORT[ioport_pin_to_port_id(pin)]);
	gpio_port->GPIO_IERC = ioport_pin_to_mask(pin);
}

/**
 * \brief Get the interrupt flag of a pin.
 *
 * \param pin The pin number.
 *
 * \return The pin interrupt flag (0/1).
 */
static inline uint32_t gpio_get_pin_interrupt_flag(ioport_pin_t pin)
{
	GpioPort *gpio_port = &(GPIO->GPIO_PORT[ioport_pin_to_port_id(pin)]);
	return (((gpio_port->GPIO_IFR && ioport_pin_to_mask(pin)) == 0) ? 0 : 1);
}

/**
 * \brief Clear the interrupt flag of a pin.
 *
 * \param pin The pin number.
 */
static inline void gpio_clear_pin_interrupt_flag(ioport_pin_t pin)
{
	GpioPort *gpio_port = &(GPIO->GPIO_PORT[ioport_pin_to_port_id(pin)]);
	gpio_port->GPIO_IFRC = ioport_pin_to_mask(pin);
}

/** @} */

/**
 * \name Peripheral Event System Support
 *
 * The GPIO can be programmed to output peripheral events whenever an interrupt
 * condition is detected, such as pin value change, or only when a rising or
 * falling edge is detected.
 *
 * @{
 */

/**
 * \brief Enable the peripheral event generation of a pin.
 *
 * \param pin The pin number.
 */
static inline void gpio_enable_pin_periph_event(ioport_pin_t pin)
{
	GpioPort *gpio_port = &(GPIO->GPIO_PORT[ioport_pin_to_port_id(pin)]);
	gpio_port->GPIO_EVERS = ioport_pin_to_mask(pin);
}

/**
 * \brief Disable the peripheral event generation of a pin.
 *
 * \param pin The pin number.
 *
 */
static inline void gpio_disable_pin_periph_event(ioport_pin_t pin)
{
	GpioPort *gpio_port = &(GPIO->GPIO_PORT[ioport_pin_to_port_id(pin)]);
	gpio_port->GPIO_EVERC = ioport_pin_to_mask(pin);
}

/** @} */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

/**
 * \page sam_gpio_quick_start Quick Start Guide for the GPIO driver
 *
 * This is the quick start guide for the \ref group_sam_drivers_gpio, with
 * step-by-step instructions on how to configure and use the driver for
 * a specific use case.
 *
 * The use cases contain several code fragments. The code fragments in the
 * steps for setup can be copied into a custom initialization function, while
 * the steps for usage can be copied into, e.g., the main application function.
 *
 * \section gpio_qs_use_cases Use Cases
 * - \ref gpio_int
 *
 * \section gpio_int GPIO Interrupt Usage
 *
 * This use case will demonstrate how to configure a pin(PC03) to trigger an
 * interrupt on SAM4L-EK board.
 *
 * \section gpio_int_setup Setup Steps
 *
 * \subsection gpio_int_prereq Prerequisites
 *
 * This module requires the following service
 * - \ref clk_group
 * - \ref ioport_group
 *
 * \subsection gpio_int_setup_code Setup Code Example
 *
 * Add this to the main loop or a setup function:
 * \code
 *   ioport_set_pin_dir(PIN_PC03, IOPORT_DIR_INPUT);
 *   ioport_set_pin_mode(PIN_PC03, IOPORT_MODE_PULLUP |
 *       IOPORT_MODE_GLITCH_FILTER);
 *   ioport_set_pin_sense_mode(PIN_PC03, IOPORT_SENSE_FALLING);
 *   if (!gpio_set_pin_callback(PIN_PC03, pb0_callback, 1)) {
 *       printf("Set pin callback failure!\r\n");
 *       while (1) {
 *       }
 *   }
 *   gpio_enable_pin_interrupt(PIN_PC03);
 * \endcode
 *
 * \subsection gpio_setup_workflow Basic Setup Workflow
 *
 * -# Initialize a pin to trigger an interrupt. Here, we initialize PC03 as
 *  input pin with pull up and glitch filter and to generate an interrupt
 *  when pin falling edge.
 *  \code
 *   ioport_set_pin_dir(PIN_PC03, IOPORT_DIR_INPUT);
 *   ioport_set_pin_mode(PIN_PC03, IOPORT_MODE_PULLUP |
 *       IOPORT_MODE_GLITCH_FILTER);
 *   ioport_set_pin_sense_mode(PIN_PC03, IOPORT_SENSE_FALLING);
 *  \endcode
 * -# Set a callback for the pin interrupt
 *  \code
 *   if (!gpio_set_pin_callback(PIN_PC03, pb0_callback, 1)) {
 *       printf("Set pin callback failure!\r\n");
 *       while (1) {
 *       }
 *   }
 *  \endcode
 * -# Enable pin interrupt
 *  \code gpio_enable_pin_interrupt(PIN_PC03); \endcode
 *
 * \section gpio_int_usage Interrupt Usage
 *
 * When an interrupt happens on a pin, it will execute your callback function.
 * \code
 *  static void pb0_callback(void)
 *  {
 *      // Pin interrupt handle code
 *  }
 * \endcode
 *
 */
#endif  /* GPIO_H_INCLUDED */
