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
#include "interrupts_hal.h"

#if HAL_PLATFORM_SLEEP_2_0
#include <chrono>
#include <memory>
#include <string.h>
#include <stdlib.h>
#include "system_network.h"
#include "system_error.h"
#include "sleep_hal.h"
#include "spark_wiring_network.h"

using spark::NetworkClass; 
#endif // HAL_PLATFORM_SLEEP_2_0

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

/**
 * @param param A SystemSleepNetwork enum cast as an integer.
 */
int system_sleep(Spark_Sleep_TypeDef mode, long seconds, uint32_t param, void* reserved);
int system_sleep_pin(uint16_t pin, uint16_t mode, long seconds, uint32_t param, void* reserved);
int system_sleep_pins(const uint16_t* pins, size_t pins_count, const InterruptMode* modes, size_t modes_count, long seconds, uint32_t param, void* reserved);


#if HAL_PLATFORM_SLEEP_2_0

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

enum class SystemSleepWakeupReason: uint16_t {
    UNKNOWN = HAL_WAKEUP_SOURCE_TYPE_UNKNOWN,
    BY_GPIO = HAL_WAKEUP_SOURCE_TYPE_GPIO,
    BY_ADC = HAL_WAKEUP_SOURCE_TYPE_ADC,
    BY_DAC = HAL_WAKEUP_SOURCE_TYPE_DAC,
    BY_RTC = HAL_WAKEUP_SOURCE_TYPE_RTC,
    BY_LPCOMP = HAL_WAKEUP_SOURCE_TYPE_LPCOMP,
    BY_UART = HAL_WAKEUP_SOURCE_TYPE_UART,
    BY_I2C = HAL_WAKEUP_SOURCE_TYPE_I2C,
    BY_SPI = HAL_WAKEUP_SOURCE_TYPE_SPI,
    BY_TIMER = HAL_WAKEUP_SOURCE_TYPE_TIMER,
    BY_CAN = HAL_WAKEUP_SOURCE_TYPE_CAN,
    BY_USB = HAL_WAKEUP_SOURCE_TYPE_USB,
    BY_BLE = HAL_WAKEUP_SOURCE_TYPE_BLE,
    BY_NFC = HAL_WAKEUP_SOURCE_TYPE_NFC,
    BY_NETWORK = HAL_WAKEUP_SOURCE_TYPE_NETWORK,
};

class SystemSleepConfigurationHelper {
public:
    SystemSleepConfigurationHelper(const hal_sleep_config_t* config)
        : config_(config) {
    }

    bool cloudDisconnectRequested() const {
#if HAL_PLATFORM_CELLULAR
        if (wakeupByNetworkInterface(NETWORK_INTERFACE_CELLULAR)) {
            return false;
        }
#endif // HAL_PLATFORM_CELLULAR

#if HAL_PLATFORM_WIFI
        if (wakeupByNetworkInterface(NETWORK_INTERFACE_WIFI_STA)) {
            return false;
        }
#endif // HAL_PLATFORM_WIFI

#if HAL_PLATFORM_MESH
        if (wakeupByNetworkInterface(NETWORK_INTERFACE_MESH)) {
            return false;
        }
#endif // HAL_PLATFORM_MESH

#if HAL_PLATFORM_ETHERNET
        if (wakeupByNetworkInterface(NETWORK_INTERFACE_ETHERNET)) {
            return false;
        }
#endif // HAL_PLATFORM_ETHERNET

        return true; 
    }

    bool wakeupByNetworkInterface(network_interface_index index) const {
        auto wakeup = wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_NETWORK);
        while (wakeup) {
            auto networkWakeup = reinterpret_cast<const hal_wakeup_source_network_t*>(wakeup);
            if (networkWakeup->index == index) {
                return true;
            }
            wakeup = wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_NETWORK, wakeup->next);
        }
        return false;
    }

    SystemSleepWait sleepWait() const {
        return static_cast<SystemSleepWait>(config_->wait);
    }

    SystemSleepMode sleepMode() const {
        return static_cast<SystemSleepMode>(config_->mode);
    }

    hal_wakeup_source_base_t* wakeupSource() const {
        return config_->wakeup_sources;
    }

    hal_wakeup_source_base_t* wakeupSourceFeatured(hal_wakeup_source_type_t type) const {
        return wakeupSourceFeatured(type, config_->wakeup_sources);
    }

    hal_wakeup_source_base_t* wakeupSourceFeatured(hal_wakeup_source_type_t type, hal_wakeup_source_base_t* start) const {
        if (!start) {
            return nullptr;
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
    const hal_sleep_config_t* config_;
};

class SystemSleepConfiguration: protected SystemSleepConfigurationHelper {
public:
    // Constructor
    SystemSleepConfiguration()
            : SystemSleepConfigurationHelper(&config_),
              config_(),
              valid_(true) {
        config_.size = sizeof(hal_sleep_config_t);
        config_.version = HAL_SLEEP_VERSION;
        config_.mode = HAL_SLEEP_MODE_NONE;
        config_.wait = HAL_SLEEP_WAIT_CLOUD;
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

    // Copy assignment operator
    SystemSleepConfiguration& operator=(const SystemSleepConfiguration& config) = delete;

    // move assignment operator
    SystemSleepConfiguration& operator=(SystemSleepConfiguration&& config) {
        valid_ = config.valid_;
        memcpy(&config_, &config.config_, sizeof(hal_sleep_config_t));
        config.config_.wakeup_sources = nullptr;
        return *this;
    }

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

    const hal_sleep_config_t* halConfig() const {
        return &config_;
    }

    // It doesn't guarantee the combination of sleep mode and
    // wakeup sources that the platform supports.
    bool valid() const {
        if (!valid_) {
            return valid_;
        }
        if (sleepMode() == SystemSleepMode::NONE) {
            return false;
        }
        // Wakeup source can be nullptr herein. HAL sleep API will check
        // if target platform supports not being woken up by any wakeup source.
        return true;
    }

    // Setters
    SystemSleepConfiguration& mode(SystemSleepMode mode) {
        if (valid_) {
            config_.mode = static_cast<hal_sleep_mode_t>(mode);
        }
        return *this;
    }

    SystemSleepConfiguration& wait(SystemSleepWait wait) {
        if (valid_) {
            config_.wait = static_cast<hal_sleep_wait_t>(wait);
        }
        return *this;
    }

    SystemSleepConfiguration& gpio(pin_t pin, InterruptMode mode) {
        if (valid_) {
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
        }
        return *this;
    }

    SystemSleepConfiguration& duration(system_tick_t ms) {
        if (valid_) {
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
        }
        return *this;
    }

    SystemSleepConfiguration& duration(std::chrono::milliseconds ms) {
        return duration(ms.count());
    }

    SystemSleepConfiguration& network(const NetworkClass& network) {
        if (valid_) {
            auto index = static_cast<network_interface_t>(network);
            auto wakeup = wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_NETWORK);
            while (wakeup) {
                auto networkWakeup = reinterpret_cast<hal_wakeup_source_network_t*>(wakeup);
                if (networkWakeup->index == index) {
                    return *this;
                }
                wakeup = wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_NETWORK, wakeup->next);
            }
            auto wakeupSource = new(std::nothrow) hal_wakeup_source_network_t();
            if (!wakeupSource) {
                valid_ = false;
                return *this;
            }
            wakeupSource->base.size = sizeof(hal_wakeup_source_gpio_t);
            wakeupSource->base.version = HAL_SLEEP_VERSION;
            wakeupSource->base.type = HAL_WAKEUP_SOURCE_TYPE_NETWORK;
            wakeupSource->base.next = config_.wakeup_sources;
            wakeupSource->index = static_cast<network_interface_index>(index);
            config_.wakeup_sources = reinterpret_cast<hal_wakeup_source_base_t*>(wakeupSource);
        }
        return *this;
    }

#if HAL_PLATFORM_BLE
    SystemSleepConfiguration& ble() {
        if (valid_) {
            // Check if BLE has been configured as wakeup source.
            auto wakeup = wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_BLE);
            if (wakeup) {
                return *this;
            }
            // Otherwise, configure BLE as wakeup source.
            auto wakeupSource = new(std::nothrow) hal_wakeup_source_base_t();
            if (!wakeupSource) {
                valid_ = false;
                return *this;
            }
            wakeupSource->size = sizeof(hal_wakeup_source_base_t);
            wakeupSource->version = HAL_SLEEP_VERSION;
            wakeupSource->type = HAL_WAKEUP_SOURCE_TYPE_BLE;
            wakeupSource->next = config_.wakeup_sources;
            config_.wakeup_sources = wakeupSource;
        }
        return *this;
    }
#endif // HAL_PLATFORM_BLE

private:
    hal_sleep_config_t config_;
    bool valid_;
};

class SystemSleepResult {
public:
    SystemSleepResult()
            : wakeupSource_(nullptr),
              error_(SYSTEM_ERROR_NONE) {
    }

    SystemSleepResult(hal_wakeup_source_base_t* source, system_error_t error)
            : SystemSleepResult() {
        copyWakeupSource(source);
        error_ = error;
    }

    SystemSleepResult(system_error_t error)
            : SystemSleepResult() {
        error_ = error;
    }

    // Copy constructor
    SystemSleepResult(const SystemSleepResult& result)
            : SystemSleepResult() {
        error_ = result.error_;
        copyWakeupSource(result.wakeupSource_);
    }

    SystemSleepResult& operator=(const SystemSleepResult& result) {
        error_ = result.error_;
        copyWakeupSource(result.wakeupSource_);
        return *this;
    }

    // Move constructor
    SystemSleepResult(SystemSleepResult&& result)
            : SystemSleepResult() {
        error_ = result.error_;
        freeWakeupSourceMemory();
        if (result.wakeupSource_) {
            wakeupSource_ = result.wakeupSource_;
            result.wakeupSource_ = nullptr;
        }
    }

    ~SystemSleepResult() {
        freeWakeupSourceMemory();
    }

    hal_wakeup_source_base_t** halWakeupSource() {
        return &wakeupSource_;
    }

    SystemSleepWakeupReason wakeupReason() const {
        if (wakeupSource_) {
            return static_cast<SystemSleepWakeupReason>(wakeupSource_->type);
        } else {
            return SystemSleepWakeupReason::UNKNOWN;
        }
    }

    pin_t wakeupPin() const {
        if (wakeupReason() == SystemSleepWakeupReason::BY_GPIO) {
            return reinterpret_cast<hal_wakeup_source_gpio_t*>(wakeupSource_)->pin;
        } else {
            return std::numeric_limits<pin_t>::max();
        }
    }

    void setError(system_error_t error) {
        error_ = error;
    }

    system_error_t error() const {
        return error_;
    }

private:
    void freeWakeupSourceMemory() {
        if (wakeupSource_) {
            free(wakeupSource_);
            wakeupSource_ = nullptr;
        }
    }

    int copyWakeupSource(hal_wakeup_source_base_t* source) {
        freeWakeupSourceMemory();
        if (source) {
            wakeupSource_ = (hal_wakeup_source_base_t*)malloc(source->size);
            if (wakeupSource_) {
                memcpy(wakeupSource_, source, source->size);
            } else {
                return SYSTEM_ERROR_NO_MEMORY;
            }
        }
        return SYSTEM_ERROR_NONE;
    }

    hal_wakeup_source_base_t* wakeupSource_;
    system_error_t error_;
};

int system_sleep_ext(const hal_sleep_config_t* config, hal_wakeup_source_base_t** reason, void* reserved);

#endif // HAL_PLATFORM_SLEEP_2_0

#ifdef __cplusplus
}
#endif


#endif /* SYSTEM_SLEEP_H */

