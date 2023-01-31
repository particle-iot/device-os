/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#include "static_recursive_mutex.h"

#ifdef __cplusplus

namespace particle {

namespace net {

struct __attribute__((packed)) WizNetifConfigData {
    uint16_t size;
    uint16_t cs_pin;
    uint16_t reset_pin;
    uint16_t int_pin;
};

class WizNetifConfig {
public:
    /**
     * Get the singleton instance of this class.
     */
    static WizNetifConfig* instance();

    void init();
    int setConfigData(const WizNetifConfigData* userConfigData = nullptr);
    int getConfigData(WizNetifConfigData* configData);
    int lock();
    int unlock();

protected:
    WizNetifConfig();
    ~WizNetifConfig() = default;

    bool initialized_;
    WizNetifConfigData wizNetifConfigData_ = {};

    StaticRecursiveMutex mutex_;

    int validateWizNetifConfigData();
    int saveWizNetifConfigData();
    int recallWizNetifConfigData();
    // int deleteWizNetifConfigData();
    // void logWizNetifConfigData(WizNetifConfigData& data);
};

class WizNetifConfigLock {
public:
    WizNetifConfigLock()
            : locked_(false) {
        lock();
    }
    ~WizNetifConfigLock() {
        if (locked_) {
            unlock();
        }
    }
    WizNetifConfigLock(WizNetifConfigLock&& lock)
            : locked_(lock.locked_) {
        lock.locked_ = false;
    }
    void lock() {
        WizNetifConfig::instance()->lock();
        locked_ = true;
    }
    void unlock() {
        WizNetifConfig::instance()->unlock();
        locked_ = false;
    }
    WizNetifConfigLock(const WizNetifConfigLock&) = delete;
    WizNetifConfigLock& operator=(const WizNetifConfigLock&) = delete;

private:
    bool locked_;
};

} // namespace net

} // namespace particle

#endif /* __cplusplus */
