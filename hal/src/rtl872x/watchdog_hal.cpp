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
#include "logging.h"
#include "static_recursive_mutex.h"

extern "C" {

#include "rtl8721d.h"

void WDG_IrqClear(void);
}

class WatchdogLock {
public:
    WatchdogLock() {
        mutex_.lock();
    }

    ~WatchdogLock() {
        mutex_.unlock();
    }

private:
    StaticRecursiveMutex mutex_;
};

/**
 * @note The watchdog will be paused during sleep modes, but not the debug mode.
 * 
 */
class RtlWatchdog : public Watchdog {
public:
    int init(const hal_watchdog_config_t* config) {
        if (started()) {
            stop();
        }

        CHECK_TRUE(config && (config->size > 0), SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(config->timeout_ms >= WATCHDOG_MIN_TIMEOUT, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(config->timeout_ms <= WATCHDOG_MAX_TIMEOUT, SYSTEM_ERROR_INVALID_ARGUMENT);

        WDG_InitTypeDef WDG_InitStruct = {};
        uint32_t CountProcess = 0;
        uint32_t DivFacProcess = 1; // Minimum requirement
        calculateFactors(config->timeout_ms, &CountProcess, &DivFacProcess);
        WDG_InitStruct.CountProcess = CountProcess;
        WDG_InitStruct.DivFacProcess = DivFacProcess;
        WDG_InitStruct.RstAllPERI = 1;
        WDG_Init(&WDG_InitStruct);
        if (config->enable_caps & HAL_WATCHDOG_CAPS_RESET) {
            BKUP_Set(BKUP_REG0, BIT_KM4SYS_RESET_HAPPEN);
        } else {
            // It will change the watchdog to INT mode.
            WDG_IrqInit((void*)rtlWatchdogEventHandler, (uint32_t)this);
        }
        
        memcpy(&info_.config, config, std::min(info_.config.size, config->size));
        info_.state = HAL_WATCHDOG_STATE_CONFIGURED;
        initialized_ = true;
        return SYSTEM_ERROR_NONE;
    }

    int start() {
        CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
        CHECK_FALSE(started(), SYSTEM_ERROR_NONE);
        WDG_Cmd(ENABLE);
        CHECK_TRUE(started(), SYSTEM_ERROR_INTERNAL);
        return SYSTEM_ERROR_NONE;
    }

    bool started() override {
        WDG_TypeDef* WDG = ((WDG_TypeDef *) WDG_REG_BASE);
        uint32_t temp = WDG->VENDOR;
        if (temp & WDG_BIT_ENABLE) {
            info_.state = HAL_WATCHDOG_STATE_STARTED;
        }
        return info_.state == HAL_WATCHDOG_STATE_STARTED;
    }

    int stop() override {
        CHECK_TRUE(started(), SYSTEM_ERROR_NONE);
        WDG_Cmd(DISABLE);
        info_.state = HAL_WATCHDOG_STATE_STOPPED;
        CHECK_FALSE(started(), SYSTEM_ERROR_INTERNAL);
        return SYSTEM_ERROR_NONE;
    }

    int refresh() {
        CHECK_TRUE(initialized_, SYSTEM_ERROR_INVALID_STATE);
        CHECK_TRUE(started(), SYSTEM_ERROR_INVALID_STATE);
        WDG_Refresh();
        return SYSTEM_ERROR_NONE;
    }

    static RtlWatchdog* instance() {
        static RtlWatchdog watchdog(HAL_WATCHDOG_CAPS_DEBUG_RUNNING,
                                    HAL_WATCHDOG_CAPS_RESET | HAL_WATCHDOG_CAPS_NOTIFY_ONLY |
                                    HAL_WATCHDOG_CAPS_RECONFIGURABLE | HAL_WATCHDOG_CAPS_STOPPABLE,
                                    WATCHDOG_MIN_TIMEOUT, WATCHDOG_MAX_TIMEOUT);
        return &watchdog;
    }

private:
    RtlWatchdog(uint32_t mandatoryCaps, uint32_t optionalCaps, uint32_t minTimeout, uint32_t maxTimeout)
            : Watchdog(mandatoryCaps, optionalCaps, minTimeout, maxTimeout),
              initialized_(false) {
    }

    ~RtlWatchdog() = default;

    void calculateFactors(system_tick_t timeout, uint32_t* count, uint32_t* div) {
        if (timeout == 0) {
            *count = 0;
            *div = 1;
            return;
        }
        uint32_t tempDiv;
        uint16_t tempCount;
        bool candidate = false;
        for (int8_t countId = 11; countId >= 0; countId--) {
            tempCount = (0x00000001 << (countId + 1)) - 1;
            tempDiv = ((timeout * 32768ULL) / tempCount / 1000);
            if (tempDiv <= 1) { // minimum *div is of 1
                continue;
            }
            if (candidate && tempDiv > 65536) {
                break;
            }
            tempDiv = std::min(tempDiv, (uint32_t)65536);
            *div = tempDiv - 1;
            *count = countId;
            candidate = true;
            // Continue to seek smaller countId
        }
    }

    static void rtlWatchdogEventHandler(void* context) {
        WDG_IrqClear();
        auto pInstance = (RtlWatchdog*)context;
        if (!pInstance) {
            return;
        }
        pInstance->notify();
    }

    volatile bool initialized_;

    // timeout = (1 / ((float)32.768 / (div + 1))) * count;
    // Minimum allowed div is 1.
    static constexpr uint32_t WATCHDOG_MIN_TIMEOUT = 0;
    static constexpr uint32_t WATCHDOG_MAX_TIMEOUT = 8190000;
};

static Watchdog* getWatchdogInstance(hal_watchdog_instance_t instance) {
    static Watchdog* watchdog[HAL_PLATFORM_HW_WATCHDOG_COUNT] = {
        RtlWatchdog::instance(),
        // Add pointer to new watchdog here.
    };
    CHECK_TRUE(instance < sizeof(watchdog) / sizeof(watchdog[0]), nullptr);
    return watchdog[instance];
}


/**** Watchdog HAL APIs ****/

int hal_watchdog_set_config(hal_watchdog_instance_t instance, const hal_watchdog_config_t* config, void* reserved) {
    WatchdogLock lk();
    auto pInstance = getWatchdogInstance(instance);
    CHECK_TRUE(pInstance, SYSTEM_ERROR_NOT_FOUND);
    return pInstance->init(config);
}

int hal_watchdog_on_expired_callback(hal_watchdog_instance_t instance, hal_watchdog_on_expired_callback_t callback, void* context, void* reserved) {
    WatchdogLock lk();
    auto pInstance = getWatchdogInstance(instance);
    CHECK_TRUE(pInstance, SYSTEM_ERROR_NOT_FOUND);
    return pInstance->setOnExpiredCallback(callback, context);
}

int hal_watchdog_start(hal_watchdog_instance_t instance, void* reserved) {
    WatchdogLock lk();
    auto pInstance = getWatchdogInstance(instance);
    CHECK_TRUE(pInstance, SYSTEM_ERROR_NOT_FOUND);
    return pInstance->start();
}

int hal_watchdog_stop(hal_watchdog_instance_t instance, void* reserved) {
    WatchdogLock lk();
    auto pInstance = getWatchdogInstance(instance);
    CHECK_TRUE(pInstance, SYSTEM_ERROR_NOT_FOUND);
    return pInstance->stop();
}

int hal_watchdog_refresh(hal_watchdog_instance_t instance, void* reserved) {
    WatchdogLock lk();
    auto pInstance = getWatchdogInstance(instance);
    CHECK_TRUE(pInstance, SYSTEM_ERROR_NOT_FOUND);
    return pInstance->refresh();
}

int hal_watchdog_get_info(hal_watchdog_instance_t instance, hal_watchdog_info_t* info, void* reserved) {
    WatchdogLock lk();
    auto pInstance = getWatchdogInstance(instance);
    CHECK_TRUE(pInstance, SYSTEM_ERROR_NOT_FOUND);
    // Update info.state according to the status register.
    pInstance->started();
    return pInstance->getInfo(info);
}

// backward compatibility for nRF52
bool hal_watchdog_reset_flagged_deprecated(void) {
    return false;
}

void hal_watchdog_refresh_deprecated() {
    hal_watchdog_refresh(HAL_WATCHDOG_INSTANCE1, nullptr);
}

#endif // HAL_PLATFORM_HW_WATCHDOG
