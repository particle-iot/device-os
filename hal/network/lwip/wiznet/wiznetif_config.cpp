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
#define WIZNETIF_CONFIG_LOG_CATEGORY "net.en.cfg"

#include "system_error.h"
#include <algorithm>
#include "check.h"
#include "delay_hal.h"

#if HAL_PLATFORM_NCP
#include "system_cache.h"
#endif // HAL_PLATFORM_NCP

#if WIZNETIF_CONFIG_ENABLE_DEBUG_LOGGING
#define WZCFG_LOG_DEBUG(_level, _fmt, ...) LOG_C(_level, WIZNETIF_CONFIG_LOG_CATEGORY, _fmt, ##__VA_ARGS__)
#else
#define WZCFG_LOG_DEBUG(_level, _fmt, ...)
#endif // WIZNETIF_CONFIG_ENABLE_DEBUG_LOGGING

namespace particle {

namespace net {

using namespace particle::services;

WizNetifConfig::WizNetifConfig() :
        initialized_(false) {
}

void WizNetifConfig::init(bool forced) {
    if (initialized_ && !forced) {
        return;
    }

    initializeWizNetifConfigData(wizNetifConfigData_);
    recallWizNetifConfigData();
    if (validateWizNetifConfigData(&wizNetifConfigData_) != SYSTEM_ERROR_NONE) {
        // XXX: In the future, make version 1 data compatible with version 2
        // if (configData->version == WIZNETIF_CONFIG_DATA_VERSION_V1) {
        //     // check size, and initialize / move elements as appropriate
        // }
        initializeWizNetifConfigData(wizNetifConfigData_);
        saveWizNetifConfigData();
    }
    initialized_ = true;
}

int WizNetifConfig::initializeWizNetifConfigData(WizNetifConfigData& configData) {
    memset(&configData, 0, sizeof(wizNetifConfigData_));
    configData.size = sizeof(WizNetifConfigData);
    configData.version = WIZNETIF_CONFIG_DATA_VERSION;
    configData.cs_pin = PIN_INVALID;
    configData.reset_pin = PIN_INVALID;
    configData.int_pin = PIN_INVALID;

    return SYSTEM_ERROR_NONE;
}

void WizNetifConfig::logWizNetifConfigData(WizNetifConfigData& configData) {
    WZCFG_LOG_DEBUG(INFO, "wizNetifConfigData size:%u ver:%u cs_pin:%u reset_pin:%u int_pin:%u",
            configData.size,
            configData.version,
            configData.cs_pin,
            configData.reset_pin,
            configData.int_pin);
}

int WizNetifConfig::saveWizNetifConfigData() {
    WizNetifConfigData tempData = {};
    tempData.size = sizeof(WizNetifConfigData);
    tempData.version = WIZNETIF_CONFIG_DATA_VERSION;
    int result = SystemCache::instance().get(SystemCacheKey::WIZNET_CONFIG_DATA, (uint8_t*)&tempData, sizeof(tempData));
    if (result != std::min(tempData.size, wizNetifConfigData_.size) ||
            memcmp(&tempData, &wizNetifConfigData_, std::min(tempData.size, wizNetifConfigData_.size)) != 0) {
        WZCFG_LOG_DEBUG(INFO, "Saving cached wizNetifConfigData, size: %d", wizNetifConfigData_.size);
        logWizNetifConfigData(wizNetifConfigData_);
        result = SystemCache::instance().set(SystemCacheKey::WIZNET_CONFIG_DATA, (uint8_t*)&wizNetifConfigData_, sizeof(wizNetifConfigData_));
    }
    return (result < 0) ? result : SYSTEM_ERROR_NONE;
}

int WizNetifConfig::recallWizNetifConfigData() {
    WizNetifConfigData tempData = {};
    tempData.size = sizeof(WizNetifConfigData);
    tempData.version = WIZNETIF_CONFIG_DATA_VERSION;
    int result = SystemCache::instance().get(SystemCacheKey::WIZNET_CONFIG_DATA, (uint8_t*)&tempData, sizeof(tempData));
    if (result != std::min(tempData.size, wizNetifConfigData_.size)) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    WZCFG_LOG_DEBUG(INFO, "Recalling cached wizNetifConfigData, size: %d", result);
    logWizNetifConfigData(tempData);
    memcpy(&wizNetifConfigData_, &tempData, sizeof(tempData));
    return SYSTEM_ERROR_NONE;
}

// int WizNetifConfig::deleteWizNetifConfigData() {
//     WZCFG_LOG_DEBUG(INFO, "Deleting cached wizNetifConfigData");
//     int result = SystemCache::instance().del(SystemCacheKey::WIZNET_CONFIG_DATA);
//     return (result < 0) ? result : SYSTEM_ERROR_NONE;
// }

int WizNetifConfig::setConfigData(const WizNetifConfigData* configData) {
    WizNetifConfigLock lk;

    init();
    CHECK_TRUE(configData, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(validateWizNetifConfigData(configData) == SYSTEM_ERROR_NONE, SYSTEM_ERROR_INVALID_ARGUMENT);

    memcpy(&wizNetifConfigData_, configData, std::min(configData->size, wizNetifConfigData_.size));
    logWizNetifConfigData(wizNetifConfigData_);
    saveWizNetifConfigData();

    return SYSTEM_ERROR_NONE;
}

int WizNetifConfig::getConfigData(WizNetifConfigData* configData) {
    WizNetifConfigLock lk;

    init();
    CHECK_TRUE(configData, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(configData->size != 0, SYSTEM_ERROR_INVALID_ARGUMENT);

    recallWizNetifConfigData();

    memcpy(configData, &wizNetifConfigData_, std::min(configData->size, wizNetifConfigData_.size));
    if (configData->cs_pin == PIN_INVALID) {
        configData->cs_pin = HAL_PLATFORM_ETHERNET_WIZNETIF_CS_PIN_DEFAULT;
    }
    if (configData->reset_pin == PIN_INVALID) {
        configData->reset_pin = HAL_PLATFORM_ETHERNET_WIZNETIF_RESET_PIN_DEFAULT;
    }
    if (configData->int_pin == PIN_INVALID) {
        configData->int_pin = HAL_PLATFORM_ETHERNET_WIZNETIF_INT_PIN_DEFAULT;
    }
    logWizNetifConfigData(wizNetifConfigData_);

    return SYSTEM_ERROR_NONE;
}

bool WizNetifConfig::isPinValid(uint16_t pin) {
    if (pin >= TOTAL_PINS && pin != PIN_INVALID) {
        return false;
    }

    return true;
}

int WizNetifConfig::validateWizNetifConfigData(const WizNetifConfigData* configData) {
    // XXX: If we add a new version in the future this will need to handle that
    if (configData->size != sizeof(WizNetifConfigData) ||
            configData->version != WIZNETIF_CONFIG_DATA_VERSION ||
            !isPinValid(configData->cs_pin) ||
            !isPinValid(configData->reset_pin) ||
            !isPinValid(configData->int_pin)) {
        LOG(ERROR, "configData not valid! size:%u ver:%u cs_pin:%u reset_pin:%u int_pin:%u",
                configData->size,
                configData->version,
                configData->cs_pin,
                configData->reset_pin,
                configData->int_pin);

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
