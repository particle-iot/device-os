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

} // anonymous

int hal_exrtc_init(void* reserved) {
    (void)Am18x5::getInstance();
    return 0;
}

int hal_exrtc_set_time(const struct timeval* tv, void* reserved) {
    return Am18x5::getInstance().setTime(tv);
}

int hal_exrtc_get_time(struct timeval* tv, void* reserved) {
    return Am18x5::getInstance().getTime(tv);
}

int hal_exrtc_set_alarm(const struct timeval* tv, uint32_t flags, hal_exrtc_alarm_handler handler, void* context, void* reserved) {
    CHECK_TRUE(tv, SYSTEM_ERROR_INVALID_ARGUMENT);
    struct timeval alarm = *tv;
    if (flags & HAL_RTC_ALARM_FLAG_IN) {
        struct timeval now;
        CHECK(hal_exrtc_get_time(&now, nullptr));
        timeradd(&now, tv, &alarm);
    }
    CHECK(Am18x5::getInstance().setAlarm(&alarm));
    CHECK(Am18x5::getInstance().enableAlarm(true, handler, context));

    int res = CHECK(Am18x5::getInstance().getAlarm(&alarm));
    struct timeval now;
    CHECK(hal_exrtc_get_time(&now, nullptr));
    // If alarm time is in the past and it hasn't fired
    if (timercmp(&alarm, &now, <) && res == 0) {
        return SYSTEM_ERROR_TIMEOUT;
    }
    return 0;
}

int hal_exrtc_cancel_alarm(void* reserved) {
    return Am18x5::getInstance().enableAlarm(false, nullptr, nullptr);
}

bool hal_exrtc_time_is_valid(void* reserved) {
    struct timeval tv;
    if (!hal_exrtc_get_time(&tv, nullptr)) {
        return tv.tv_sec > UNIX_TIME_201801010000;
    }
    return false;
}

int hal_exrtc_enable_watchdog(system_tick_t ms, void* reserved) {
    uint8_t value; // Maximum 31.
    Am18x5WatchdogFrequency frequency;
    if (ms < 1937) { // 31 * 1000 / 16
        frequency = Am18x5WatchdogFrequency::HZ_16;
        value = ms * 16 / 1000;
    } else if (ms <= 7750) {
        frequency = Am18x5WatchdogFrequency::HZ_4;
        value = ms * 4 / 1000;
    } else if (ms <= 31000) {
        frequency = Am18x5WatchdogFrequency::HZ_1;
        value = ms / 1000;
    } else if (ms <= 124000) {
        frequency = Am18x5WatchdogFrequency::HZ_1_4;
        value = ms / 4 / 1000;
    } else {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    CHECK_TRUE(value > 0, SYSTEM_ERROR_INVALID_ARGUMENT);
    return Am18x5::getInstance().enableWatchdog(value, frequency);
}

int hal_exrtc_disable_watchdog(void* reserved) {
    return Am18x5::getInstance().disableWatchdog();
}

int hal_exrtc_feed_watchdog(void* reserved) {
    return Am18x5::getInstance().feedWatchdog();
}

int hal_exrtc_sleep_timer(system_tick_t ms, void* reserved) {
    uint8_t ticks;
    Am18x5TimerFrequency frequency;
    if (ms <= 3984) { // 255 * 1000 / 64
        frequency = Am18x5TimerFrequency::HZ_64;
        ticks = ms * 64 / 1000;
    } else if (ms <= 255000) {
        frequency = Am18x5TimerFrequency::HZ_1;
        ticks = ms / 1000;
    } else if (ms <= 15300000) {
        frequency = Am18x5TimerFrequency::HZ_1_60;
        ticks = ms / 60 / 1000;
    } else {
        // TODO: use alarm or watchdog as the wakeup source
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    CHECK_TRUE(ticks > 0, SYSTEM_ERROR_INVALID_ARGUMENT);
    return Am18x5::getInstance().sleep(ticks, frequency);
}

int hal_exrtc_calibrate_xt(int adjValue, void* reserved) {
    return Am18x5::getInstance().xtOscillatorDigitalCalibration(adjValue);
}

void hal_exrtc_get_watchdog_limits(system_tick_t* low, system_tick_t* high, void* reserved) {
    if (low) {
        *low = 63; // round(Am18x5WatchdogFrequency::HZ_16 * 1)
    }
    if (high) {
        *high = 124000; // // round(Am18x5WatchdogFrequency::HZ_1_4 * 31)
    }
}

#endif // HAL_PLATFORM_EXTERNAL_RTC
