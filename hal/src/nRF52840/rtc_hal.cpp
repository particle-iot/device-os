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

time_t HAL_RTC_Get_UnixTime(void)
{
    return 0;
}

void HAL_RTC_Set_Alarm(uint32_t value)
{
}

void HAL_RTC_Set_UnixAlarm(time_t value)
{

}

void HAL_RTC_Cancel_UnixAlarm(void)
{
}

void HAL_RTC_Set_UnixTime(time_t value)
{
}

uint8_t HAL_RTC_Time_Is_Valid(void* reserved)
{
    return 0;
}