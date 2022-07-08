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

#pragma once

#include "sleep_hal.h"
#include <chrono>
#include <memory>
#include <string.h>
#include <stdlib.h>
#include "system_network.h"
#include "system_error.h"
#include "spark_wiring_network.h"
#include "spark_wiring_usartserial.h"
#include "spark_wiring_vector.h"
#include "enumflags.h"
#if HAL_PLATFORM_RTL872X
#include "system_task.h"
#endif

#define SYSTEM_SLEEP_NETWORK_FLAG_SUPPORTED_VER     (3)

using spark::Vector;

namespace particle {

enum class SystemSleepMode: uint8_t {
    NONE            = HAL_SLEEP_MODE_NONE,
    STOP            = HAL_SLEEP_MODE_STOP,
    ULTRA_LOW_POWER = HAL_SLEEP_MODE_ULTRA_LOW_POWER,
    HIBERNATE       = HAL_SLEEP_MODE_HIBERNATE,
};

enum class AnalogInterruptMode: uint8_t {
    ABOVE = HAL_SLEEP_LPCOMP_ABOVE,
    BELOW = HAL_SLEEP_LPCOMP_BELOW,
    CROSS = HAL_SLEEP_LPCOMP_CROSS
};

enum class SystemSleepFlag: uint32_t {
    NONE = HAL_SLEEP_FLAG_NONE,
    WAIT_CLOUD = HAL_SLEEP_FLAG_WAIT_CLOUD
};

enum class SystemSleepNetworkFlag: uint16_t {
    NONE = HAL_SLEEP_NETWORK_FLAG_NONE,
    INACTIVE_STANDBY = HAL_SLEEP_NETWORK_FLAG_INACTIVE_STANDBY
};
ENABLE_ENUM_CLASS_BITWISE(SystemSleepNetworkFlag);

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

#if HAL_PLATFORM_ETHERNET
        if (wakeupByNetworkInterface(NETWORK_INTERFACE_ETHERNET)) {
            return false;
        }
#endif // HAL_PLATFORM_ETHERNET

        return true; 
    }

    bool wakeupByFuelGauge() const {
#if HAL_PLATFORM_FUELGAUGE_MAX17043
        auto wakeup = wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_GPIO);
        while (wakeup) {
            auto gpioWakeup = reinterpret_cast<const hal_wakeup_source_gpio_t*>(wakeup);
            if (gpioWakeup->pin == LOW_BAT_UC) {
                return true;
            }
            wakeup = wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_GPIO, wakeup->next);
        }
#endif // HAL_PLATFORM_FUELGAUGE_MAX17043
        return false;
    }

    bool wakeupByNetworkInterface(network_interface_index index) const {
        auto wakeup = wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_NETWORK);
        while (wakeup) {
            auto networkWakeup = reinterpret_cast<const hal_wakeup_source_network_t*>(wakeup);
            if (networkWakeup->index == index && !(networkWakeup->flags & HAL_SLEEP_NETWORK_FLAG_INACTIVE_STANDBY)) {
                return true;
            }
            wakeup = wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_NETWORK, wakeup->next);
        }
        return false;
    }

    EnumFlags<SystemSleepNetworkFlag> networkFlags(network_interface_index index) const {
        if (config_->version < SYSTEM_SLEEP_NETWORK_FLAG_SUPPORTED_VER) {
            return EnumFlags<SystemSleepNetworkFlag>(SystemSleepNetworkFlag::NONE);
        }
        auto wakeup = wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_NETWORK);
        while (wakeup) {
            auto networkWakeup = reinterpret_cast<const hal_wakeup_source_network_t*>(wakeup);
            if (networkWakeup->index == index) {
                return EnumFlags<SystemSleepNetworkFlag>::fromUnderlying(networkWakeup->flags);
            }
            wakeup = wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_NETWORK, wakeup->next);
        }
        return EnumFlags<SystemSleepNetworkFlag>(SystemSleepNetworkFlag::NONE);
    }

    EnumFlags<SystemSleepFlag> sleepFlags() const {
        return EnumFlags<SystemSleepFlag>::fromUnderlying(config_->flags);
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
        config_.flags = 0;
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
#if HAL_PLATFORM_RTL872X
            system_pool_free(wakeupSource, nullptr);
#else
            delete wakeupSource;
#endif
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

    SystemSleepConfiguration& flag(EnumFlags<SystemSleepFlag> f) {
        if (valid_) {
            config_.flags |= f.value();
        }
        return *this;
    }

    SystemSleepConfiguration& gpio(hal_pin_t pin, InterruptMode mode) {
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
#if HAL_PLATFORM_RTL872X
            auto wakeupSource = (hal_wakeup_source_gpio_t*)system_pool_alloc(sizeof(hal_wakeup_source_gpio_t), nullptr);
#else
            auto wakeupSource = new(std::nothrow) hal_wakeup_source_gpio_t();
#endif
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

    SystemSleepConfiguration& gpios(const Vector<std::pair<hal_pin_t, InterruptMode>>& pins) {
        for (const auto& pin : pins) {
            gpio(pin.first, pin.second);
        }
        return *this;
    }

    SystemSleepConfiguration& gpios(const Vector<hal_pin_t>& pins, InterruptMode mode) {
        for (const auto& pin : pins) {
            gpio(pin, mode);
        }
        return *this;
    }

    SystemSleepConfiguration& gpios(const hal_pin_t* pins, size_t count, InterruptMode mode) {
        for (size_t i = 0; i < count; i++) {
            gpio(pins[i], mode);
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
#if HAL_PLATFORM_RTL872X
            auto wakeupSource = (hal_wakeup_source_rtc_t*)system_pool_alloc(sizeof(hal_wakeup_source_rtc_t), nullptr);
#else
            auto wakeupSource = new(std::nothrow) hal_wakeup_source_rtc_t();
#endif
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

    SystemSleepConfiguration& analog(hal_pin_t pin, uint16_t voltage, AnalogInterruptMode trig) {
        if (valid_) {
            // Check if an analog pin has been configured as wakeup source.
            // Only one analog pin can be configured as wakeup source.
            auto wakeup = wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_LPCOMP);
            if (wakeup) {
                auto lpcomp = reinterpret_cast<hal_wakeup_source_lpcomp_t*>(wakeup);
                lpcomp->pin = pin;
                lpcomp->voltage = voltage;
                lpcomp->trig = static_cast<hal_sleep_lpcomp_trig_t>(trig);
                return *this;
            }
            // Otherwise, configure analog pin as wakeup source.
#if HAL_PLATFORM_RTL872X
            auto wakeupSource = (hal_wakeup_source_lpcomp_t*)system_pool_alloc(sizeof(hal_wakeup_source_lpcomp_t), nullptr);
#else
            auto wakeupSource = new(std::nothrow) hal_wakeup_source_lpcomp_t();
#endif
            if (!wakeupSource) {
                valid_ = false;
                return *this;
            }
            wakeupSource->base.size = sizeof(hal_wakeup_source_lpcomp_t);
            wakeupSource->base.version = HAL_SLEEP_VERSION;
            wakeupSource->base.type = HAL_WAKEUP_SOURCE_TYPE_LPCOMP;
            wakeupSource->base.next = config_.wakeup_sources;
            wakeupSource->pin = pin;
            wakeupSource->voltage = voltage;
            wakeupSource->trig = static_cast<hal_sleep_lpcomp_trig_t>(trig);
            config_.wakeup_sources = reinterpret_cast<hal_wakeup_source_base_t*>(wakeupSource);
        }
        return *this;
    }

    SystemSleepConfiguration& duration(std::chrono::milliseconds ms) {
        return duration(ms.count());
    }

    SystemSleepConfiguration& usart(const USARTSerial& serial) {
        if (valid_) {
            // Check if USART has been configured as wakeup source.
            auto wakeup = wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_USART);
            while (wakeup) {
                auto usartkWakeup = reinterpret_cast<hal_wakeup_source_usart_t*>(wakeup);
                if (usartkWakeup->serial == serial.interface()) {
                    return *this;
                }
                wakeup = wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_USART, wakeup->next);
            }
            // Otherwise, configure USART as wakeup source.
#if HAL_PLATFORM_RTL872X
            auto wakeupSource = (hal_wakeup_source_usart_t*)system_pool_alloc(sizeof(hal_wakeup_source_usart_t), nullptr);
#else
            auto wakeupSource = new(std::nothrow) hal_wakeup_source_usart_t();
#endif
            if (!wakeupSource) {
                valid_ = false;
                return *this;
            }
            wakeupSource->base.size = sizeof(hal_wakeup_source_usart_t);
            wakeupSource->base.version = HAL_SLEEP_VERSION;
            wakeupSource->base.type = HAL_WAKEUP_SOURCE_TYPE_USART;
            wakeupSource->base.next = config_.wakeup_sources;
            wakeupSource->serial = serial.interface();
            config_.wakeup_sources = reinterpret_cast<hal_wakeup_source_base_t*>(wakeupSource);
        }
        return *this;
    }

    SystemSleepConfiguration& network(network_interface_t netif, EnumFlags<SystemSleepNetworkFlag> flags = SystemSleepNetworkFlag::NONE) {
        if (valid_) {
            auto wakeup = wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_NETWORK);
            while (wakeup) {
                auto networkWakeup = reinterpret_cast<hal_wakeup_source_network_t*>(wakeup);
                if (networkWakeup->index == netif) {
                    if (networkWakeup->flags != flags.value()) {
                        networkWakeup->flags |= flags.value();
                    }
                    return *this;
                }
                wakeup = wakeupSourceFeatured(HAL_WAKEUP_SOURCE_TYPE_NETWORK, wakeup->next);
            }
#if HAL_PLATFORM_RTL872X
            auto wakeupSource = (hal_wakeup_source_network_t*)system_pool_alloc(sizeof(hal_wakeup_source_network_t), nullptr);
#else
            auto wakeupSource = new(std::nothrow) hal_wakeup_source_network_t();
#endif
            if (!wakeupSource) {
                valid_ = false;
                return *this;
            }
            wakeupSource->base.size = sizeof(hal_wakeup_source_gpio_t);
            wakeupSource->base.version = HAL_SLEEP_VERSION;
            wakeupSource->base.type = HAL_WAKEUP_SOURCE_TYPE_NETWORK;
            wakeupSource->base.next = config_.wakeup_sources;
            wakeupSource->index = static_cast<network_interface_index>(netif);
            wakeupSource->flags = flags.value();
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
#if HAL_PLATFORM_RTL872X
            auto wakeupSource = (hal_wakeup_source_base_t*)system_pool_alloc(sizeof(hal_wakeup_source_base_t), nullptr);
#else
            auto wakeupSource = new(std::nothrow) hal_wakeup_source_base_t();
#endif
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

} /* namespace particle */
