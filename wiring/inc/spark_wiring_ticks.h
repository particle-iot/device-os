/**
 ******************************************************************************
 * @file    spark_wiring_ticks.h
 * @author  Satish Nair, Zachary Crockett, Zach Supalla and Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   Header for spark_wiring.c module
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

#ifndef SPARK_WIRING_TICKS_H
#define	SPARK_WIRING_TICKS_H

#include <chrono>

#include "delay_hal.h"
#include "timer_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

inline system_tick_t millis(void) { return HAL_Timer_Get_Milli_Seconds(); }
inline unsigned long micros(void) { return HAL_Timer_Get_Micro_Seconds(); }
void delay(unsigned long ms);
inline void delayMicroseconds(unsigned int us) { HAL_Delay_Microseconds(us); }

#ifdef __cplusplus
}
#endif

static inline void delay(std::chrono::milliseconds ms) { ::delay(ms.count()); }
static inline void delayMicroseconds(std::chrono::microseconds us) { ::delayMicroseconds(us.count()); }

#endif	/* SPARK_WIRING_TICKS_H */
