/**
 ******************************************************************************
 * @file    particle_wiring_interrupts.h
 * @author  Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for particle_wiring_interrupts.c module
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.
  Copyright (c) 2004-05 Hernando Barragan
  Modified 24 November 2006 by David A. Mellis
  Modified 1 August 2010 by Mark Sproul

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */
#ifndef __PARTICLE_WIRING_INTERRUPTS_H
#define __PARTICLE_WIRING_INTERRUPTS_H

#include "particle_wiring.h"
#include "interrupts_hal.h"
#include <functional>

typedef std::function<void()> wiring_interrupt_handler_t;
typedef void (*raw_interrupt_handler_t)(void);

/*
 * GPIO Interrupts
 */
bool attachInterrupt(uint16_t pin, wiring_interrupt_handler_t handler, InterruptMode mode);
bool attachInterrupt(uint16_t pin, raw_interrupt_handler_t handler, InterruptMode mode);
template <typename T>
bool attachInterrupt(uint16_t pin, void (T::*handler)(), T *instance, InterruptMode mode) {
    using namespace std::placeholders;
    return attachInterrupt(pin, std::bind(handler, instance), mode);
}
void detachInterrupt(uint16_t pin);
void interrupts(void);
void noInterrupts(void);

/*
 * System Interrupts
 */
bool attachSystemInterrupt(hal_irq_t irq, wiring_interrupt_handler_t handler);

/**
 * Removes all registered handlers from the given system interrupt.
 * @param irq   The interrupt from which all handlers are removed.
 * @return {@code true} if handlers were removed.
 */
bool detachSystemInterrupt(hal_irq_t irq);

#endif /* PARTICLE_WIRING_INTERRUPTS_H_ */
