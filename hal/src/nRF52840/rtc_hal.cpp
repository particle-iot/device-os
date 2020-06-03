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

#include "rtc_hal.h"
#include "timer_hal.h"
#include "hal_irq_flag.h"
#include "concurrent_hal.h"
#include "service_debug.h"
#include "hal_platform.h"
#include "check.h"

#if HAL_PLATFORM_EXTERNAL_RTC
#include "exrtc_hal.h"
#endif

// This implementation uses timer_hal. See timer_hal.cpp for additional information
// on millisecond and microsecond counter source and their properties.

extern "C" void HAL_RTCAlarm_Handler(void);

namespace {

const uint64_t UNIX_TIME_201801010000 = 1514764800000000; // 2018/01/01 00:00:00

uint64_t s_unix_time_base = 946684800000000; // Default date/time to 2000/01/01 00:00:00
uint64_t s_unix_time_base_us = 0; // Microsecond clock reference to the s_unix_time_base

const uint64_t US_IN_SECONDS = 1000000ULL;

#if !HAL_PLATFORM_EXTERNAL_RTC
os_timer_t s_alarm_timer = nullptr; // software alarm timer
hal_rtc_alarm_handler s_alarm_handler = nullptr;
void* s_alarm_context = nullptr;
#endif // !HAL_PLATFORM_EXTERNAL_RTC

uint64_t getUsUnixTime() {
    int st = HAL_disable_irq();
    auto unix_time_base = s_unix_time_base;
    auto unix_time_base_us = s_unix_time_base_us;
    HAL_enable_irq(st);
    uint64_t us = hal_timer_micros(nullptr);
    uint64_t unixTimeUs = unix_time_base + (us - unix_time_base_us);
    return unixTimeUs;
}

void timevalFromUsUnixtime(struct timeval* tv, uint64_t us) {
    tv->tv_sec = us / US_IN_SECONDS;
    tv->tv_usec = (us - (tv->tv_sec * US_IN_SECONDS));
}

uint64_t usUnixtimeFromTimeval(const struct timeval* tv) {
    return (tv->tv_sec * US_IN_SECONDS + tv->tv_usec);
}

} // anonymous

void hal_rtc_init(void) {
#if HAL_PLATFORM_EXTERNAL_RTC
    hal_exrtc_init(nullptr);
    struct timeval tv = {};
    if (!hal_exrtc_get_time(&tv, nullptr)) {
        hal_rtc_set_time(&tv, nullptr);
    }
#else
    // Do nothing
#endif
}

bool hal_rtc_time_is_valid(void* reserved) {
    return s_unix_time_base > UNIX_TIME_201801010000;
}

int hal_rtc_get_time(struct timeval* tv, void* reserved) {
    CHECK_TRUE(tv, SYSTEM_ERROR_INVALID_ARGUMENT);
    auto unixTimeUs = getUsUnixTime();
    timevalFromUsUnixtime(tv, unixTimeUs);
    return 0;
}

int hal_rtc_set_time(const struct timeval* tv, void* reserved) {
    CHECK_TRUE(tv, SYSTEM_ERROR_INVALID_ARGUMENT);
    uint64_t us = hal_timer_micros(nullptr);
    int st = HAL_disable_irq();
    s_unix_time_base = usUnixtimeFromTimeval(tv);
    s_unix_time_base_us = us;
    HAL_enable_irq(st);
#if HAL_PLATFORM_EXTERNAL_RTC
    CHECK(hal_exrtc_set_time(tv, nullptr));
#endif
    return 0;
}

int hal_rtc_set_alarm(const struct timeval* tv, uint32_t flags, hal_rtc_alarm_handler handler, void* context, void* reserved) {
#if HAL_PLATFORM_EXTERNAL_RTC
    return hal_exrtc_set_alarm(tv, flags, handler, context, nullptr);
#else
    CHECK_TRUE(tv, SYSTEM_ERROR_INVALID_ARGUMENT);
    struct timeval alarm = *tv;
    if (flags & HAL_RTC_ALARM_FLAG_IN) {
        struct timeval now;
        CHECK(hal_rtc_get_time(&now, nullptr));
        timeradd(&alarm, &now, &alarm);
    }
    auto unixTimeMs = getUsUnixTime() / 1000;
    auto alarmTimeMs = usUnixtimeFromTimeval(&alarm) / 1000;
    if (alarmTimeMs <= unixTimeMs) {
        // Too late to set such an alarm
        return SYSTEM_ERROR_TIMEOUT;
    }

    // This implementation is only used for System.sleep(seconds) (network sleep)
    // on Gen3 devices.
    if (!s_alarm_timer) {
        os_timer_create(&s_alarm_timer, 0, [](os_timer_t timer) {
            if (s_alarm_handler) {
                s_alarm_handler(s_alarm_context);
            }
        }, nullptr, true, nullptr);
        SPARK_ASSERT(s_alarm_timer);
    }

    hal_rtc_cancel_alarm();

    s_alarm_context = context;
    s_alarm_handler = handler;

    unsigned diffMs = (unsigned)(alarmTimeMs - unixTimeMs);
    // NOTE: changing the period of a timer in a dormant state will also
    // start the timer.
    int r = os_timer_change(s_alarm_timer, OS_TIMER_CHANGE_PERIOD, false, diffMs,
            0xffffffff, nullptr);

    if (r != 0) {
        return SYSTEM_ERROR_INTERNAL;
    }

    return r;
#endif
}

void hal_rtc_cancel_alarm(void) {
#if HAL_PLATFORM_EXTERNAL_RTC
    hal_exrtc_cancel_alarm(nullptr);
#else
    // This implementation is only used for System.sleep(seconds) (network sleep)
    // on Gen3 devices.
    if (s_alarm_timer) {
        os_timer_change(s_alarm_timer, OS_TIMER_CHANGE_STOP, false, 0, 0xffffffff, nullptr);
    }
#endif
}

// These are deprecated due to time_t size changes
void hal_rtc_set_unixtime_deprecated(time32_t value) {
    struct timeval tv = {
        .tv_sec = value,
        .tv_usec = 0
    };
    hal_rtc_set_time(&tv, nullptr);
}

time32_t hal_rtc_get_unixtime_deprecated(void) {
    struct timeval tv = {};
    hal_rtc_get_time(&tv, nullptr);
    return (time32_t)tv.tv_sec;
}
