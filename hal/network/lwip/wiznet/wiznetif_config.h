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

// STATIC_ASSERT already defined from static_assert.h as PARTICLE_STATIC_ASSERT
#ifdef STATIC_ASSERT
#undef STATIC_ASSERT
#endif
// app_util.h unconditionally defines STATIC_ASSERT
#include "static_recursive_mutex.h"
#include "platforms.h"

#ifdef __cplusplus

namespace particle {

namespace net {

#if PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_BORON || PLATFORM_ID == PLATFORM_P2 || PLATFORM_ID == PLATFORM_TRACKERM
#define WIZNETIF_CS_PIN_DEFAULT    (D5)
#define WIZNETIF_RESET_PIN_DEFAULT (D3)
#define WIZNETIF_INT_PIN_DEFAULT   (D4)
#elif PLATFORM_ID == PLATFORM_ESOMX
#define WIZNETIF_CS_PIN_DEFAULT    (B0)
#define WIZNETIF_RESET_PIN_DEFAULT (B1)
#define WIZNETIF_INT_PIN_DEFAULT   (D2)
#elif PLATFORM_ID == PLATFORM_ASOM || PLATFORM_ID == PLATFORM_BSOM || PLATFORM_ID == PLATFORM_B5SOM
#define WIZNETIF_CS_PIN_DEFAULT    (D8)
#define WIZNETIF_RESET_PIN_DEFAULT (A7)
#define WIZNETIF_INT_PIN_DEFAULT   (D22)
#elif PLATFORM_ID == PLATFORM_TRACKER
#define WIZNETIF_CS_PIN_DEFAULT    (D2)
#define WIZNETIF_RESET_PIN_DEFAULT (D6)
#define WIZNETIF_INT_PIN_DEFAULT   (D7)
#endif

#define WIZNETIF_CONFIG_DATA_VERSION_V1 (1)
const uint16_t WIZNETIF_CONFIG_DATA_VERSION = WIZNETIF_CONFIG_DATA_VERSION_V1;

struct __attribute__((packed)) WizNetifConfigData {
    uint16_t size;
    uint16_t version;
    uint16_t cs_pin;
    uint16_t reset_pin;
    uint16_t int_pin;
};

// Change to 1 for debugging
#define WIZNETIF_CONFIG_ENABLE_DEBUG_LOGGING (0)

class WizNetifConfig {
public:
    /**
     * Get the singleton instance of this class.
     */
    static WizNetifConfig* instance();

    void init(bool forced = false);
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

    bool isPinValid(uint16_t pin);
    int initializeWizNetifConfigData(WizNetifConfigData& data);
    int validateWizNetifConfigData(const WizNetifConfigData* data);
    int saveWizNetifConfigData();
    int recallWizNetifConfigData();
    // int deleteWizNetifConfigData();
    void logWizNetifConfigData(WizNetifConfigData& data);
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
