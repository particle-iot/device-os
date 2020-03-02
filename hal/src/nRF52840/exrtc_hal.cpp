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

#include "exrtc_hal.h"

#if HAL_PLATFORM_EXTERNAL_RTC

#include "check.h"
#include "system_error.h"
#include "am18x5.h"

using namespace particle;

namespace {

const auto UNIX_TIME_201801010000 = 1514764800; // 2018/01/01 00:00:00
const auto UNIX_TIME_YEAR_BASE = 118; // 2018 - 1900

hal_exrtc_alarm_handler_t alarmHandler = nullptr;
void* alarmContext = nullptr;
uint8_t alarmYear = 0;

} // anonymous

int hal_exrtc_set_unixtime(time_t unixtime, void* reserved) {
    struct tm* calendar = gmtime(&unixtime);
    if (!calendar) {
        return SYSTEM_ERROR_INTERNAL;
    }
    CHECK_TRUE(calendar->tm_year >= UNIX_TIME_YEAR_BASE, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK(AM18X5.setWeekday(calendar->tm_wday));
    CHECK(AM18X5.setYears(calendar->tm_year - UNIX_TIME_YEAR_BASE));
    CHECK(AM18X5.setMonths(calendar->tm_mon));
    CHECK(AM18X5.setDate(calendar->tm_mday));
    CHECK(AM18X5.setHours(calendar->tm_hour, HourFormat::HOUR24));
    CHECK(AM18X5.setMinutes(calendar->tm_min));
    CHECK(AM18X5.setSeconds(calendar->tm_sec));
    return SYSTEM_ERROR_NONE;
}

time_t hal_exrtc_get_unixtime(time_t* unixtime, void* reserved) {
    struct tm calendar;
    CHECK(calendar.tm_sec = AM18X5.getSeconds());
    CHECK(calendar.tm_min = AM18X5.getMinutes());
    HourFormat format = HourFormat::HOUR24;
    CHECK(calendar.tm_hour = AM18X5.getHours(&format));
    if (format == HourFormat::HOUR12_PM) {
        calendar.tm_hour += 12;
    }
    CHECK(calendar.tm_mday = AM18X5.getDate());
    CHECK(calendar.tm_mon = AM18X5.getMonths());
    CHECK(calendar.tm_year = AM18X5.getYears());
    calendar.tm_year += UNIX_TIME_YEAR_BASE;
    CHECK(calendar.tm_wday = AM18X5.getWeekday());
    *unixtime = mktime(&calendar);
    return SYSTEM_ERROR_NONE;
}

int hal_exrtc_set_unix_alarm(time_t unixtime, hal_exrtc_alarm_handler_t handler, void* context, void* reserved) {
    CHECK_TRUE(handler, SYSTEM_ERROR_INVALID_ARGUMENT);
    struct tm* calendar = gmtime(&unixtime);
    if (!calendar) {
        return SYSTEM_ERROR_INTERNAL;
    }
    CHECK_TRUE(calendar->tm_year >= UNIX_TIME_YEAR_BASE, SYSTEM_ERROR_INVALID_ARGUMENT);
    alarmYear = calendar->tm_year - UNIX_TIME_YEAR_BASE;
    CHECK(AM18X5.setMonths(calendar->tm_mon, true));
    CHECK(AM18X5.setDate(calendar->tm_mday, true));
    CHECK(AM18X5.setHours(calendar->tm_hour, HourFormat::HOUR24, true));
    CHECK(AM18X5.setMinutes(calendar->tm_min, true));
    CHECK(AM18X5.setSeconds(calendar->tm_sec, true));
    alarmHandler = handler;
    alarmContext = context;
    return SYSTEM_ERROR_NONE;
}

int hal_exrtc_cancel_unixalarm(void* reserved) {
    return SYSTEM_ERROR_NONE;
}

bool hal_exrtc_time_is_valid(void* reserved) {
    time_t unixtime = 0;
    CHECK(hal_exrtc_get_unixtime(&unixtime, nullptr));
    return unixtime > UNIX_TIME_201801010000;
}

#endif // HAL_PLATFORM_EXTERNAL_RTC
