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

#include "wiznetif_config.h"

#include "logging.h"
LOG_SOURCE_CATEGORY("net.en.cfg")

#include "system_error.h"

#include "check.h"
#include "delay_hal.h"

#if HAL_PLATFORM_NCP
#include "system_cache.h"
#endif // HAL_PLATFORM_NCP

namespace particle {

namespace net {

using namespace particle::services;

WizNetifConfig::WizNetifConfig() :
        initialized_(false) {
}

void WizNetifConfig::init() {
    memset(&wizNetifConfigData_, 0, sizeof(wizNetifConfigData_));
    recallWizNetifConfigData();
    if (validateWizNetifConfigData() != SYSTEM_ERROR_NONE) {
        recallWizNetifConfigData();
    }
    initialized_ = true;
}

// void WizNetifConfig::logWizNetifConfigData(WizNetifConfigData& configData) {
//     LOG(INFO, "wizNetifConfigData size:%u cs_pin:%u reset_pin:%u int_pin:%u",
//             configData.size,
//             configData.cs_pin,
//             configData.reset_pin,
//             configData.int_pin);
// }

int WizNetifConfig::saveWizNetifConfigData() {
    WizNetifConfigData tempData = {};
    int result = SystemCache::instance().get(SystemCacheKey::WIZNET_CONFIG_DATA, (uint8_t*)&tempData, sizeof(tempData));
    if (result != sizeof(tempData) ||
            /* memcmp(tempData, wizNetifConfigData_, sizeof(tempData) != 0)) { // not reliable */
            tempData.size != wizNetifConfigData_.size ||
            tempData.cs_pin != wizNetifConfigData_.cs_pin ||
            tempData.reset_pin != wizNetifConfigData_.reset_pin ||
            tempData.int_pin != wizNetifConfigData_.int_pin ) {
        // LOG(INFO, "Saving cached wizNetifConfigData, size: %d", wizNetifConfigData_.size);
        // logWizNetifConfigData(wizNetifConfigData_);
        result = SystemCache::instance().set(SystemCacheKey::WIZNET_CONFIG_DATA, (uint8_t*)&wizNetifConfigData_, sizeof(wizNetifConfigData_));
    }
    return (result < 0) ? result : SYSTEM_ERROR_NONE;
}

int WizNetifConfig::recallWizNetifConfigData() {
    WizNetifConfigData tempData;
    int result = SystemCache::instance().get(SystemCacheKey::WIZNET_CONFIG_DATA, (uint8_t*)&tempData, sizeof(tempData));
    if (result != sizeof(tempData)) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    // LOG(INFO, "Recalling cached wizNetifConfigData");
    // logWizNetifConfigData(tempData);
    memcpy(&wizNetifConfigData_, &tempData, sizeof(wizNetifConfigData_));
    return SYSTEM_ERROR_NONE;
}

// int WizNetifConfig::deleteWizNetifConfigData() {
//     LOG(INFO, "Deleting cached wizNetifConfigData");
//     int result = SystemCache::instance().del(SystemCacheKey::WIZNET_CONFIG_DATA);
//     return (result < 0) ? result : SYSTEM_ERROR_NONE;
// }

int WizNetifConfig::setConfigData(const WizNetifConfigData* configData) {
    WizNetifConfigLock lk;

    if (!initialized_) {
        init();
    } else {
        validateWizNetifConfigData();
    }

    if (configData) {
        memcpy(&wizNetifConfigData_, configData, sizeof(WizNetifConfigData));
        // logWizNetifConfigData(wizNetifConfigData_);
        saveWizNetifConfigData();
    } else {
        return SYSTEM_ERROR_BAD_DATA;
    }

    return SYSTEM_ERROR_NONE;
}

int WizNetifConfig::getConfigData(WizNetifConfigData* configData) {
    WizNetifConfigLock lk;

    if (!initialized_) {
        init();
    } else {
        validateWizNetifConfigData();
    }

    if (configData) {
        recallWizNetifConfigData();
        memcpy(configData, &wizNetifConfigData_, sizeof(WizNetifConfigData));
        // logWizNetifConfigData(wizNetifConfigData_);
    }

    return SYSTEM_ERROR_NONE;
}

int WizNetifConfig::validateWizNetifConfigData() {
    if (wizNetifConfigData_.size != sizeof(WizNetifConfigData)) {
        memset(&wizNetifConfigData_, 0, sizeof(wizNetifConfigData_));
        wizNetifConfigData_.size = sizeof(WizNetifConfigData);
        wizNetifConfigData_.cs_pin = PIN_INVALID;
        wizNetifConfigData_.reset_pin = PIN_INVALID;
        wizNetifConfigData_.int_pin = PIN_INVALID;

        saveWizNetifConfigData();

        // LOG(INFO, "wizNetifConfigData_ initialized");
        return SYSTEM_ERROR_BAD_DATA;
    }

    return SYSTEM_ERROR_NONE;
}

WizNetifConfig* WizNetifConfig::instance() {
    static WizNetifConfig instance;
    return &instance;
}

int WizNetifConfig::lock() {
    return mutex_.lock();
}

int WizNetifConfig::unlock() {
    return mutex_.unlock();
}

} // namespace net

} // namespace particle
