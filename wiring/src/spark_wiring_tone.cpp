/**
 ******************************************************************************
 * @file    spark_wiring_tone.cpp
 * @author  Satish Nair
 * @version V1.0.0
 * @date    21-April-2014
 * @brief   Generates a square wave of the specified frequency and duration
 * 	  	  	(and 50% duty cycle) on a timer channel pin
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

#include "spark_wiring_tone.h"
#include "tone_hal.h"

void tone(uint8_t pin, unsigned int frequency, unsigned long duration)
{
    if (pinAvailable(pin) && HAL_Validate_Pin_Function(pin, PF_TIMER)==PF_TIMER) {
        HAL_Tone_Start(pin, frequency, duration);
    }
}

void noTone(uint8_t pin)
{
    if (pinAvailable(pin) && HAL_Validate_Pin_Function(pin, PF_TIMER)==PF_TIMER) {
        HAL_Tone_Stop(pin);
    }
}
