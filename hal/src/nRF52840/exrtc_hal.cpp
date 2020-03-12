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

// #define LOG_CHECKED_ERRORS 1

#include "check.h"
#include "system_error.h"
#include "am18x5.h"

using namespace particle;

namespace {

const auto UNIX_TIME_201801010000 = 1514764800; // 2018/01/01 00:00:00
const auto UNIX_TIME_YEAR_BASE = 118; // 2018 - 1900

} // anonymous

int hal_exrtc_set_unixtime(time_t unixtime, void* reserved) {
    struct tm* calendar = gmtime(&unixtime);
    if (!calendar) {
        return SYSTEM_ERROR_INTERNAL;
    }
    CHECK(Am18x5::getInstance().setCalendar(calendar));
    return SYSTEM_ERROR_NONE;
}

time_t hal_exrtc_get_unixtime(void* reserved) {
    struct tm calendar;
    CHECK(Am18x5::getInstance().getCalendar(&calendar));
    return mktime(&calendar);
}

int hal_exrtc_set_unix_alarm(time_t unixtime, hal_exrtc_alarm_handler_t handler, void* context, void* reserved) {
    CHECK_TRUE(handler, SYSTEM_ERROR_INVALID_ARGUMENT);
    struct tm* calendar = gmtime(&unixtime);
    if (!calendar) {
        return SYSTEM_ERROR_INTERNAL;
    }
    CHECK(Am18x5::getInstance().setAlarm(calendar));
    return Am18x5::getInstance().enableAlarm(true, handler, context);
}

int hal_exrtc_cancel_unixalarm(void* reserved) {
    return Am18x5::getInstance().enableAlarm(false, nullptr, nullptr);
}

bool hal_exrtc_time_is_valid(void* reserved) {
    time_t unixtime = 0;
    CHECK(hal_exrtc_get_unixtime(nullptr));
    return unixtime > UNIX_TIME_201801010000;
}

#endif // HAL_PLATFORM_EXTERNAL_RTC
