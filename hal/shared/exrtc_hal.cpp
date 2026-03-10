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
#undef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ALL
#include "hal_platform.h"
#if HAL_PLATFORM_NRF52840
#include "nrf52840.h" // FIXME: pinmap woes
#endif // HAL_PLATFORM_NRF52840

#include "exrtc_hal.h"
#include "exrtc_hal_internal.h"
#include "check.h"
#include "system_cache.h"
#include "spark_wiring_buffer.h"
#include "platforms.h"
#include <algorithm>
#include <iterator>

#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL

#include "am18x5.h"

// FIXME: for now this is all am18x5-specific, but the API itself should be somewhat extensible

using namespace particle;
using namespace particle::services;

namespace {

const uint32_t AM18X5_SUPPORTED_CAPS =
        HAL_EXRTC_CAPS_POWER_GATE |
        HAL_EXRTC_CAPS_CLOCK_SOURCE |
        HAL_EXRTC_CAPS_CLOCK_OUTPUT |
        HAL_EXRTC_CAPS_AUTO_CALIBRATION |
        HAL_EXRTC_CAPS_AUTO_CLOCK_SOURCE_INTERNAL_ON_BATTERY |
        HAL_EXRTC_CAPS_AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL |
        HAL_EXRTC_CAPS_SLEEP |
        HAL_EXRTC_CAPS_EXTI |
        HAL_EXRTC_CAPS_EXTI_LEVEL_TRIGGER |
        HAL_EXRTC_CAPS_WATCHDOG;

struct RtcEventHandler {
    hal_exrtc_event_handler_t handler = nullptr;
    void* context = nullptr;
} g_eventHandler;

int writeMfgXtalCalibration(const hal_exrtc_calibration_data_t* data) {
    CHECK_TRUE(data && data->size >= sizeof(uint16_t) * 2, SYSTEM_ERROR_INVALID_ARGUMENT);
    hal_exrtc_calibration_data_t stored = {};
    stored.version = HAL_EXRTC_API_VERSION;
    stored.size = sizeof(stored);
    stored.value = data->value;
    return SystemCache::instance().set(SystemCacheKey::EXRTC_MFG_XTAL_CALIBRATION, &stored, sizeof(stored));
}

void am18x5OscEventHandler(uint8_t events, void* context) {
    (void)context;
    uint32_t exrtcEvents = HAL_EXRTC_EVENT_NONE;
    if (events & Am18x5OscEvent::XT_OSC_FAILURE) {
        exrtcEvents |= HAL_EXRTC_EVENT_CLOCK_SOURCE_EXTERNAL_FAILURE;
    }
    if (events & Am18x5OscEvent::AUTO_CAL_FAILURE) {
        exrtcEvents |= HAL_EXRTC_EVENT_CALIBRATION_FAILURE;
    }
    if (exrtcEvents && g_eventHandler.handler) {
        g_eventHandler.handler(exrtcEvents, nullptr, g_eventHandler.context);
    }
}

struct RtcBinding {
    RtcBinding() = default;

    void reset() {
        loaded_ = false;
        memset(&device_, 0, sizeof(device_));
        memset(&config_, 0, sizeof(config_));
        memset(&binding_, 0, sizeof(binding_));
        vendor_.resize(0);
    }

    int validate() {
        CHECK_TRUE(device_.type == HAL_EXRTC_TYPE_AM18X5, SYSTEM_ERROR_NOT_SUPPORTED);
        CHECK_TRUE(device_.transport == HAL_EXRTC_TRANSPORT_I2C, SYSTEM_ERROR_NOT_SUPPORTED);
        CHECK_TRUE(device_.i2c.interface < HAL_PLATFORM_I2C_NUM, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(device_.i2c.address == HAL_EXRTC_TYPE_AM18X5_DEFAULT_ADDRESS, SYSTEM_ERROR_NOT_SUPPORTED);
        if (vendor()) {
            CHECK_TRUE(vendor()->type == HAL_EXRTC_TYPE_AM18X5, SYSTEM_ERROR_INVALID_ARGUMENT);
        }
        return 0;
    }

    int load(const hal_exrtc_binding_t* binding) {
        reset();

        CHECK_TRUE(binding && binding->device && binding->config, SYSTEM_ERROR_BAD_DATA);
        CHECK_TRUE(binding->device->size > sizeof(uint16_t) * 2, SYSTEM_ERROR_BAD_DATA);
        CHECK_TRUE(binding->config->size > sizeof(uint16_t) * 2, SYSTEM_ERROR_BAD_DATA);
        if (binding->vendor) {
            CHECK_TRUE(binding->vendor->size >= sizeof(hal_exrtc_vendor_config_t), SYSTEM_ERROR_BAD_DATA);
        }
        memcpy(&device_, binding->device, std::min<size_t>(binding->device->size, sizeof(device_)));
        memcpy(&config_, binding->config, std::min<size_t>(binding->config->size, sizeof(config_)));
        if (binding->vendor) {
            CHECK_TRUE(vendor_.resize(binding->vendor->size), SYSTEM_ERROR_NO_MEMORY);
            memcpy(vendor_.data(), binding->vendor, binding->vendor->size);
        }
        CHECK(validate());
        loaded_ = true;
        return 0;
    }

    int load(SystemCacheKey key) {
        reset();

        const size_t configSize = CHECK(SystemCache::instance().size(key));
        CHECK_TRUE(configSize > 0, SYSTEM_ERROR_BAD_DATA);
        Buffer data;
        CHECK_TRUE(data.resize(configSize), SYSTEM_ERROR_NO_MEMORY);
        CHECK(SystemCache::instance().get(key, data.data(), configSize));
        CHECK_TRUE(data.size() > 0, SYSTEM_ERROR_BAD_DATA);

        size_t pos = 0;

        {
            auto tmp = reinterpret_cast<hal_exrtc_device_t*>(data.data());
            CHECK_TRUE((int)data.size() - pos > sizeof(uint16_t) * 2, SYSTEM_ERROR_BAD_DATA);
            CHECK_TRUE(tmp->size > 0, SYSTEM_ERROR_BAD_DATA);
            memcpy(&device_, tmp, std::min<size_t>(tmp->size, sizeof(device_)));
            pos += tmp->size;
        }

        {
            auto tmp = reinterpret_cast<hal_exrtc_config_t*>(data.data() + pos);
            CHECK_TRUE((int)data.size() - pos > sizeof(uint16_t) * 2, SYSTEM_ERROR_BAD_DATA);
            CHECK_TRUE(tmp->size > 0, SYSTEM_ERROR_BAD_DATA);
            memcpy(&config_, tmp, std::min<size_t>(tmp->size, sizeof(config_)));
            pos += tmp->size;
        }

        ssize_t vendorConfigSize = data.size() - pos;
        CHECK_TRUE(vendorConfigSize == 0 || vendorConfigSize >= (ssize_t)sizeof(hal_exrtc_vendor_config_t), SYSTEM_ERROR_BAD_DATA);
        if (vendorConfigSize > 0) {
            CHECK_TRUE(vendor_.resize(vendorConfigSize), SYSTEM_ERROR_NO_MEMORY);
            memcpy(vendor_.data(), data.data() + pos, vendorConfigSize);
        }
        CHECK(validate());
        loaded_ = true;
        return 0;
    }

    int store(SystemCacheKey key) {
        CHECK_TRUE(loaded_, SYSTEM_ERROR_INVALID_STATE);
        size_t size = device_.size + config_.size + vendor_.size();
        Buffer data;
        CHECK_TRUE(data.resize(size), SYSTEM_ERROR_NO_MEMORY);
        size_t pos = 0;
        memcpy(data.data() + pos, &device_, device_.size);
        pos += device_.size;
        memcpy(data.data() + pos, &config_, config_.size);
        pos += config_.size;
        memcpy(data.data() + pos, vendor_.data(), vendor_.size());
        return SystemCache::instance().set(key, data.data(), size);
    }

    int setConfig(hal_exrtc_config_t* config, hal_exrtc_vendor_config_t* vendor = nullptr) {
        CHECK_TRUE(loaded_, SYSTEM_ERROR_INVALID_STATE);
        CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);
        memcpy(&config_, config, std::min<size_t>(config->size, sizeof(config_)));
        if (vendor) {
            CHECK_TRUE(vendor->size >= sizeof(hal_exrtc_vendor_config_t), SYSTEM_ERROR_INVALID_ARGUMENT);
            CHECK_TRUE(vendor_.resize(vendor->size), SYSTEM_ERROR_NO_MEMORY);
            memcpy(vendor_.data(), vendor, vendor->size);
        }
        return validate();
    }

    hal_exrtc_binding_t* binding() {
        if (!loaded_) {
            return nullptr;
        }
        memset(&binding_, 0, sizeof(binding_));
        binding_.size = sizeof(binding_);
        binding_.version = HAL_EXRTC_API_VERSION;
        binding_.config = config();
        binding_.device = device();
        binding_.vendor = vendor();
        return &binding_;
    }

    hal_exrtc_device_t* device() {
        if (!loaded_) {
            return nullptr;
        }
        return &device_;
    }

    hal_exrtc_config_t* config() {
        if (!loaded_) {
            return nullptr;
        }
        return &config_;
    }

    hal_exrtc_vendor_config_t* vendor() {
        if (!loaded_) {
            return nullptr;
        }
        if (vendor_.size() == 0) {
            return nullptr;
        }
        return reinterpret_cast<hal_exrtc_vendor_config_t*>(vendor_.data());
    }

private:
    bool loaded_ = false;
    hal_exrtc_binding_t binding_ = {};
    hal_exrtc_device_t device_ = {};
    hal_exrtc_config_t config_ = {};
    Buffer vendor_;
};

int loadCurrentBinding(RtcBinding* binding) {
    CHECK_TRUE(binding, SYSTEM_ERROR_INVALID_ARGUMENT);
    if (!binding->load(SystemCacheKey::EXRTC_CONFIG_DATA)) {
        return 0;
    }
    if (auto platformDefault = hal_exrtc_default_binding()) {
        return binding->load(platformDefault);
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

} // anonymous

int hal_exrtc_init(void) {
    RtcBinding stored;
    if (!stored.load(SystemCacheKey::EXRTC_CONFIG_DATA)) {
        return Am18x5::getInstance().bind(stored.binding());
    }

    auto platformDefault = hal_exrtc_default_binding();
    if (platformDefault) {
        return Am18x5::getInstance().bind(platformDefault);
    }

    return SYSTEM_ERROR_NOT_FOUND;
}

int hal_exrtc_bind(hal_exrtc_instance_t instance, const hal_exrtc_binding_t* binding, void* reserved) {
    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(binding, SYSTEM_ERROR_INVALID_ARGUMENT);

    RtcBinding loaded;
    CHECK(loaded.load(binding));
    CHECK(loaded.store(SystemCacheKey::EXRTC_CONFIG_DATA));
    return Am18x5::getInstance().bind(loaded.binding());
}

int hal_exrtc_get_device(hal_exrtc_instance_t instance, hal_exrtc_device_t* device, void* reserved) {
    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(device && device->size >= sizeof(uint16_t) * 2, SYSTEM_ERROR_INVALID_ARGUMENT);

    if (Am18x5::getInstance().isPresent()) {
        return Am18x5::getInstance().getDevice(device);
    }

    RtcBinding binding;
    CHECK(loadCurrentBinding(&binding));
    auto src = binding.device();
    CHECK_TRUE(src, SYSTEM_ERROR_NOT_FOUND);
    memcpy(device, src, std::min<size_t>(device->size, src->size));
    return 0;
}

int hal_exrtc_unbind(hal_exrtc_instance_t instance, void* reserved) {
    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);

    // Ignore errors
    Am18x5::getInstance().end();
    SystemCache::instance().del(SystemCacheKey::EXRTC_CONFIG_DATA);
    return 0;
}

int hal_exrtc_get_status(hal_exrtc_instance_t instance, hal_exrtc_status_t* status, void* reserved, void* reserved1) {
    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(status && status->size >= sizeof(uint16_t) * 2, SYSTEM_ERROR_INVALID_ARGUMENT);

    RtcBinding binding;
    int r = loadCurrentBinding(&binding);
    CHECK_TRUE(!r || r == SYSTEM_ERROR_NOT_FOUND, r);

    hal_exrtc_status_t tmp = {};
    tmp.size = sizeof(tmp);
    tmp.version = HAL_EXRTC_API_VERSION;
    if (Am18x5::getInstance().isPresent()) {
        CHECK(Am18x5::getInstance().getStatus(&tmp));
    } else {
        tmp.type = HAL_EXRTC_TYPE_AM18X5;
        tmp.caps_supported = AM18X5_SUPPORTED_CAPS;
        if (!r) {
            tmp.clock_source = binding.config()->clock_source;
            tmp.caps_enabled = binding.config()->caps_enable;
        }
    }
    if (!r) {
        tmp.status |= HAL_EXRTC_STATUS_BOUND;
    }
    if (hal_exrtc_default_binding()) {
        tmp.status |= HAL_EXRTC_STATUS_BUILT_IN;
    }
    memcpy(status, &tmp, std::min<size_t>(status->size, sizeof(tmp)));
    return 0;
}

int hal_exrtc_set_config(hal_exrtc_instance_t instance, const hal_exrtc_config_t* config, const hal_exrtc_vendor_config_t* vendor, void* reserved) {
    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);

    RtcBinding binding;
    CHECK(loadCurrentBinding(&binding));
    CHECK(binding.setConfig(const_cast<hal_exrtc_config_t*>(config), const_cast<hal_exrtc_vendor_config_t*>(vendor)));
    CHECK(binding.store(SystemCacheKey::EXRTC_CONFIG_DATA));
    return Am18x5::getInstance().bind(binding.binding());
}

int hal_exrtc_get_config(hal_exrtc_instance_t instance, hal_exrtc_config_t* config, hal_exrtc_vendor_config_t* vendor, void* reserved) {
    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);

    if (Am18x5::getInstance().isPresent()) {
        return Am18x5::getInstance().getConfig(config, vendor);
    }

    RtcBinding binding;
    CHECK(loadCurrentBinding(&binding));
    auto srcConfig = binding.config();
    CHECK_TRUE(srcConfig, SYSTEM_ERROR_NOT_FOUND);
    memcpy(config, srcConfig, std::min<size_t>(config->size, srcConfig->size));
    if (vendor) {
        auto srcVendor = binding.vendor();
        CHECK_TRUE(srcVendor, SYSTEM_ERROR_NOT_FOUND);
        memcpy(vendor, srcVendor, std::min<size_t>(vendor->size, srcVendor->size));
    }
    return 0;
}

void* hal_exrtc_event_handler_add(hal_exrtc_instance_t instance, hal_exrtc_event_handler_t handler, void* context, void* reserved) {
    if (instance != HAL_EXRTC_INSTANCE_1) {
        return nullptr;
    }
    g_eventHandler.handler = handler;
    g_eventHandler.context = context;
    if (!Am18x5::getInstance().isPresent()) {
        return handler ? nullptr : &g_eventHandler;
    }
    uint8_t events = handler ? (Am18x5OscEvent::XT_OSC_FAILURE | Am18x5OscEvent::AUTO_CAL_FAILURE) : 0;
    if (Am18x5::getInstance().onOscillatorEvent(events, handler ? am18x5OscEventHandler : nullptr, nullptr)) {
        return nullptr;
    }
    return &g_eventHandler;
}

int hal_exrtc_command(hal_exrtc_instance_t instance, hal_exrtc_command_t cmd, void* arg, void* arg1, void* reserved) {
    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);
    if (cmd == HAL_EXRTC_COMMAND_WRITE_MFG_XTAL_CALIBRATION) {
        return writeMfgXtalCalibration(static_cast<const hal_exrtc_calibration_data_t*>(arg));
    }
    return Am18x5::getInstance().command(cmd, arg, arg1);
}

// Just mimicking rtc_hal to simplify rtc_hal <-> exrtc_hal coupling

int hal_exrtc_get_time_internal(struct timeval* tv) {
    if (Am18x5::getInstance().isPresent()) {
        return Am18x5::getInstance().getTime(tv);
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int hal_exrtc_set_time_internal(const struct timeval* tv) {
    if (Am18x5::getInstance().isPresent()) {
        return Am18x5::getInstance().setTime(tv);
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int hal_exrtc_set_alarm(const struct timeval* tv, uint32_t flags, hal_rtc_alarm_handler handler, void* context) {
    if (Am18x5::getInstance().isPresent()) {
        return Am18x5::getInstance().setAlarm(true, flags, tv, handler, context);
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int hal_exrtc_cancel_alarm(void) {
    if (Am18x5::getInstance().isPresent()) {
        return Am18x5::getInstance().setAlarm(false);
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

bool hal_exrtc_is_default(void) {
    return Am18x5::getInstance().isDefault();
}

hal_exrtc_binding_t* __attribute__((weak)) hal_exrtc_default_binding() {
    return nullptr;
}

#endif // HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
