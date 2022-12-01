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

#ifndef WATCHDOG_BASE_H
#define	WATCHDOG_BASE_H

#if HAL_PLATFORM_HW_WATCHDOG

#include <algorithm>
#include "check.h"

class Watchdog {
public:
    Watchdog(uint32_t mandatoryCaps, uint32_t optionalCaps, uint32_t minTimeout, uint32_t maxTimeout)
            : info_{},
              callback_(nullptr),
              context_(nullptr) {
        info_.config.size = sizeof(hal_watchdog_config_t);
        info_.config.version = HAL_WATCHDOG_VERSION;

        info_.size = sizeof(hal_watchdog_info_t);
        info_.version = HAL_WATCHDOG_VERSION;
        info_.mandatory_caps = mandatoryCaps;
        info_.optional_caps = optionalCaps;
        info_.min_timeout_ms = minTimeout;
        info_.max_timeout_ms = maxTimeout;
        info_.state = HAL_WATCHDOG_STATE_DISABLED;
    }

    Watchdog() = default;

    void notify() {
        if (callback_) {
            callback_(context_);
        }
    }

    virtual bool started() { 
        return info_.state == HAL_WATCHDOG_STATE_STARTED;
    }

    virtual int stop() {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    
    virtual int getInfo(hal_watchdog_info_t* info) {
        CHECK_TRUE(info && (info->size > 0), SYSTEM_ERROR_INVALID_ARGUMENT);
        memcpy(info, &info_, std::min(info->size, info_.size));
        return SYSTEM_ERROR_NONE;
    }

    virtual int setOnExpiredCallback(hal_watchdog_on_expired_callback_t callback, void* context) {
        callback_ = callback;
        context_ = context;
        return SYSTEM_ERROR_NONE;
    }

    virtual int init(const hal_watchdog_config_t* config) = 0;
    virtual int start() = 0;
    virtual int refresh() = 0;

    hal_watchdog_info_t info_;
    hal_watchdog_on_expired_callback_t callback_;
    void* context_;
};

#endif // HAL_PLATFORM_HW_WATCHDOG

#endif	/* WATCHDOG_BASE_H */

