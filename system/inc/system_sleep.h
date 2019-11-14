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


enum class SystemSleepMode: uint16_t {
    NONE        = HAL_SLEEP_MODE_NONE,
    STOP        = HAL_SLEEP_MODE_STOP,
    HIBERNATE   = HAL_SLEEP_MODE_HIBERNATE,
    SHUTDOWN    = HAL_SLEEP_MODE_SHUTDOWN
};

class SystemSleepConfiguration {
public:
    // Constructor
    SystemSleepConfiguration()
            : config_(),
              valid_(true) {
        config_ = {};
        config_.size = sizeof(hal_sleep_config_t);
        config_.version = HAL_SLEEP_VERSION;
        config_.mode = HAL_SLEEP_MODE_NONE;
        config_.wakeup_sources = nullptr;
    }
    // Move constructor
    SystemSleepConfiguration(SystemSleepConfiguration&& config) {
        memcpy(&config_, &config.config_, sizeof(hal_sleep_config_t));
        valid_ = config.valid_;
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

    // Getters
    const hal_sleep_config_t* halSleepConfig() const {
        return &config_;
    }

    SystemSleepMode sleepMode() const {
        return static_cast<SystemSleepMode>(config_.mode);
    }

    hal_wakeup_source_base_t* wakeupSourceFeatured(hal_wakeup_source_type_t type, hal_wakeup_source_base_t* start = nullptr) const {
        if (!start) {
            start = config_.wakeup_sources;
        }
        while (start) {
            if (start->type == type) {
                return start;
            }
            start = start->next;
        }
        return nullptr;
    }

    // Setters
    SystemSleepConfiguration& mode(SystemSleepMode mode) {
        config_.mode = static_cast<hal_sleep_mode_t>(mode);
        return *this;
    }

    SystemSleepConfiguration& gpio(pin_t pin, InterruptMode mode) {
        if (!valid_) {
            return *this;
        }
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
        return *this;
    }

    SystemSleepConfiguration& duration(system_tick_t ms) {
        if (!valid_) {
            return *this;
        }
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
        return *this;
    }

    SystemSleepConfiguration& duration(std::chrono::milliseconds ms) {
        return duration(ms.count());
    }

private:
    hal_sleep_config_t config_;
    bool valid_;
};


/**
 * @param param A SystemSleepNetwork enum cast as an integer.
 */
int system_sleep(Spark_Sleep_TypeDef mode, long seconds, uint32_t param, void* reserved);
int system_sleep_pin(uint16_t pin, uint16_t mode, long seconds, uint32_t param, void* reserved);
int system_sleep_pins(const uint16_t* pins, size_t pins_count, const InterruptMode* modes, size_t modes_count, long seconds, uint32_t param, void* reserved);
int system_sleep_ext(const SystemSleepConfiguration& config, void* reserved);

#ifdef __cplusplus
}
#endif


#endif /* SYSTEM_SLEEP_H */

