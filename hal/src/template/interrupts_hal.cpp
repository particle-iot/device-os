/**
 ******************************************************************************
 * @file    interrupts_hal.c
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

/* Includes ------------------------------------------------------------------*/
#include "interrupts_hal.h"
int hal_interrupt_attach(uint16_t pin, hal_interrupt_handler_t handler, void* data, InterruptMode mode, hal_interrupt_extra_configuration_t* config)
{
    return 0;
}

int hal_interrupt_detach(uint16_t pin)
{
    return 0;
}

int hal_interrupt_detach_ext(uint16_t pin, uint8_t keepHandler, void* reserved)
{
    return 0;
}

void hal_interrupt_enable_all(void)
{
}

void hal_interrupt_disable_all(void)
{
}

void hal_interrupt_suspend(void)
{
}

void hal_interrupt_restore(void)
{
}

/*******************************************************************************
 * Function Name  : HAL_EXTI_Handler (Declared as weak in stm32_it.cpp)
 * Description    : This function is called by any of the interrupt handlers. It
                                                 essentially fetches the user function pointer from the array
                                                 and calls it.
 * Input          : EXTI_Line (Supported: 0, 1, 3, 4, 5, 6, 7, 13, 14 and 15)
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_EXTI_Handler(uint8_t EXTI_Line)
{
}


int HAL_disable_irq()
{
    return 0;
}

void HAL_enable_irq(int is)
{
}

int hal_interrupt_set_direct_handler(IRQn_Type irqn, hal_interrupt_direct_handler_t handler, uint32_t flags, void* reserved)
{
    return 1;
}
