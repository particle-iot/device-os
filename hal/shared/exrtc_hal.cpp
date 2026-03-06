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

#include "exrtc_hal.h"
#include "exrtc_hal_internal.h"
#include "check.h"
#include "system_cache.h"
#include "spark_wiring_buffer.h"
#include "platforms.h"
#include <algorithm>

#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL

#include "am18x5.h"

// FIXME: for now this is all am18x5-specific, but the API itself should be somewhat extensible

using namespace particle;
using namespace particle::services;

namespace {

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
        return 0;
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

} // anonymous

int hal_exrtc_init(void) {
    RtcBinding stored;
    if (!stored.load(SystemCacheKey::EXRTC_CONFIG_DATA)) {
        // if (!Am18x5::getInstance().begin(stored.binding())) {
        //     // Using stored configuration
        //     return 0;
        // }
    }

#if HAL_PLATFORM_EXTERNAL_RTC
    auto platformDefault = hal_exrtc_default_binding();
    (void)platformDefault;
    // if (platformDefault) {
    //     return Am18x5::getInstance().begin(platformDefault); // Use defaults
    // }
#endif // HAL_PLATFORM_EXTERNAL_RTC

    return SYSTEM_ERROR_NOT_FOUND;
}

int hal_exrtc_bind(hal_exrtc_instance_t instance, const hal_exrtc_binding_t* binding, void* reserved) {
    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(binding, SYSTEM_ERROR_INVALID_ARGUMENT);

    RtcBinding loaded;
    CHECK(loaded.load(binding));
    CHECK(loaded.store(SystemCacheKey::EXRTC_CONFIG_DATA));

    // return Am18x5::getInstance().begin(loaded.binding());
    return 0;
}

int hal_exrtc_get_device(hal_exrtc_instance_t instance, hal_exrtc_device_t* device, void* reserved) {
    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);

    //return Am18x5::getInstance().getDevice(device);
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

    // return Am18x5::getInstance().getStatus(status);
    return 0;
}

int hal_exrtc_set_config(hal_exrtc_instance_t instance, const hal_exrtc_config_t* config, const hal_exrtc_vendor_config_t* vendor, void* reserved) {
    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);

    // return Am18x5::getInstance().setConfig(config, vendor);
    return 0;
}

int hal_exrtc_get_config(hal_exrtc_instance_t instance, hal_exrtc_config_t* config, hal_exrtc_vendor_config_t* vendor, void* reserved) {
    CHECK_TRUE(instance == HAL_EXRTC_INSTANCE_1, SYSTEM_ERROR_INVALID_ARGUMENT);

    // return Am18x5::getInstance().getConfig(config, vendor);
    return 0;
}

void* hal_exrtc_event_handler_add(hal_exrtc_instance_t instance, hal_exrtc_event_handler_t handler, void* context, void* reserved) {
    if (instance != HAL_EXRTC_INSTANCE_1) {
        return nullptr;
    }

    // return Am18x5::getInstance().onEvent(handler, context);
    return 0;
}

int hal_exrtc_command(hal_exrtc_instance_t instance, hal_exrtc_command_t cmd, void* arg, void* arg1, void* reserved) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
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
