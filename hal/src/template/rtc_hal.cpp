/**
 ******************************************************************************
 * @file    rtc_hal.c
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
#include "rtc_hal.h"

void hal_rtc_init(void) {
}

int hal_rtc_get_time(struct timeval* tv, void* reserved) {
    return 0;
}

int hal_rtc_set_time(const struct timeval* tv, void* reserved) {
    return 0;
}

bool hal_rtc_time_is_valid(void* reserved) {
    return false;
}

int hal_rtc_set_alarm(const struct timeval* tv, uint32_t flags, hal_rtc_alarm_handler handler, void* context, void* reserved) {
    return 0;
}

void hal_rtc_cancel_alarm(void) {
}
