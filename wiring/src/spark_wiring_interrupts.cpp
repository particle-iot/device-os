/**
 ******************************************************************************
 * @file    spark_wiring_interrupts.cpp
 * @author  Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Wrapper for wiring interrupts
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
#include "spark_wiring_interrupts.h"
#include "spark_wiring_platform.h"
#include "system_error.h"

static wiring_interrupt_handler_t* handlers[TOTAL_PINS];

wiring_interrupt_handler_t* allocate_handler(uint16_t pin, wiring_interrupt_handler_t& fn)
{
    if (handlers[pin]) {
        handlers[pin]->swap(fn);
    } else {
        handlers[pin] = new wiring_interrupt_handler_t(fn);
    }
    return handlers[pin];
}

void call_wiring_interrupt_handler(void* data)
{
    wiring_interrupt_handler_t* handler = (wiring_interrupt_handler_t*)data;
    (*handler)();
}

void call_raw_interrupt_handler(void* data)
{
    raw_interrupt_handler_t handler = raw_interrupt_handler_t(data);
    handler();
}

/*******************************************************************************
 * Function Name  : attachInterrupt
 * Description    : Arduino compatible function to attach hardware interrupts to
						        the Core pins
 * Input          : pin number, user function name and interrupt mode
 * Output         : None.
 * Return         : true if function handler was allocated, false otherwise.
 *******************************************************************************/

HAL_InterruptExtraConfiguration* configure_interrupt(HAL_InterruptExtraConfiguration& extra, int8_t priority, uint8_t subpriority)
{
  extra.version = HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION_1;
  if (priority >= 0) {
    extra.IRQChannelPreemptionPriority = (uint8_t)priority;
    extra.IRQChannelSubPriority = subpriority;
    return &extra;
  }
  return nullptr;
}

bool attachInterrupt(uint16_t pin, wiring_interrupt_handler_t fn, InterruptMode mode, int8_t priority, uint8_t subpriority)
{
    HAL_Interrupts_Detach(pin);
    wiring_interrupt_handler_t* handler = allocate_handler(pin, fn);
    if (handler) {
        HAL_InterruptExtraConfiguration extra = {};
        if (SYSTEM_ERROR_NONE != HAL_Interrupts_Attach(pin, call_wiring_interrupt_handler, handler, mode, configure_interrupt(extra, priority, subpriority))) {
            return false;
        }
    }
    return handler!=NULL;
}

bool attachInterrupt(uint16_t pin, raw_interrupt_handler_t handler, InterruptMode mode, int8_t priority, uint8_t subpriority)
{
    HAL_Interrupts_Detach(pin);
    HAL_InterruptExtraConfiguration extra = {};
    if (SYSTEM_ERROR_NONE != HAL_Interrupts_Attach(pin, call_raw_interrupt_handler, (void*)handler, mode, configure_interrupt(extra, priority, subpriority))) {
        return false;
    }
    return true;
}

/*******************************************************************************
 * Function Name  : detachInterrupt
 * Description    : Arduino compatible function to detach hardware interrupts that
						        were asssigned previously using attachInterrupt
 * Input          : pin number to which the interrupt was attached
 * Output         : None.
 * Return         : true or false
 *******************************************************************************/
bool detachInterrupt(uint16_t pin)
{
    if (SYSTEM_ERROR_NONE != HAL_Interrupts_Detach(pin)) {
        return false;
    }
    // NB: We do not `delete handlers[pin]` here since this would cause an error
    // when detachInterrupt is called in an ISR.
    return true;
}

/*******************************************************************************
 * Function Name  : noInterrupts
 * Description    : Disable all external interrupts
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void noInterrupts(void)
{
  //Only disable the interrupts that are exposed to the user
  HAL_Interrupts_Disable_All();
}


/*******************************************************************************
 * Function Name  : interrupts
 * Description    : Enable all external interrupts
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void interrupts(void)
{
  //Only enable the interrupts that are exposed to the user
  HAL_Interrupts_Enable_All();
}

/*
 * System Interrupts
 */
bool attachSystemInterrupt(hal_irq_t irq, wiring_interrupt_handler_t handler)
{
    HAL_InterruptCallback callback;
    callback.handler = call_wiring_interrupt_handler;
    wiring_interrupt_handler_t& h = handler;
    callback.data = new wiring_interrupt_handler_t(h);
    HAL_InterruptCallback prev = {};
    const bool ok = HAL_Set_System_Interrupt_Handler(irq, &callback, &prev, NULL);
    delete (wiring_interrupt_handler_t*)prev.data;
    return ok;
}

/**
 * Removes all registered handlers from the given system interrupt.
 * @param irq   The interrupt from which all handlers are removed.
 * @return {@code true} if handlers were removed.
 */
bool detachSystemInterrupt(hal_irq_t irq)
{
    // NB: We do not delete the previous handler here since this would cause an error
    // when detachInterrupt is called in an ISR.
    return HAL_Set_System_Interrupt_Handler(irq, NULL, NULL, NULL);
}

bool attachInterruptDirect(IRQn_Type irq, HAL_Direct_Interrupt_Handler handler, bool enable)
{
    const bool ok = !HAL_Set_Direct_Interrupt_Handler(irq, handler, enable ? HAL_DIRECT_INTERRUPT_FLAG_ENABLE : HAL_DIRECT_INTERRUPT_FLAG_NONE, nullptr);
    return ok;
}

bool detachInterruptDirect(IRQn_Type irq, bool disable)
{
    const bool ok = !HAL_Set_Direct_Interrupt_Handler(irq, nullptr,
        HAL_DIRECT_INTERRUPT_FLAG_RESTORE | (disable ? HAL_DIRECT_INTERRUPT_FLAG_DISABLE : HAL_DIRECT_INTERRUPT_FLAG_NONE), nullptr);

    return ok;
}
