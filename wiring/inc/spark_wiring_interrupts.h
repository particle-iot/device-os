/**
 ******************************************************************************
 * @file    spark_wiring_interrupts.h
 * @author  Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_wiring_interrupts.c module
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
#ifndef __SPARK_WIRING_INTERRUPTS_H
#define __SPARK_WIRING_INTERRUPTS_H

#include <functional>

#include "interrupts_hal.h"
#include "atomic_section.h"

typedef std::function<void()> wiring_interrupt_handler_t;
typedef void (*raw_interrupt_handler_t)(void);

/*
 * GPIO Interrupts
 */
bool attachInterrupt(uint16_t pin, wiring_interrupt_handler_t handler, InterruptMode mode, int8_t priority = -1, uint8_t subpriority = 0);
bool attachInterrupt(uint16_t pin, raw_interrupt_handler_t handler, InterruptMode mode, int8_t priority = -1, uint8_t subpriority = 0);
template <typename T>
bool attachInterrupt(uint16_t pin, void (T::*handler)(), T *instance, InterruptMode mode, int8_t priority = -1, uint8_t subpriority = 0) {
    using namespace std::placeholders;
    return attachInterrupt(pin, std::bind(handler, instance), mode, priority, subpriority);
}
bool detachInterrupt(uint16_t pin);
void interrupts(void);
void noInterrupts(void);

/*
 * System Interrupts
 */
bool attachInterruptDirect(IRQn_Type irq, hal_interrupt_direct_handler_t handler, bool enable = true);
bool detachInterruptDirect(IRQn_Type irq, bool disable = true);

#endif /* SPARK_WIRING_INTERRUPTS_H_ */
