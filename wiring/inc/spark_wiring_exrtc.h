/*
 * Copyright (c) 2026 Particle Industries, Inc.  All rights reserved.
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

#include "hal_platform.h"

#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL

#include "spark_wiring_time.h"
#include "exrtc_hal.h"
#include "enumflags.h"
#include "enumclass.h"
#include "spark_wiring_i2c.h"
#include "spark_wiring_vector.h"
#include "system_error.h"
#include <algorithm>
#include <type_traits>
#include <iterator>

namespace particle {

enum class RtcType : uint32_t {
    NONE = HAL_EXRTC_TYPE_NONE,
    UNKNOWN = HAL_EXRTC_TYPE_UNKNOWN,
#if HAL_PLATFORM_AM18X5
    AM18X5 = HAL_EXRTC_TYPE_AM18X5,
#endif // HAL_PLATFORM_AM18X5
};

enum class RtcCap : uint32_t {
    NONE = HAL_EXRTC_CAPS_NONE,
    POWER_GATE = HAL_EXRTC_CAPS_POWER_GATE,
    CLOCK_SOURCE = HAL_EXRTC_CAPS_CLOCK_SOURCE,
    CLOCK_OUTPUT = HAL_EXRTC_CAPS_CLOCK_OUTPUT,
    AUTO_CALIBRATION = HAL_EXRTC_CAPS_AUTO_CALIBRATION,
    AUTO_CLOCK_SOURCE_INTERNAL_ON_BATTERY = HAL_EXRTC_CAPS_AUTO_CLOCK_SOURCE_INTERNAL_ON_BATTERY,
    AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL = HAL_EXRTC_CAPS_AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL,
    SLEEP = HAL_EXRTC_CAPS_SLEEP,
    EXTI = HAL_EXRTC_CAPS_EXTI,
    EXTI_LEVEL_TRIGGER = HAL_EXRTC_CAPS_EXTI_LEVEL_TRIGGER,
    WATCHDOG = HAL_EXRTC_CAPS_WATCHDOG,
};
ENABLE_ENUM_CLASS_BITWISE(RtcCap);

using RtcCaps = EnumFlags<RtcCap>;

enum class RtcStatusFlag : uint8_t {
    NONE = HAL_EXRTC_STATUS_NONE,
    BUILT_IN = HAL_EXRTC_STATUS_BUILT_IN,
    BOUND = HAL_EXRTC_STATUS_BOUND,
    PRESENT = HAL_EXRTC_STATUS_PRESENT,
    READY = HAL_EXRTC_STATUS_READY,
};
ENABLE_ENUM_CLASS_BITWISE(RtcStatusFlag);

enum class RtcClockSource : uint8_t {
    NONE = HAL_EXRTC_CLOCK_SOURCE_NONE,
    INTERNAL = HAL_EXRTC_CLOCK_SOURCE_INTERNAL,
    EXTERNAL = HAL_EXRTC_CLOCK_SOURCE_EXTERNAL,
};

using RtcStatusFlags = EnumFlags<RtcStatusFlag>;

class RtcConfigurationTag {
protected:
    ~RtcConfigurationTag() = default;
};

template <typename Concrete>
class RtcConfigurationBase: public RtcConfigurationTag {
public:
    RtcConfigurationBase() {
        device_.size = sizeof(device_);
        device_.version = HAL_EXRTC_API_VERSION;

        // FIXME: when new transports are added
        device_.i2c.pin_int = PIN_INVALID;
        std::fill(std::begin(device_.i2c.pins), std::end(device_.i2c.pins), PIN_INVALID);

        config_.size = sizeof(config_);
        config_.version = HAL_EXRTC_API_VERSION;

        // Default setting
        defaultTimeSource(true);
    }

    bool valid() const {
        return valid_;
    }

    int error() const {
        return error_;
    }

    Concrete& type(RtcType type) {
        device_.type = static_cast<hal_exrtc_type_t>(type);
        return self();
    }

    RtcType type() const {
        return static_cast<RtcType>(device_.type);
    }

    Concrete& capabilities(RtcCaps caps) {
        config_.caps_enable = caps.value();
        return self();
    }

    RtcCaps capabilities() const {
        return RtcCaps::fromUnderlying(config_.caps_enable);
    }

    Concrete& i2c(TwoWire& wire, uint8_t address) {
        device_.transport = HAL_EXRTC_TRANSPORT_I2C;
        device_.i2c.address = address;
        device_.i2c.interface = wire.interface();
        return self();
    }

    TwoWire& interface() const {
        return particle::detail::wireForInterface(device_.i2c.interface);
    }

    uint8_t address() const {
        return device_.i2c.address;
    }

    Concrete& interrupt(hal_pin_t pin) {
        device_.i2c.pin_int = pin;
        return self();
    }

    hal_pin_t interrupt() const {
        return device_.i2c.pin_int;
    }

    Concrete& pins(const Vector<hal_pin_t>& gpios) {
        return pins(gpios.data(), gpios.size());
    }

    Concrete& pins(const hal_pin_t* gpios, size_t count) {
        for (size_t i = 0; i < count; i++) {
            pin(i, gpios[i]);
        }
        return self();
    }

    Concrete& pin(size_t idx, hal_pin_t gpio) {
        if (idx >= sizeof(device_.i2c.pins) / sizeof(device_.i2c.pins[0])) {
            return self();
        }
        device_.i2c.pins[idx] = gpio;
        return self();
    }

    hal_pin_t pin(size_t idx) const {
        if (idx >= sizeof(device_.i2c.pins) / sizeof(device_.i2c.pins[0])) {
            return PIN_INVALID;
        }
        return device_.i2c.pins[idx];
    }

    Concrete& defaultTimeSource(bool state) {
        if (state) {
            config_.flags |= HAL_EXRTC_CONFIG_USE_AS_MAIN_RTC;
        } else {
            config_.flags &= ~(HAL_EXRTC_CONFIG_USE_AS_MAIN_RTC);
        }
        return self();
    }

    bool defaultTimeSource() const {
        return config_.flags & HAL_EXRTC_CONFIG_USE_AS_MAIN_RTC;
    }

    Concrete& sleepExtiCheck(bool state) {
        if (state) {
            config_.flags |= HAL_EXRTC_CONFIG_SLEEP_EXTI_CHECK;
        } else {
            config_.flags &= ~(HAL_EXRTC_CONFIG_SLEEP_EXTI_CHECK);
        }
        return self();
    }

    bool sleepExtiCheck() const {
        return config_.flags & HAL_EXRTC_CONFIG_SLEEP_EXTI_CHECK;
    }

    Concrete& clockSource(RtcClockSource source) {
        config_.clock_source = static_cast<hal_exrtc_clock_source_t>(source);
        return self();
    }

    RtcClockSource clockSource() const {
        return static_cast<RtcClockSource>(config_.clock_source);
    }

    hal_exrtc_binding_t toHalBinding() const {
        hal_exrtc_binding_t binding = {};
        binding.size = sizeof(binding);
        binding.version = HAL_EXRTC_API_VERSION;

        binding.device = const_cast<hal_exrtc_device_t*>(&device_);
        binding.config = const_cast<hal_exrtc_config_t*>(&config_);

        binding.vendor = self().vendorConfig();

        return binding;
    }

    hal_exrtc_vendor_config_t* vendorConfig() const {
        return nullptr;
    }

    hal_exrtc_config_t* halConfig() {
        return &config_;
    }

    hal_exrtc_device_t* halDevice() {
        return &device_;
    }

    hal_exrtc_vendor_config_t* halVendorConfig() {
        return self().halVendorConfigImpl();
    }

    void setResult(int error) {
        error_ = error;
        valid_ = (error == SYSTEM_ERROR_NONE);
    }

    hal_exrtc_vendor_config_t* halVendorConfigImpl() {
        return nullptr;
    }

    hal_exrtc_device_t device_ = {};
    hal_exrtc_config_t config_ = {};
    bool valid_ = true;
    int error_ = SYSTEM_ERROR_NONE;

private:
    Concrete& self() {
        return static_cast<Concrete&>(*this);
    }

    const Concrete& self() const {
        return static_cast<const Concrete&>(*this);
    }
};

class RtcConfiguration : public RtcConfigurationBase<RtcConfiguration> {
public:
    using RtcConfigurationBase<RtcConfiguration>::RtcConfigurationBase;
};

template <typename C>
using IsRtcConfigurationTrait = std::is_base_of<RtcConfigurationTag, typename std::decay<C>::type>;

#if HAL_PLATFORM_AM18X5

class Am18x5Configuration : public RtcConfigurationBase<Am18x5Configuration> {
public:
    using Base = RtcConfigurationBase<Am18x5Configuration>;
    using Base::Base;
    using Base::i2c;

    Am18x5Configuration() {
        vendor_.base.size = sizeof(vendor_);
        vendor_.base.version = HAL_EXRTC_API_VERSION;
        vendor_.base.type = HAL_EXRTC_TYPE_AM18X5;

        this->type(RtcType::AM18X5);
    }

    Am18x5Configuration& i2c(TwoWire& wire) {
        return Base::i2c(wire, HAL_EXRTC_TYPE_AM18X5_DEFAULT_ADDRESS);
    }

    Am18x5Configuration& watchdogPin(hal_pin_t pin) {
        this->pin(0, pin);
        return *this;
    }

    Am18x5Configuration& xtalCalibration(int8_t value) {
        vendor_.xtal_calibration = value;
        vendor_.xtal_calibration_set = true;
        return *this;
    }

     hal_exrtc_vendor_config_t* vendorConfig() const {
        return const_cast<decltype(vendor_.base)*>(&vendor_.base);
    }

    hal_exrtc_vendor_config_t* halVendorConfigImpl() {
        return &vendor_.base;
    }

private:
    hal_exrtc_vendor_config_am18x5_t vendor_ = {};
};

#endif // HAL_PLATFORM_AM18X5

class RtcStatus {
public:
    RtcStatus() = default;
    explicit RtcStatus(hal_exrtc_status_t status)
            : status_(status),
              valid_(true),
              error_(SYSTEM_ERROR_NONE) {
    }

    explicit RtcStatus(int error)
            : valid_(false),
              error_(error) {
    }

    explicit operator bool() const {
        return valid_ && type() != RtcType::NONE;
    }

    bool valid() const {
        return valid_;
    }

    int error() const {
        return error_;
    }

    RtcType type() const {
        return static_cast<RtcType>(status_.type);
    }

    RtcStatusFlags flags() const {
        return RtcStatusFlags::fromUnderlying(status_.status);
    }

    RtcCaps supportedCapabilities() const {
        return RtcCaps::fromUnderlying(status_.caps_supported);
    }

    RtcCaps optionalCapabilities() const {
        return RtcCaps::fromUnderlying(status_.caps_optional);
    }

    RtcCaps enabledCapabilities() const {
        return RtcCaps::fromUnderlying(status_.caps_enabled);
    }

    RtcClockSource clockSource() const {
        return static_cast<RtcClockSource>(status_.clock_source);
    }

    bool builtIn() const {
        return status_.status & HAL_EXRTC_STATUS_BUILT_IN;
    }

    bool bound() const {
        return status_.status & HAL_EXRTC_STATUS_BOUND;
    }

    bool present() const {
        return status_.status & HAL_EXRTC_STATUS_PRESENT;
    }

    bool ready() const {
        return status_.status & HAL_EXRTC_STATUS_READY;
    }

private:
    hal_exrtc_status_t status_ = {};
    bool valid_ = false;
    int error_ = SYSTEM_ERROR_NOT_FOUND;
};

class ExRtcBase: public TimeClass {
public:
    explicit ExRtcBase(hal_exrtc_instance_t instance)
            : TimeClass(),
            instance_(instance) {
    }

    int disable() {
        return hal_exrtc_unbind(instance_, nullptr);
    }

    RtcStatus status() const {
        hal_exrtc_status_t stat = {};
        stat.size = sizeof(stat);
        stat.version = HAL_EXRTC_API_VERSION;
        int r = hal_exrtc_get_status(instance_, &stat, nullptr, nullptr);
        if (!r) {
            return RtcStatus(stat);
        }
        return RtcStatus(r);
    }

protected:
    int bind(const hal_exrtc_binding_t* binding) {
        return hal_exrtc_bind(instance_, binding, nullptr);
    }
    int setConfigCommon(const hal_exrtc_config_t* config, const hal_exrtc_vendor_config_t* vendor) {
        return hal_exrtc_set_config(instance_, config, vendor, nullptr);
    }
    int getConfigCommon(hal_exrtc_config_t* config, hal_exrtc_vendor_config_t* vendor) const {
        return hal_exrtc_get_config(instance_, config, vendor, nullptr);
    }

    hal_exrtc_instance_t instance_;
};

template <typename Config>
class ExRtcT : public ExRtcBase {
public:
    using ConfigT = Config;
    using ExRtcBase::ExRtcBase;

    int enable(const Config& config) {
        if (!config.valid()) {
            return config.error() ? config.error() : SYSTEM_ERROR_INVALID_STATE;
        }
        auto binding = config.toHalBinding();
        return this->bind(&binding);
    }

    template <typename C, typename std::enable_if<IsRtcConfigurationTrait<C>::value &&
            !std::is_same<typename std::decay<C>::type, Config>::value, int>::type = 0>
    int enable(const C& config) {
        if (!config.valid()) {
            return config.error() ? config.error() : SYSTEM_ERROR_INVALID_STATE;
        }
        auto binding = config.toHalBinding();
        return this->bind(&binding);
    }

    int setConfig(const Config& config) {
        if (!config.valid()) {
            return config.error() ? config.error() : SYSTEM_ERROR_INVALID_STATE;
        }
        auto binding = config.toHalBinding();
        return setConfigCommon(binding.config, binding.vendor);
    }

    template <typename C, typename std::enable_if<IsRtcConfigurationTrait<C>::value &&
            !std::is_same<typename std::decay<C>::type, Config>::value, int>::type = 0>
    int setConfig(const C& config) {
        if (!config.valid()) {
            return config.error() ? config.error() : SYSTEM_ERROR_INVALID_STATE;
        }
        auto binding = config.toHalBinding();
        return this->setConfigCommon(binding.config, binding.vendor);
    }

    int getConfig(Config& config) const {
        int r = hal_exrtc_get_device(this->instance_, config.halDevice(), nullptr);
        if (!r) {
            r = this->getConfigCommon(config.halConfig(), config.halVendorConfig());
        }
        config.setResult(r);
        return r;
    }

    Config getConfig() const {
        Config config;
        (void)getConfig(config);
        return config;
    }
};

class ExRtc : public ExRtcT<RtcConfiguration> {
public:
    using Base = ExRtcT<RtcConfiguration>;

    static ExRtc& instance() {
        static ExRtc x(HAL_EXRTC_INSTANCE_1);
        return x;
    }

protected:
    ExRtc(hal_exrtc_instance_t instance)
            : Base(instance) {
    }
};

#define ExternalTime ExRtc::instance()

#if HAL_PLATFORM_AM18X5
class Am18x5Rtc : public ExRtcT<Am18x5Configuration> {
public:
    using Base = ExRtcT<Am18x5Configuration>;
    
    static Am18x5Rtc& instance() {
        static Am18x5Rtc x;
        return x;
    }

protected:
    Am18x5Rtc()
            : Base(HAL_EXRTC_INSTANCE_DEFAULT) {
    }
};

#define Am18x5 Am18x5Rtc::instance()

#endif // HAL_PLATFORM_AM18X5

} // particle

#endif // HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
