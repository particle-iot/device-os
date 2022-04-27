/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include "testapi.h"
#include "rtc_hal.h"

test(rtc_hal_backwards_compatibility) {
    // These APIs are exposed to user application.
    // Deprecated *dynalib* APIs for backwards compatibility
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    // These APIs are known deprecated APIs, we don't need to see this warning in tests
    API_COMPILE(HAL_RTC_Configuration());
    API_COMPILE({ auto v = HAL_RTC_Get_UnixTime(); (void)v; });
    API_COMPILE(HAL_RTC_Set_UnixTime(12345));
    API_COMPILE(HAL_RTC_Cancel_UnixAlarm());
    API_COMPILE({auto v = HAL_RTC_Time_Is_Valid(nullptr); (void)v; });
#pragma GCC diagnostic pop
}
