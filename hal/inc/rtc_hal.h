/**
 ******************************************************************************
 * @file    rtc_hal.h
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __RTC_HAL_H
#define __RTC_HAL_H

#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include "time_compat.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*hal_rtc_alarm_handler)(void* context);

typedef enum hal_rtc_alarm_flags {
    HAL_RTC_ALARM_FLAG_IN = 0x01 // In provided amount of time, instead of an absolute timestamp
} hal_rtc_alarm_flags;

void hal_rtc_init(void);
int hal_rtc_get_time(struct timeval* tv, void* reserved);
int hal_rtc_set_time(const struct timeval* tv, void* reserved);
bool hal_rtc_time_is_valid(void* reserved);
int hal_rtc_set_alarm(const struct timeval* tv, uint32_t flags, hal_rtc_alarm_handler handler, void* context, void* reserved);
void hal_rtc_cancel_alarm(void);

// These functions are deprecated and are only used for backwards compatibility
// due to time_t size change
time32_t hal_rtc_get_unixtime_deprecated(void);
void hal_rtc_set_unixtime_deprecated(time32_t value);
//

#ifdef __cplusplus
}
#endif

#endif  /* __RTC_HAL_H */
