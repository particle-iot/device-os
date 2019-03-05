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

// This implementation uses timer_hal, which in turn relies on alarm implementation
// necessary for OpenThread functionality. See timer_hal.cpp for additional information
// on millisecond and microsecond counter source and their properties.

namespace {

const auto UNIX_TIME_201801010000 = 1514764800; // 2018/01/01 00:00:00

time_t s_unix_time_base = 946684800; // Default date/time to 2000/01/01 00:00:00
uint64_t s_unix_time_base_ms = 0; // Millisecond clock reference to the s_unix_time_base
os_timer_t s_alarm_timer = nullptr; // software alarm timer

} // anonymous

extern "C" void HAL_RTCAlarm_Handler(void);

void HAL_RTC_Configuration(void) {
    // Do nothing
}

void HAL_RTC_Set_UnixTime(time_t value) {
    uint64_t ms = hal_timer_millis(nullptr);
    int st = HAL_disable_irq();
    s_unix_time_base = value;
    s_unix_time_base_ms = ms;
    HAL_enable_irq(st);
}

time_t HAL_RTC_Get_UnixTime(void) {
    int st = HAL_disable_irq();
    auto unix_time_base = s_unix_time_base;
    auto unix_time_base_ms = s_unix_time_base_ms;
    HAL_enable_irq(st);
    uint64_t ms = hal_timer_millis(nullptr);
    return unix_time_base + (time_t)((ms - unix_time_base_ms) / 1000);
}

void HAL_RTC_Set_UnixAlarm(time_t value) {
    // This implementation is only used for System.sleep(seconds) (network sleep)
    // on Gen3 devices.
    if (!s_alarm_timer) {
        os_timer_create(&s_alarm_timer, 0, [](os_timer_t timer) {
            HAL_RTCAlarm_Handler();
        }, nullptr, true, nullptr);
        SPARK_ASSERT(s_alarm_timer);
    }

    HAL_RTC_Cancel_UnixAlarm();

    // NOTE: changing the period of a timer in a dormant state will also
    // start the timer.
    os_timer_change(s_alarm_timer, OS_TIMER_CHANGE_PERIOD, false, value * 1000,
            0xffffffff, nullptr);
}

void HAL_RTC_Cancel_UnixAlarm(void) {
    // This implementation is only used for System.sleep(seconds) (network sleep)
    // on Gen3 devices.
    if (s_alarm_timer) {
        os_timer_change(s_alarm_timer, OS_TIMER_CHANGE_STOP, false, 0, 0xffffffff, nullptr);
    }
}

uint8_t HAL_RTC_Time_Is_Valid(void* reserved) {
    return HAL_RTC_Get_UnixTime() > UNIX_TIME_201801010000;
}
