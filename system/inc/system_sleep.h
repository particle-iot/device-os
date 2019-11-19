/**
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

#ifndef SYSTEM_SLEEP_H
#define SYSTEM_SLEEP_H

#include <stdint.h>
#include <stddef.h>
#include <chrono>
#include <memory>
#include <string.h>
#include "system_network.h"
#include "interrupts_hal.h"
#include "sleep_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SLEEP_MODE_WLAN = 0, SLEEP_MODE_DEEP = 1, SLEEP_MODE_SOFTPOWEROFF = 2
} Spark_Sleep_TypeDef;

typedef enum System_Sleep_Flag
{
    SYSTEM_SLEEP_FLAG_NETWORK_OFF = 0x00,
    SYSTEM_SLEEP_FLAG_NETWORK_STANDBY = 0x01,
    SYSTEM_SLEEP_FLAG_DISABLE_WKP_PIN = 0x02,
    SYSTEM_SLEEP_FLAG_NO_WAIT = 0x04
} System_Sleep_Flag;

enum WakeupReason {
    WAKEUP_REASON_NONE = 0,
    WAKEUP_REASON_PIN = 1,
    WAKEUP_REASON_RTC = 2,
    WAKEUP_REASON_PIN_OR_RTC = 3
};


enum class SystemSleepMode: uint8_t {
    NONE            = HAL_SLEEP_MODE_NONE,
    STOP            = HAL_SLEEP_MODE_STOP,
    ULTRA_LOW_POWER = HAL_SLEEP_MODE_ULTRA_LOW_POWER,
    HIBERNATE       = HAL_SLEEP_MODE_HIBERNATE,
};

enum class SystemSleepWait: uint8_t {
    NO_WAIT = HAL_SLEEP_WAIT_NO_WAIT,
    CLOUD   = HAL_SLEEP_WAIT_CLOUD
};

class SystemSleepConfigurationHelper {
public:
    SystemSleepConfigurationHelper(hal_sleep_config_t* config)
        : config_(config) {
    }

    bool cloudDisconnectRequested() const {
        // TODO: As long as one of the network interfaces is specified as wakeup source,
        // we should not disconnect from the cloud
        return true; 
    }

    bool networkDisconnectRequested(network_interface_index index) const {
        // TODO: If the given network interface is not specified as wakeup source, suspend it.
        return true;
    }

    hal_sleep_config_t* halConfig() const {
        return config_;
    }

    SystemSleepWait sleepWait() const {
        return static_cast<SystemSleepWait>(config_->wait);
    }

    SystemSleepMode sleepMode() const {
        return static_cast<SystemSleepMode>(config_->mode);
    }

    const hal_wakeup_source_base_t* wakeupSourceFeatured(hal_wakeup_source_type_t type, hal_wakeup_source_base_t* start = nullptr) const {
        return wakeupSourceFeatured(type, start);
    }

    hal_wakeup_source_base_t* wakeupSourceFeatured(hal_wakeup_source_type_t type, hal_wakeup_source_base_t* start = nullptr) {
        if (!start) {
            start = config_->wakeup_sources;
        }
        while (start) {
            if (start->type == type) {
                return start;
            }
            start = start->next;
        }
        return nullptr;
    }

private:
    hal_sleep_config_t* config_;
};

class SystemSleepConfiguration: public SystemSleepConfigurationHelper {
public:
    // Constructor
    SystemSleepConfiguration()
            : SystemSleepConfigurationHelper(&config_),
              config_(),
              valid_(false) {
        config_.size = sizeof(hal_sleep_config_t);
        config_.version = HAL_SLEEP_VERSION;
        config_.mode = HAL_SLEEP_MODE_NONE;
        config_.wait = HAL_SLEEP_WAIT_NO_WAIT;
        config_.wakeup_sources = nullptr;
    }

    // Move constructor
    SystemSleepConfiguration(SystemSleepConfiguration&& config)
            : SystemSleepConfigurationHelper(&config_),
              valid_(config.valid_) {
        memcpy(&config_, &config.config_, sizeof(hal_sleep_config_t));
        config.config_.wakeup_sources = nullptr;
    }

    // Copy constructor
    SystemSleepConfiguration(const SystemSleepConfiguration& config) = delete;

    // Copy operator
    SystemSleepConfiguration& operator=(const SystemSleepConfiguration& config) = delete;

    // Destructor
    ~SystemSleepConfiguration() {
        // Free memory
        auto wakeupSource = config_.wakeup_sources;
        while (wakeupSource) {
            auto next = wakeupSource->next;
            delete wakeupSource;
            wakeupSource = next;
        }
    }

    bool valid() const {
        return valid_;
    }

    // Setters
    SystemSleepConfiguration& mode(SystemSleepMode mode) {
        config_.mode = static_cast<hal_sleep_mode_t>(mode);
        return *this;
    }

    SystemSleepConfiguration& wait(SystemSleepWait wait) {
        config_.wait = static_cast<hal_sleep_wait_t>(wait);
        return *this;
    }

    SystemSleepConfiguration& gpio(pin_t pin, InterruptMode mode) {
        // Check if this pin has been featured.
        auto wakeup = wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_GPIO);
        while (wakeup) {
            auto gpioWakeup = reinterpret_cast<hal_wakeup_source_gpio_t*>(wakeup);
            if (gpioWakeup->pin == pin) {
                gpioWakeup->mode = mode;
                return *this;
            }
            wakeup = wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_GPIO, wakeup->next);
        }
        // Otherwise, configure this pin as wakeup source.
        auto wakeupSource = new(std::nothrow) hal_wakeup_source_gpio_t();
        if (!wakeupSource) {
            valid_ = false;
            return *this;
        }
        wakeupSource->base.size = sizeof(hal_wakeup_source_gpio_t);
        wakeupSource->base.version = HAL_SLEEP_VERSION;
        wakeupSource->base.type = HAL_WAKEUP_SOURCE_TYPE_GPIO;
        wakeupSource->base.next = config_.wakeup_sources;
        wakeupSource->pin = pin;
        wakeupSource->mode = mode;
        config_.wakeup_sources = reinterpret_cast<hal_wakeup_source_base_t*>(wakeupSource);
        valid_= true;
        return *this;
    }

    SystemSleepConfiguration& duration(system_tick_t ms) {
        // Check if RTC has been configured as wakeup source.
        auto wakeup = wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_RTC);
        if (wakeup) {
            reinterpret_cast<hal_wakeup_source_rtc_t*>(wakeup)->ms = ms;
            return *this;
        }
        // Otherwise, configure RTC as wakeup source.
        auto wakeupSource = new(std::nothrow) hal_wakeup_source_rtc_t();
        if (!wakeupSource) {
            valid_ = false;
            return *this;
        }
        wakeupSource->base.size = sizeof(hal_wakeup_source_rtc_t);
        wakeupSource->base.version = HAL_SLEEP_VERSION;
        wakeupSource->base.type = HAL_WAKEUP_SOURCE_TYPE_RTC;
        wakeupSource->base.next = config_.wakeup_sources;
        wakeupSource->ms = ms;
        config_.wakeup_sources = reinterpret_cast<hal_wakeup_source_base_t*>(wakeupSource);
        valid_ = true;
        return *this;
    }

    SystemSleepConfiguration& duration(std::chrono::milliseconds ms) {
        return duration(ms.count());
    }

private:
    hal_sleep_config_t config_;
    bool valid_; // TODO: This should be an enum value instead to indicate different errors.
};


/**
 * @param param A SystemSleepNetwork enum cast as an integer.
 */
int system_sleep(Spark_Sleep_TypeDef mode, long seconds, uint32_t param, void* reserved);
int system_sleep_pin(uint16_t pin, uint16_t mode, long seconds, uint32_t param, void* reserved);
int system_sleep_pins(const uint16_t* pins, size_t pins_count, const InterruptMode* modes, size_t modes_count, long seconds, uint32_t param, void* reserved);
int system_sleep_ext(hal_sleep_config_t* config, WakeupReason*, void* reserved);

#ifdef __cplusplus
}
#endif


#endif /* SYSTEM_SLEEP_H */

