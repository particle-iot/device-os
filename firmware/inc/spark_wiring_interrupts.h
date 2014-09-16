/**
 ******************************************************************************
 * @file    spark_wiring_interrupts.h
 * @author  Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_wiring_interrupts.c module
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
#ifndef __SPARK_WIRING_INTERRUPTS_H
#define __SPARK_WIRING_INTERRUPTS_H

#include "spark_wiring.h"

/*
*Interrupts
*/

typedef enum InterruptMode {
  CHANGE,
  RISING,
  FALLING
} InterruptMode;

typedef void (*voidFuncPtr)(void);

void attachInterrupt(uint16_t pin, voidFuncPtr handler, InterruptMode mode);
void detachInterrupt(uint16_t pin);
void interrupts(void);
void noInterrupts(void);

#endif /* SPARK_WIRING_INTERRUPTS_H_ */
