/**
 ******************************************************************************
 * @file    spark_wiring_interrupts.cpp
 * @author  Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Wrapper for wiring interrupts
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.
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

/*******************************************************************************
 * Function Name  : attachInterrupt
 * Description    : Arduino compatible function to attach hardware interrupts to
						 the Core pins
 * Input          : pin number, user function name and interrupt mode
 * Output         : None.
 * Return         : None.
 *******************************************************************************/

void attachInterrupt(uint16_t pin, voidFuncPtr handler, InterruptMode mode)
{
  HAL_Interrupts_Attach(pin, handler, mode);
}


/*******************************************************************************
 * Function Name  : detachInterrupt
 * Description    : Arduino compatible function to detach hardware interrupts that
						 were asssigned previously using attachInterrupt
 * Input          : pin number to which the interrupt was attached
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void detachInterrupt(uint16_t pin)
{
  HAL_Interrupts_Detach(pin);
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
