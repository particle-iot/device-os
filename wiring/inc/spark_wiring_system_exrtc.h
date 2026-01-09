/*
 * Copyright (c) 2025 Particle Industries, Inc.  All rights reserved.
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

#pragma once

#include "exrtc_hal.h"
#include "spark_wiring_platform.h"

#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL

namespace particle {

class SystemExternalRtcConfiguration {
public:

    SystemExternalRtcConfiguration()
            : conf_{} {
        conf_.version = HAL_AM18X5_CONFIG_VERSION;
        conf_.size = sizeof(conf_);
        conf_.wdi_pin = PIN_INVALID;
        conf_.int_pin = PIN_INVALID;
    }

    SystemExternalRtcConfiguration(SystemExternalRtcConfiguration&&) = default;
    SystemExternalRtcConfiguration(const hal_am18x5_config_t& conf) : conf_(conf) {}
    SystemExternalRtcConfiguration& operator=(SystemExternalRtcConfiguration&&) = default;

    SystemExternalRtcConfiguration& defaultRtc(bool val) {
        conf_.default_rtc = val;
        return *this;
    }

    bool defaultRtc() const {
        return conf_.default_rtc != 0;
    }

    SystemExternalRtcConfiguration& watchdogInputPin(uint8_t pin) {
        conf_.wdi_pin = pin;
        return *this;
    }

    uint8_t watchdogInputPin() const {
        return conf_.wdi_pin;
    }

    SystemExternalRtcConfiguration& interruptPin(uint8_t pin) {
        conf_.int_pin = pin;
        return *this;
    }

    uint8_t interruptPin() const {
        return conf_.int_pin;
    }

    SystemExternalRtcConfiguration& i2cInterface(hal_i2c_interface_t i2c) {
        conf_.i2c_if = i2c;
        return *this;
    }

    hal_i2c_interface_t i2cInterface() const {
        return conf_.i2c_if;
    }

    SystemExternalRtcConfiguration& rcFallbackOnXtalFailure(bool val) {
        conf_.rc_fallback = val;
        return *this;
    }

    bool rcFallbackOnXtalFailure() const {
        return conf_.rc_fallback != 0;
    }

    SystemExternalRtcConfiguration& rcOnBatteryPowered(bool val) {
        conf_.rc_on_battery = val;
        return *this;
    }

    bool rcOnBatteryPowered() const {
        return conf_.rc_on_battery != 0;
    }

    SystemExternalRtcConfiguration& oscSource(Am18x5Oscillator oscSrc) {
        conf_.osc_src = oscSrc;
        return *this;
    }

    Am18x5Oscillator oscSource() const {
        return conf_.osc_src;
    }

    SystemExternalRtcConfiguration& xtalCalibrationValue(int8_t val) {
        conf_.osc_cal_xt = val;
        return *this;
    }

    int8_t xtalCalibrationValue() const {
        return conf_.osc_cal_xt;
    }

    const hal_am18x5_config_t* config() const {
        return &conf_;
    }
private:
    hal_am18x5_config_t conf_;
};


class SystemExternalRtcSleepConfiguration {
public:

    SystemExternalRtcSleepConfiguration()
            : conf_{} {
        conf_.version = HAL_AM18X5_CONFIG_VERSION;
        conf_.size = sizeof(conf_);
    }

    SystemExternalRtcSleepConfiguration(SystemExternalRtcSleepConfiguration&&) = default;
    SystemExternalRtcSleepConfiguration(const hal_am18x5_sleep_config_t& conf) : conf_(conf) {}
    SystemExternalRtcSleepConfiguration& operator=(SystemExternalRtcSleepConfiguration&&) = default;

    SystemExternalRtcSleepConfiguration& extiTriggerLatched(bool latched) {
        conf_.exti_trigger_latched = latched;
        return *this;
    }

    bool extiTriggerLatched() const {
        return conf_.exti_trigger_latched;
    }

    SystemExternalRtcSleepConfiguration& extiPolarity(Am18x5ExtiPolarity polarity) {
        conf_.exti_polarity = polarity;
        return *this;
    }

    Am18x5ExtiPolarity extiPolarity() const {
        return conf_.exti_polarity;
    }

    SystemExternalRtcSleepConfiguration& duration(system_tick_t duration) {
        conf_.duration = duration;
        return *this;
    }

    SystemExternalRtcSleepConfiguration& duration(std::chrono::milliseconds ms) {
        system_tick_t seconds = ms.count() / 1000;
        return duration(seconds);
    }

    uint8_t duration() const {
        return conf_.duration;
    }

    const hal_am18x5_sleep_config_t* config() const {
        return &conf_;
    }
private:
    hal_am18x5_sleep_config_t conf_;
};

} // particle

#endif // HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL