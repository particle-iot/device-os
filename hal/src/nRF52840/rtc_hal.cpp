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
#include "core_hal.h"
#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
#include "am18x5.h"
using namespace particle;
#endif

// This implementation uses timer_hal. See timer_hal.cpp for additional information
// on millisecond and microsecond counter source and their properties.

extern "C" void HAL_RTCAlarm_Handler(void);

namespace {

const uint64_t UNIX_TIME_201801010000 = 1514764800000000; // 2018/01/01 00:00:00

const uint64_t UNIX_TIME_20000101000000 = 946684800UL;   // 2000/01/01 00:00:00
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
#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
    bool exRtcPresent = false;
#if HAL_PLATFORM_EXTERNAL_RTC
    hal_am18x5_config_t config = {};
    config.size = sizeof(hal_am18x5_config_t);
    if (Am18x5::getInstance().getConfig(&config) != SYSTEM_ERROR_NONE) {
        config.version = HAL_EXRTC_API_VERSION;
        config.size = sizeof(hal_am18x5_config_t);
        config.default_rtc = false;
        config.wdi_pin = RTC_WDI;
        config.int_pin = RTC_INT;
        config.i2c_if = HAL_PLATFORM_EXTERNAL_RTC_I2C;
        config.rc_fallback = false;
        config.rc_on_battery = false;
        config.osc_src = Am18x5Oscillator::EXTERNAL_CRYSTAL;
        config.osc_cal_xt = HAL_PLATFORM_EXTERNAL_RTC_CAL_XT;
        config.clk_out_en = false;
        config.clk_out_freq = Am18x5SqwFrequency::HZ_32768;
        config.auto_calibration = Am18x5AutoCalibration::AUTO_CAL_DISABLE;
        config.mfg_magic = HAL_EXRTC_MFG_MAGIC;
        config.mfg_osc_cal_xt = HAL_PLATFORM_EXTERNAL_RTC_CAL_XT;
        Am18x5::getInstance().setConfig(&config);
    }
    if (Am18x5::getInstance().begin() == SYSTEM_ERROR_NONE) {
        exRtcPresent = true;
    }
#else
    if (HAL_Feature_Get(FEATURE_EXRTC_DETECTION)) {
        if (Am18x5::getInstance().begin() == SYSTEM_ERROR_NONE) {
            exRtcPresent = true;
        }
    }
#endif
    if (exRtcPresent) {
        struct timeval exRtcTv = {};
        if (!Am18x5::getInstance().isTimeValid(&exRtcTv)) {
            struct timeval tv = {
                .tv_sec = UNIX_TIME_20000101000000,
                .tv_usec = 0
            };
            Am18x5::getInstance().setTime(&tv);
        } else {
            // Sync time from external RTC to internal RTC
            hal_rtc_set_time(&exRtcTv, nullptr);
        }
    }
#else
    // Do nothing
#endif
}

bool hal_rtc_time_is_valid(void* reserved) {
#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
    if (Am18x5::getInstance().isDefault()) {
        return Am18x5::getInstance().isTimeValid();
    }
#endif
    return s_unix_time_base > UNIX_TIME_201801010000;
}

int hal_rtc_get_time(struct timeval* tv, void* reserved) {
    CHECK_TRUE(tv, SYSTEM_ERROR_INVALID_ARGUMENT);
#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
    if (Am18x5::getInstance().isDefault()) {
        return Am18x5::getInstance().getTime(tv);
    }
#endif
    auto unixTimeUs = getUsUnixTime();
    timevalFromUsUnixtime(tv, unixTimeUs);
    return 0;
}

int hal_rtc_set_time(const struct timeval* tv, void* reserved) {
    CHECK_TRUE(tv, SYSTEM_ERROR_INVALID_ARGUMENT);
#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
    if (Am18x5::getInstance().isPresent()) {
        int ret = Am18x5::getInstance().setTime(tv);
        if (Am18x5::getInstance().isDefault()) {
            CHECK(ret);
        }
    }
#endif
    uint64_t us = hal_timer_micros(nullptr);
    int st = HAL_disable_irq();
    s_unix_time_base = usUnixtimeFromTimeval(tv);
    s_unix_time_base_us = us;
    HAL_enable_irq(st);
    return 0;
}

int hal_rtc_set_alarm(const struct timeval* tv, uint32_t flags, hal_rtc_alarm_handler handler, void* context, void* reserved) {
#if HAL_PLATFORM_EXTERNAL_RTC // For backwards compatibility, tracker platform always use external RTC for alarm
    return Am18x5::getInstance().setAlarm(true, flags, tv, handler, context);
#else
#if HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
    if (Am18x5::getInstance().isDefault()) {
        return Am18x5::getInstance().setAlarm(true, flags, tv, handler, context);
    }
#endif
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
#if HAL_PLATFORM_EXTERNAL_RTC // For backwards compatibility, tracker platform always use external RTC for alarm
    Am18x5::getInstance().setAlarm(false);
#else
#if HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
    if (Am18x5::getInstance().isDefault()) {
        Am18x5::getInstance().setAlarm(false);
    }
#else
    // This implementation is only used for System.sleep(seconds) (network sleep)
    // on Gen3 devices.
    if (s_alarm_timer) {
        os_timer_change(s_alarm_timer, OS_TIMER_CHANGE_STOP, false, 0, 0xffffffff, nullptr);
    }
#endif
#endif
}

int hal_rtc_set_source(hal_rtc_source_t source, void* reserved) {
#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
    hal_am18x5_config_t config = {};
    config.size = sizeof(hal_am18x5_config_t);
    if (!Am18x5::getInstance().isPresent()) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    CHECK(Am18x5::getInstance().getConfig(&config));
    uint8_t currDefault = config.default_rtc;
    if (source == HAL_RTC_SOURCE_EXTERNAL && !currDefault) {
        config.default_rtc = true;
    }
    if (source == HAL_RTC_SOURCE_INTERNAL && currDefault) {
        config.default_rtc = false;
    }
    if (currDefault != config.default_rtc) {
        CHECK(Am18x5::getInstance().setConfig(&config));
    }
#else
    if (source == HAL_RTC_SOURCE_EXTERNAL) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
#endif
    return SYSTEM_ERROR_NONE;
}

hal_rtc_source_t hal_rtc_get_source(void* reserved) {
#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
    if (Am18x5::getInstance().isDefault()) {
        return HAL_RTC_SOURCE_EXTERNAL;
    }
#endif
    return HAL_RTC_SOURCE_INTERNAL;
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

#if PLATFORM_ID == PLATFORM_TRACKER // For Tracker-specific backwards compatibility
void hal_exrtc_get_watchdog_limits_deprecated(system_tick_t* low, system_tick_t* high, void* reserved) {
    Am18x5::getInstance().getWatchdogLimits(low, high);
}

int hal_exrtc_enable_watchdog_deprecated(system_tick_t ms, void* reserved) {
    return Am18x5::getInstance().enableWatchdog(ms);
}

int hal_exrtc_disable_watchdog_deprecated(void* reserved) {
    return Am18x5::getInstance().disableWatchdog();
}

int hal_exrtc_feed_watchdog_deprecated(void* reserved) {
    return Am18x5::getInstance().feedWatchdog();
}
#endif // PLATFORM_ID == PLATFORM_TRACKER
