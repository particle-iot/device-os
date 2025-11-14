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

#include "watchdog_hal_impl.h"
#include "static_recursive_mutex.h"

#if HAL_PLATFORM_HW_WATCHDOG || HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL

#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
#include "am18x5.h"
using namespace particle;
#endif

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

static WatchdogBase* getWatchdogInstance(hal_watchdog_instance_t instance) {
    static WatchdogBase* watchdogs[] = {
#if HAL_PLATFORM_NRF52840
        Nrf52Watchdog::instance(),
#endif
#if HAL_PLATFORM_RTL872X
        RtlWatchdog::instance(),
#endif
#if HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
        Am18x5Watchdog::instance(),
#endif
        // Add pointer to new watchdog here.
    };
    CHECK_TRUE(instance < sizeof(watchdogs) / sizeof(watchdogs[0]), nullptr);
    return watchdogs[instance];
}

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

// Backward compatibility
bool hal_watchdog_reset_flagged_deprecated(void) {
    return false;
}

void hal_watchdog_refresh_deprecated() {
    hal_watchdog_refresh(HAL_WATCHDOG_INSTANCE1, nullptr);
}

#endif // HAL_PLATFORM_HW_WATCHDOG || HAL_PLATFORM_EXTERNAL_RTC || HAL_PLATFORM_EXTERNAL_RTC_OPTIONAL
