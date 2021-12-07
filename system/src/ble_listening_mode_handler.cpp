/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "logging.h"
LOG_SOURCE_CATEGORY("system.listen.ble")

#include "ble_listening_mode_handler.h"

#if HAL_PLATFORM_BLE

#include "system_error.h"
#include "check.h"
#include "scope_guard.h"
#include "device_code.h"
#include "service_debug.h"
#include "core_hal.h"

namespace {

using namespace particle::system;
using namespace particle::ble;

} // unnamed

BleListeningModeHandler::BleListeningModeHandler()
        : preAdvParams_(),
          preTxPower_(0),
          prePpcp_(),
          preAdvertising_(false),
          preConnected_(false),
          preAutoAdv_(BLE_AUTO_ADV_ALWAYS),
          userAdv_(false),
          restoreUserConfig_(false) {
}

BleListeningModeHandler::~BleListeningModeHandler() {
}

BleListeningModeHandler* BleListeningModeHandler::instance() {
  static BleListeningModeHandler blelstmodehndlr;
  return &blelstmodehndlr;
}

bool BleListeningModeHandler::getProvModeStatus() {
    return provMode_;
}

void BleListeningModeHandler::setProvModeStatus(bool enabled) {
    provMode_ = enabled;
}

int BleListeningModeHandler::constructControlRequestAdvData() {
    CHECK_FALSE(exited_, SYSTEM_ERROR_INVALID_STATE);

    Vector<uint8_t> tempAdvData;
    Vector<uint8_t> tempSrData;

    // AD Flag
    CHECK_TRUE(tempAdvData.append(0x02), SYSTEM_ERROR_NO_MEMORY);
    CHECK_TRUE(tempAdvData.append(BLE_SIG_AD_TYPE_FLAGS), SYSTEM_ERROR_NO_MEMORY);
    CHECK_TRUE(tempAdvData.append(BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE), SYSTEM_ERROR_NO_MEMORY);

    char devName[32] = {};
    CHECK(get_device_name(devName, sizeof(devName)));

    // Complete local name
    CHECK_TRUE(tempAdvData.append(strlen(devName) + 1), SYSTEM_ERROR_NO_MEMORY);
    CHECK_TRUE(tempAdvData.append(BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME), SYSTEM_ERROR_NO_MEMORY);
    CHECK_TRUE(tempAdvData.append((uint8_t*)devName, strlen(devName)), SYSTEM_ERROR_NO_MEMORY);

    uint16_t platformID = PLATFORM_ID;
    uint16_t companyID = PARTICLE_COMPANY_ID;

    // Manufacturing specific data
    CHECK_TRUE(tempAdvData.append(sizeof(platformID) + sizeof(companyID) + 1), SYSTEM_ERROR_NO_MEMORY);
    CHECK_TRUE(tempAdvData.append(BLE_SIG_AD_TYPE_MANUFACTURER_SPECIFIC_DATA), SYSTEM_ERROR_NO_MEMORY);
    CHECK_TRUE(tempAdvData.append((uint8_t*)&companyID, 2), SYSTEM_ERROR_NO_MEMORY);
    CHECK_TRUE(tempAdvData.append((uint8_t*)&platformID, sizeof(platformID)), SYSTEM_ERROR_NO_MEMORY);

    // Particle Control Request Service 128-bits UUID
    CHECK_TRUE(tempSrData.append(sizeof(BLE_CTRL_REQ_SVC_UUID) + 1), SYSTEM_ERROR_NO_MEMORY);
    CHECK_TRUE(tempSrData.append(BLE_SIG_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE), SYSTEM_ERROR_NO_MEMORY);
    CHECK_TRUE(tempSrData.append(BLE_CTRL_REQ_SVC_UUID, sizeof(BLE_CTRL_REQ_SVC_UUID)), SYSTEM_ERROR_NO_MEMORY);

    ctrlReqAdvData_ = std::move(tempAdvData);
    ctrlReqSrData_ = std::move(tempSrData);
    return SYSTEM_ERROR_NONE;
}

int BleListeningModeHandler::cacheUserConfigurations() {
    LOG_DEBUG(TRACE, "Cache user's BLE configurations.");
    CHECK_FALSE(exited_, SYSTEM_ERROR_INVALID_STATE);

    Vector<uint8_t> tempAdvData(BLE_MAX_ADV_DATA_LEN);
    Vector<uint8_t> tempSrData(BLE_MAX_ADV_DATA_LEN);

    // Advertising data set by user application
    size_t len = CHECK(hal_ble_gap_get_advertising_data(tempAdvData.data(), BLE_MAX_ADV_DATA_LEN, nullptr));
    tempAdvData.resize(len);

    // Scan response data set by user application
    len = CHECK(hal_ble_gap_get_scan_response_data(tempSrData.data(), BLE_MAX_ADV_DATA_LEN, nullptr));
    tempSrData.resize(len);

    // Advertising and connection parameters set by user application
    preAdvParams_.size = sizeof(hal_ble_adv_params_t);
    CHECK(hal_ble_gap_get_advertising_parameters(&preAdvParams_, nullptr));
    CHECK(hal_ble_gap_get_tx_power(&preTxPower_, nullptr));
    CHECK(hal_ble_gap_get_ppcp(&prePpcp_, nullptr));
    CHECK(hal_ble_gap_get_auto_advertise(&preAutoAdv_, nullptr));

    // Current BLE status in user application
    preAdvertising_ = hal_ble_gap_is_advertising(nullptr);
    preConnected_ = hal_ble_gap_is_connected(nullptr, nullptr);

    preAdvData_ = std::move(tempAdvData);
    preSrData_ = std::move(tempSrData);
    return SYSTEM_ERROR_NONE;
}

int BleListeningModeHandler::restoreUserConfigurations() {
    LOG_DEBUG(TRACE, "Restore user's BLE configurations.");
    CHECK_FALSE(exited_, SYSTEM_ERROR_INVALID_STATE);

    // Do not allow other thread to modify the BLE configurations.
    BleLock lk;

    // Restore the advertising data, advertising parameters and connection parameters.
    CHECK(hal_ble_gap_set_advertising_data(preAdvData_.data(), preAdvData_.size(), nullptr));
    CHECK(hal_ble_gap_set_scan_response_data(preSrData_.data(), preSrData_.size(), nullptr));
    CHECK(hal_ble_gap_set_ppcp(&prePpcp_, nullptr));
    CHECK(hal_ble_gap_set_advertising_parameters(&preAdvParams_, nullptr));
    CHECK(hal_ble_gap_set_tx_power(preTxPower_, nullptr));
    CHECK(hal_ble_gap_set_auto_advertise(preAutoAdv_, nullptr));

    // It is advertising when entering listening mode.
    if (preAdvertising_) {
        if (!hal_ble_gap_is_connected(nullptr, nullptr)) {
            CHECK(hal_ble_gap_start_advertising(nullptr));
        }
        // Currently it is connected. When disconnected, use previous automatic advertising scheme.
        return SYSTEM_ERROR_NONE;
    }
    // It is connected but not advertising when entering listening mode.
    if (preConnected_) {
        if (!hal_ble_gap_is_connected(nullptr, nullptr)) {
            if (preAutoAdv_ == BLE_AUTO_ADV_ALWAYS) {
                CHECK(hal_ble_gap_start_advertising(nullptr));
            } else {
                CHECK(hal_ble_gap_stop_advertising(nullptr));
            }
        }
        return SYSTEM_ERROR_NONE;
    }
    // It is neither connected nor advertising when entering listening mode.
    if (hal_ble_gap_is_connected(nullptr, nullptr)) {
        if (preAutoAdv_ == BLE_AUTO_ADV_ALWAYS) {
            // Do not automatically start advertising when disconnected.
            preAutoAdv_ = BLE_AUTO_ADV_SINCE_NEXT_CONN;
            CHECK(hal_ble_gap_set_auto_advertise(preAutoAdv_, nullptr));
        }
        return SYSTEM_ERROR_NONE;
    }
    // Stop advertising anyway.
    CHECK(hal_ble_gap_stop_advertising(nullptr));

    return SYSTEM_ERROR_NONE;
}

int BleListeningModeHandler::applyControlRequestConfigurations() {
    LOG_DEBUG(TRACE, "Apply BLE control request configurations.");
    CHECK_FALSE(exited_, SYSTEM_ERROR_INVALID_STATE);

    // Do not allow other thread to modify the BLE configurations.
    BleLock lk;

    bool success = false;
    SCOPE_GUARD ({
        if (!success) {
            exit();
        }
    });

    CHECK(hal_ble_exit_locked_mode(nullptr));

    // Set PPCP (Peripheral Preferred Connection Parameters)
    hal_ble_conn_params_t ppcp = {};
    ppcp.size = sizeof(hal_ble_conn_params_t);
    ppcp.min_conn_interval = BLE_CTRL_REQ_MIN_CONN_INTERVAL;
    ppcp.max_conn_interval = BLE_CTRL_REQ_MAX_CONN_INTERVAL;
    ppcp.conn_sup_timeout = BLE_CTRL_REQ_CONN_SUP_TIMEOUT;
    ppcp.slave_latency = BLE_CTRL_REQ_SLAVE_LATENCY;
    CHECK(hal_ble_gap_set_ppcp(&ppcp, nullptr));

    // Advertising parameters
    hal_ble_adv_params_t advParams = {};
    advParams.size = sizeof(hal_ble_adv_params_t);
    advParams.type = BLE_ADV_CONNECTABLE_SCANNABLE_UNDIRECRED_EVT;
    advParams.filter_policy = BLE_ADV_FP_ANY;
    advParams.interval = BLE_CTRL_REQ_ADV_INTERVAL;
    advParams.timeout = provMode_ ? 0 : BLE_CTRL_REQ_ADV_TIMEOUT;
    advParams.inc_tx_power = false;
    CHECK(hal_ble_gap_set_advertising_parameters(&advParams, nullptr));
    CHECK(hal_ble_gap_set_auto_advertise(BLE_AUTO_ADV_ALWAYS, nullptr));

    // TX power
    CHECK(hal_ble_gap_set_tx_power(BLE_CTRL_REQ_TX_POWER, nullptr));

    CHECK(hal_ble_enter_locked_mode(nullptr));

    success = true;
    return SYSTEM_ERROR_NONE;
}

int BleListeningModeHandler::applyUserAdvData() {
    CHECK_FALSE(exited_, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE((preAdvertising_ || preConnected_), SYSTEM_ERROR_INVALID_STATE);
    if (!userAdv_) {
        LOG_DEBUG(TRACE, "Apply user's advertising data set.");

        // Do not allow other thread to modify the BLE configurations.
        BleLock lk;

        bool success = false;
        SCOPE_GUARD ({
            if (!success) {
                exit();
            }
        });

        CHECK(hal_ble_exit_locked_mode(nullptr));
        CHECK(hal_ble_gap_set_advertising_data(preAdvData_.data(), preAdvData_.size(), nullptr));
        CHECK(hal_ble_gap_set_scan_response_data(preSrData_.data(), preSrData_.size(), nullptr));
        CHECK(hal_ble_enter_locked_mode(nullptr));
        success = true;
    }
    userAdv_ = !userAdv_;
    return userAdv_ ? SYSTEM_ERROR_NONE : SYSTEM_ERROR_INVALID_STATE;
}

int BleListeningModeHandler::applyControlRequestAdvData() {
    LOG_DEBUG(TRACE, "Apply control request advertising data set.");
    CHECK_FALSE(exited_, SYSTEM_ERROR_INVALID_STATE);

    // Do not allow other thread to modify the BLE configurations.
    BleLock lk;

    bool success = false;
    SCOPE_GUARD ({
        if (!success) {
            exit();
        }
    });

    CHECK(hal_ble_exit_locked_mode(nullptr));
    CHECK(hal_ble_gap_set_advertising_data(ctrlReqAdvData_.data(), ctrlReqAdvData_.size(), nullptr));
    CHECK(hal_ble_gap_set_scan_response_data(ctrlReqSrData_.data(), ctrlReqSrData_.size(), nullptr));
    CHECK(hal_ble_enter_locked_mode(nullptr));
    success = true;
    return SYSTEM_ERROR_NONE;
}

int BleListeningModeHandler::enter() {
    // Enter listening mode handler only if FEATURE_FLAG_DISABLE_LISTENING_MODE is enabled
    // Not really needed to check here because this check is already done in the previous calls
    //if (HAL_Feature_Get(FEATURE_DISABLE_LISTENING_MODE)) {
    //    LOG(ERROR, "BLE prov/listening mode not allowed");
    //    return SYSTEM_ERROR_NOT_ALLOWED;
    //}

    exited_ = false;

    // Do not allow other thread to modify the BLE configurations.
    BleLock lk;

    bool success = false;
    SCOPE_GUARD ({
        if (!success) {
            exit();
        }
    });

    SPARK_ASSERT(hal_ble_stack_init(nullptr) == SYSTEM_ERROR_NONE);

    // Now the BLE configurations are non-modifiable.
    CHECK(hal_ble_enter_locked_mode(nullptr));
    CHECK(constructControlRequestAdvData());
    CHECK(cacheUserConfigurations());
    // Restore the user's BLE configurations on exited.
    restoreUserConfig_ = true;
    CHECK(applyControlRequestAdvData());
    CHECK(applyControlRequestConfigurations());
    if (!provMode_) {
        CHECK(hal_ble_set_callback_on_adv_events(onBleAdvEvents, this, nullptr));
    }
    if (!preAdvertising_ && !preConnected_) {
        // Start advertising if it is neither connected nor advertising.
        CHECK(hal_ble_gap_start_advertising(nullptr));
    }

    success = true;
    return SYSTEM_ERROR_NONE;
}

int BleListeningModeHandler::exit() {
    if (exited_) {
        return SYSTEM_ERROR_NONE;
    }

    // FIXME: BLE is still connected to finalize the mobile setup when this function is called. In that case,
    // there is a chance that the mobile setup process is terminated by other thread or user application.

    // It should not CHECK() in this function, in case that the BLE HAL status cannot be restored correctly.

    // Do not allow other thread to modify the BLE configurations until this function exits.
    BleLock lk;

    SCOPE_GUARD ({
        preAdvData_.clear();
        preSrData_.clear();
        ctrlReqAdvData_.clear();
        ctrlReqSrData_.clear();
    });

    // Now the BLE configurations are modifiable.
    hal_ble_exit_locked_mode(nullptr);
    if (restoreUserConfig_) {
        restoreUserConfig_ = false;
        if (restoreUserConfigurations() != SYSTEM_ERROR_NONE) {
            LOG(ERROR, "Failed to restore user configuration.");
        }
    }
    if (!provMode_) {
        hal_ble_cancel_callback_on_adv_events(onBleAdvEvents, this, nullptr);
    }

    exited_ = true;
    return SYSTEM_ERROR_NONE;
}

void BleListeningModeHandler::onBleAdvEvents(const hal_ble_adv_evt_t *event, void* context) {
    if (BleListeningModeHandler::exited_) {
        return;
    }
    const auto handler = (BleListeningModeHandler*)context;
    switch (event->type) {
        case BLE_EVT_ADV_STOPPED: {
            LOG_DEBUG(TRACE, "BLE_EVT_ADV_STOPPED");
            if (handler->applyUserAdvData() != SYSTEM_ERROR_NONE) {
                if (handler->applyControlRequestAdvData() != SYSTEM_ERROR_NONE) {
                    LOG(ERROR, "Failed to apply control request advertising data set.");
                    break;
                }
            }
            if (hal_ble_gap_start_advertising(nullptr) != SYSTEM_ERROR_NONE) {
                LOG(ERROR, "Failed to restart BLE advertising.");
            }
            break;
        }
        default: {
            break;
        }
    }
}

bool BleListeningModeHandler::exited_ = true;
bool BleListeningModeHandler::provMode_ = false;

#endif /* HAL_PLATFORM_BLE */
