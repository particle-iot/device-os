/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
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

#include "watchdog_hal.h"
#include "watchdog_base.h"

#if HAL_PLATFORM_HW_WATCHDOG
#include "check.h"
#include "nrfx_wdt.h"
#include "logging.h"

/**
 * @brief The watchdog reset behavior is the same as the pin reset behavior.
 * The following resources will be reset due to watchdog reset:
 *  - CPU
 *  - Peripherals
 *  - GPIO
 *  - Debug
 *  - RAM
 *  - WDT
 *  - Retained registers
 * 
 * The watchdog will be reset by the following reset sources:
 *  - Watchdog reset
 *  - Pin reset
 *  - Brownout reset
 *  - Power on reset
 * 
 * @note When the device starts running again, after a reset, or waking up from OFF mode,
 * the watchdog configuration registers will be available for configuration again.
 */
class Nrf52Watchdog : public WatchdogBase {
public:
    int init(const hal_watchdog_config_t* config) {
        CHECK_FALSE(started(), SYSTEM_ERROR_INVALID_STATE);
        CHECK_TRUE(config && (config->size > 0), SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(config->timeout_ms >= WATCHDOG_MIN_TIMEOUT, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(config->timeout_ms <= WATCHDOG_MAX_TIMEOUT, SYSTEM_ERROR_INVALID_ARGUMENT);

        nrfx_wdt_config_t nrfConfig = {};
        nrfConfig.reload_value = config->timeout_ms;
        nrfConfig.interrupt_priority = NRF52_WATCHDOG_PRIORITY;
        if ((config->enable_caps & HAL_WATCHDOG_CAPS_SLEEP_RUNNING) && (config->enable_caps & HAL_WATCHDOG_CAPS_DEBUG_RUNNING)) {
            nrfConfig.behaviour = NRF_WDT_BEHAVIOUR_RUN_SLEEP_HALT;
        } else if (config->enable_caps & HAL_WATCHDOG_CAPS_SLEEP_RUNNING) {
            nrfConfig.behaviour = NRF_WDT_BEHAVIOUR_RUN_SLEEP;
        } else if (config->enable_caps & HAL_WATCHDOG_CAPS_DEBUG_RUNNING) {
            nrfConfig.behaviour = NRF_WDT_BEHAVIOUR_RUN_HALT; // WDT will be paused when CPU is in SLEEP or HALT mode.
        } else {
            nrfConfig.behaviour = NRF_WDT_BEHAVIOUR_PAUSE_SLEEP_HALT;
        }
        if (!initialized_) {
            nrfx_err_t ret = nrfx_wdt_init(&nrfConfig, nrf52WatchdogEventHandler);
            SPARK_ASSERT(ret == NRF_SUCCESS);
            ret = nrfx_wdt_channel_alloc(&channelId_);
            SPARK_ASSERT(ret == NRF_SUCCESS);
        } else {
            nrf_wdt_behaviour_set(nrfConfig.behaviour);
            uint64_t ticks = (nrfConfig.reload_value * 32768ULL) / 1000;
            SPARK_ASSERT(ticks <= UINT32_MAX);
            nrf_wdt_reload_value_set((uint32_t)ticks);
        }

        memcpy(&info_.config, config, std::min(info_.config.size, config->size));
        info_.state = HAL_WATCHDOG_STATE_CONFIGURED;
        initialized_ = true;
        return SYSTEM_ERROR_NONE;
    }

    int start() {
        CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
        CHECK_FALSE(started(), SYSTEM_ERROR_NONE);
        nrfx_wdt_enable();
        CHECK_TRUE(started(), SYSTEM_ERROR_INTERNAL);
        return SYSTEM_ERROR_NONE;
    }

    bool started() override {
        if (nrf_wdt_started()) {
            info_.state = HAL_WATCHDOG_STATE_STARTED;
        }
        return info_.state == HAL_WATCHDOG_STATE_STARTED;
    }

    int refresh() {
        CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
        CHECK_TRUE(started(), SYSTEM_ERROR_INVALID_STATE);
        nrfx_wdt_channel_feed(channelId_);
        return SYSTEM_ERROR_NONE;
    }

    static Nrf52Watchdog* instance() {
        static Nrf52Watchdog watchdog(HAL_WATCHDOG_CAPS_RESET,
                                      HAL_WATCHDOG_CAPS_NOTIFY | HAL_WATCHDOG_CAPS_SLEEP_RUNNING | HAL_WATCHDOG_CAPS_DEBUG_RUNNING,
                                      WATCHDOG_MIN_TIMEOUT, WATCHDOG_MAX_TIMEOUT);
        return &watchdog;
    }

private:
    Nrf52Watchdog(uint32_t mandatoryCaps, uint32_t optionalCaps, uint32_t minTimeout, uint32_t maxTimeout)
            : WatchdogBase(mandatoryCaps, optionalCaps, minTimeout, maxTimeout),
              initialized_(false) {
    }

    ~Nrf52Watchdog() = default;

    static void nrf52WatchdogEventHandler() {
        // NOTE: The max amount of time we can spend in WDT interrupt is two cycles of 32768[Hz] clock - after that, reset occurs
        if (Nrf52Watchdog::instance()->info_.config.enable_caps & HAL_WATCHDOG_CAPS_NOTIFY) {
            Nrf52Watchdog::instance()->notify();
        }
    }

    volatile bool initialized_;
    nrfx_wdt_channel_id channelId_;
    static constexpr uint8_t NRF52_WATCHDOG_PRIORITY = 7;
    static constexpr uint32_t WATCHDOG_MIN_TIMEOUT = 0;
    static constexpr uint32_t WATCHDOG_MAX_TIMEOUT = 131071999; // 0xFFFFFFFF / 32.768
};

#endif // HAL_PLATFORM_HW_WATCHDOG
