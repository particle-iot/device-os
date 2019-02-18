/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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
 */

#ifndef __SERVO_HAL_H
#define __SERVO_HAL_H

#include "pinmap_hal.h"

#define SERVO_TIM_PWM_FREQ      50 //50Hz

#ifdef __cplusplus
extern "C" {
#endif

void HAL_Servo_Attach(uint16_t pin);
void HAL_Servo_Detach(uint16_t pin);
void HAL_Servo_Write_Pulse_Width(uint16_t pin, uint16_t pulseWidth);
uint16_t HAL_Servo_Read_Pulse_Width(uint16_t pin);
uint16_t HAL_Servo_Read_Frequency(uint16_t pin);

#ifdef __cplusplus
}
#endif

#endif  /* __SERVO_HAL_H */
