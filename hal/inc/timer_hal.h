/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TIMER_HAL_H
#define __TIMER_HAL_H

/* Includes ------------------------------------------------------------------*/
#include "system_tick_hal.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

#define HAL_Timer_Microseconds HAL_Timer_Get_Micro_Seconds
#define HAL_Timer_Milliseconds HAL_Timer_Get_Milli_Seconds

/* Exported functions --------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

system_tick_t HAL_Timer_Get_Micro_Seconds(void);
system_tick_t HAL_Timer_Get_Milli_Seconds(void);

/**
 * Returns the number of milliseconds passed since the device was last reset. This function is
 * similar to HAL_Timer_Get_Milli_Seconds() but returns a 64-bit value.
 */
uint64_t hal_timer_millis(void* reserved);

/**
 * Returns the number of microseconds passed since the device was last reset. This function is
 * similar to HAL_Timer_Get_Micro_Seconds() but returns a 64-bit value.
 */
uint64_t hal_timer_micros(void* reserved);

typedef struct {
    uint16_t size;
    uint16_t version;
    uint64_t base_clock_offset;
} hal_timer_init_config_t;

/**
 * @brief      Initializes timer HAL
 *
 * @param      conf  (optional) Configuration
 *
 * @return     0 in case of success, any other value in case of an error
 */
int hal_timer_init(const hal_timer_init_config_t* conf);

/**
 * @brief      Deinitializes timer HAL
 *
 * @param      reserved  Reserved argument
 *
 * @return     0 in case of success, any other value in case of an error
 */
int hal_timer_deinit(void* reserved);

#ifdef __cplusplus
}
#endif

#endif  /* __TIMER_HAL_H */
