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
#include "eeprom_hal.h"
#include "service_debug.h"

using namespace particle;

namespace {

const auto UNIX_TIME_201801010000 = 1514764800; // 2018/01/01 00:00:00

} // anonymous

int hal_exrtc_init(void* reserved) {
    hal_am18x5_config_t config = {};
    config.version = HAL_AM18X5_CONFIG_VERSION;
    config.size = sizeof(hal_am18x5_config_t);
    if (Am18x5::getInstance().getConfig(&config) != SYSTEM_ERROR_NONE) {
        int8_t calValue = 0; 
        size_t eepromSize = HAL_EEPROM_Length();
        SPARK_ASSERT(eepromSize >= 4);
        HAL_EEPROM_Get(eepromSize - 4, &calValue, sizeof(calValue));
        if (calValue < -65 || calValue > -25) {
            calValue = HAL_PLATFORM_EXTERNAL_RTC_CAL_XT;
        }
        config.default_rtc = false;
        config.wdi_pin = RTC_WDI;
        config.int_pin = RTC_INT;
        config.i2c_if = HAL_PLATFORM_EXTERNAL_RTC_I2C;
        config.rc_fallback = false;
        config.rc_on_battery = false;
        config.osc_src = Am18x5Oscillator::EXTERNAL_CRYSTAL;
        config.osc_cal_xt = calValue;
        config.clk_out_en = false;
        config.clk_out_freq = Am18x5SqwFrequency::HZ_32768;
        config.auto_calibration = Am18x5AutoCalibration::AUTO_CAL_DISABLE;
        if (Am18x5::getInstance().setConfig(&config) != SYSTEM_ERROR_NONE) {
            return SYSTEM_ERROR_INTERNAL;
        }
    }
    return Am18x5::getInstance().begin();
}

int hal_exrtc_set_time(const struct timeval* tv, void* reserved) {
    return Am18x5::getInstance().setTime(tv);
}

int hal_exrtc_get_time(struct timeval* tv, void* reserved) {
    return Am18x5::getInstance().getTime(tv);
}

int hal_exrtc_set_alarm(const struct timeval* tv, uint32_t flags, hal_exrtc_alarm_handler handler, void* context, void* reserved) {
    return Am18x5::getInstance().setAlarm(true, flags, tv, handler, context);
}

int hal_exrtc_cancel_alarm(void* reserved) {
    return Am18x5::getInstance().setAlarm(false);
}

bool hal_exrtc_time_is_valid(void* reserved) {
    struct timeval tv;
    if (!hal_exrtc_get_time(&tv, nullptr)) {
        return tv.tv_sec > UNIX_TIME_201801010000;
    }
    return false;
}

int hal_exrtc_enable_watchdog(system_tick_t ms, void* reserved) {
    return Am18x5::getInstance().enableWatchdog(ms);
}

int hal_exrtc_disable_watchdog(void* reserved) {
    return Am18x5::getInstance().disableWatchdog();
}

int hal_exrtc_feed_watchdog(void* reserved) {
    return Am18x5::getInstance().feedWatchdog();
}

int hal_exrtc_sleep_timer(system_tick_t ms, void* reserved) {
    hal_am18x5_sleep_config_t sleepConfig = {
        .version = HAL_AM18X5_CONFIG_VERSION,
        .size = sizeof(hal_am18x5_sleep_config_t),
        .exti_polarity = Am18x5ExtiPolarity::NONE,
        .exti_trigger_latched = false,
        .duration = ms / 1000
    };
    return Am18x5::getInstance().sleep(&sleepConfig);
}

void hal_exrtc_get_watchdog_limits(system_tick_t* low, system_tick_t* high, void* reserved) {
    Am18x5::getInstance().getWatchdogLimits(low, high);
}

int hal_exrtc_set_config(const hal_am18x5_config_t* conf, void* reserved) {
    return Am18x5::getInstance().setConfig(conf);
}

int hal_exrtc_get_config(hal_am18x5_config_t* conf, void* reserved) {
    return Am18x5::getInstance().getConfig(conf);
}

int hal_exrtc_get_id(char* buf, size_t len, void* reserved) {
    return Am18x5::getInstance().getIdString(buf, len);
}

#endif // HAL_PLATFORM_EXTERNAL_RTC
