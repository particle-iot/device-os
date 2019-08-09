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

namespace {

using namespace particle::system;

} // unnamed

BleListeningModeHandler::BleListeningModeHandler()
        : preAdvData_(nullptr),
          preAdvDataLen_(0),
          preSrData_(nullptr),
          preSrDataLen_(0),
          preAdvParams_(),
          prePpcp_(),
          preAdvertising_(false),
          preConnected_(false),
          preAutoAdv_(BLE_AUTO_ADV_ALWAYS),
          userAdv_(false),
          ctrlReqAdvData_(nullptr),
          ctrlReqAdvDataLen_(0),
          ctrlReqSrData_(nullptr),
          ctrlReqSrDataLen_(0) {
}

BleListeningModeHandler::~BleListeningModeHandler() {
}

int BleListeningModeHandler::constructControlRequestAdvData() {
    ctrlReqAdvData_.reset(new(std::nothrow) uint8_t[BLE_MAX_ADV_DATA_LEN]);
    CHECK_TRUE(ctrlReqAdvData_, SYSTEM_ERROR_NO_MEMORY);
    ctrlReqSrData_.reset(new(std::nothrow) uint8_t[BLE_MAX_ADV_DATA_LEN]);
    CHECK_TRUE(ctrlReqSrData_, SYSTEM_ERROR_NO_MEMORY);
    ctrlReqAdvDataLen_ = 0;
    ctrlReqSrDataLen_ = 0;

    // AD Flag
    ctrlReqAdvData_[ctrlReqAdvDataLen_++] = 0x02;
    ctrlReqAdvData_[ctrlReqAdvDataLen_++] = BLE_SIG_AD_TYPE_FLAGS;
    ctrlReqAdvData_[ctrlReqAdvDataLen_++] = BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

    char devName[32] = {};
    CHECK(get_device_name(devName, sizeof(devName)));

    // Complete local name
    ctrlReqAdvData_[ctrlReqAdvDataLen_++] = strlen(devName) + 1;
    ctrlReqAdvData_[ctrlReqAdvDataLen_++] = BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME;
    memcpy(&ctrlReqAdvData_[ctrlReqAdvDataLen_], devName, strlen(devName));
    ctrlReqAdvDataLen_ += strlen(devName);

    uint16_t platformID = PLATFORM_ID;
    uint16_t companyID = PARTICLE_COMPANY_ID;

    // Manufacturing specific data
    ctrlReqAdvData_[ctrlReqAdvDataLen_++] = sizeof(platformID) + sizeof(companyID) + 1;
    ctrlReqAdvData_[ctrlReqAdvDataLen_++] = BLE_SIG_AD_TYPE_MANUFACTURER_SPECIFIC_DATA;
    memcpy(&ctrlReqAdvData_[ctrlReqAdvDataLen_], (uint8_t*)&companyID, 2);
    ctrlReqAdvDataLen_ += sizeof(companyID);
    memcpy(&ctrlReqAdvData_[ctrlReqAdvDataLen_], (uint8_t*)&platformID, sizeof(platformID));
    ctrlReqAdvDataLen_ += sizeof(platformID);

    // Particle Control Request Service 128-bits UUID
    ctrlReqSrData_[ctrlReqSrDataLen_++] = sizeof(BLE_CTRL_REQ_SVC_UUID) + 1;
    ctrlReqSrData_[ctrlReqSrDataLen_++] = BLE_SIG_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE;
    memcpy(&ctrlReqSrData_[ctrlReqSrDataLen_], BLE_CTRL_REQ_SVC_UUID, sizeof(BLE_CTRL_REQ_SVC_UUID));
    ctrlReqSrDataLen_ += sizeof(BLE_CTRL_REQ_SVC_UUID);
    return SYSTEM_ERROR_NONE;
}

int BleListeningModeHandler::cacheUserConfigurations() {
    LOG_DEBUG(TRACE, "Cache user's BLE configurations.");

    // Advertising data set by user application
    preAdvDataLen_ = hal_ble_gap_get_advertising_data(nullptr, 0, nullptr);
    CHECK(preAdvDataLen_);
    if (preAdvDataLen_ > 0) {
        preAdvData_.reset(new(std::nothrow) uint8_t[preAdvDataLen_]);
        CHECK_TRUE(preAdvData_, SYSTEM_ERROR_NO_MEMORY);
        hal_ble_gap_get_advertising_data(preAdvData_.get(), preAdvDataLen_, nullptr);
    }

    // Scan response data set by user application
    preSrDataLen_ = hal_ble_gap_get_scan_response_data(nullptr, 0, nullptr);
    CHECK(preSrDataLen_);
    if (preSrDataLen_ > 0) {
        preSrData_.reset(new(std::nothrow) uint8_t[preSrDataLen_]);
        CHECK_TRUE(preSrData_, SYSTEM_ERROR_NO_MEMORY);
        hal_ble_gap_get_scan_response_data(preSrData_.get(), preSrDataLen_, nullptr);
    }

    // Advertising and connection parameters set by user application
    preAdvParams_.size = sizeof(hal_ble_adv_params_t);
    CHECK(hal_ble_gap_get_advertising_parameters(&preAdvParams_, nullptr));
    CHECK(hal_ble_gap_get_ppcp(&prePpcp_, nullptr));
    CHECK(hal_ble_gap_get_auto_advertise(&preAutoAdv_, nullptr));

    // Current BLE status in user application
    preAdvertising_ = hal_ble_gap_is_advertising(nullptr);
    preConnected_ = hal_ble_gap_is_connected(nullptr, nullptr);
    return SYSTEM_ERROR_NONE;
}

int BleListeningModeHandler::restoreUserConfigurations() {
    /*
     * FIXME: It's not possible to restore the final state that is changed by user application
     * during in the listening mode when threading is enabled.
     */

    LOG_DEBUG(TRACE, "Restore user's BLE configurations.");

    // Restore the advertising data, advertising parameters and connection parameters.
    CHECK(hal_ble_gap_set_advertising_data(preAdvData_.get(), preAdvDataLen_, nullptr));
    CHECK(hal_ble_gap_set_scan_response_data(preSrData_.get(), preSrDataLen_, nullptr));
    CHECK(hal_ble_gap_set_ppcp(&prePpcp_, nullptr));
    CHECK(hal_ble_gap_set_advertising_parameters(&preAdvParams_, nullptr));
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
    advParams.timeout = BLE_CTRL_REQ_ADV_TIMEOUT;
    advParams.inc_tx_power = false;
    CHECK(hal_ble_gap_set_advertising_parameters(&advParams, nullptr));
    CHECK(hal_ble_gap_set_auto_advertise(BLE_AUTO_ADV_ALWAYS, nullptr));

    return SYSTEM_ERROR_NONE;
}

int BleListeningModeHandler::applyUserAdvData() {
    CHECK_TRUE((preAdvertising_ || preConnected_), SYSTEM_ERROR_INVALID_STATE);
    if (!userAdv_) {
        LOG_DEBUG(TRACE, "Apply user's advertising data set.");
        CHECK(hal_ble_gap_set_advertising_data(preAdvData_.get(), preAdvDataLen_, nullptr));
        CHECK(hal_ble_gap_set_scan_response_data(preSrData_.get(), preSrDataLen_, nullptr));
    }
    userAdv_ = !userAdv_;
    return userAdv_ ? SYSTEM_ERROR_NONE : SYSTEM_ERROR_INVALID_STATE;
}

int BleListeningModeHandler::applyControlRequestAdvData() {
    LOG_DEBUG(TRACE, "Apply control request advertising data set.");
    CHECK(hal_ble_gap_set_advertising_data(ctrlReqAdvData_.get(), ctrlReqAdvDataLen_, nullptr));
    CHECK(hal_ble_gap_set_scan_response_data(ctrlReqSrData_.get(), ctrlReqSrDataLen_, nullptr));
    return SYSTEM_ERROR_NONE;
}

int BleListeningModeHandler::enter() {
    CHECK(constructControlRequestAdvData());
    CHECK(cacheUserConfigurations());
    CHECK(applyControlRequestAdvData());
    CHECK(applyControlRequestConfigurations());

    CHECK(hal_ble_set_callback_on_adv_events(onBleAdvEvents, this, nullptr));

    if (!preAdvertising_ && !preConnected_) {
        // Start advertising if it is neither connected nor advertising.
        CHECK(hal_ble_gap_start_advertising(nullptr));
    }
    return SYSTEM_ERROR_NONE;
}

int BleListeningModeHandler::exit() {
    CHECK(restoreUserConfigurations());
    CHECK(hal_ble_cancel_callback_on_adv_events(onBleAdvEvents, this, nullptr));
    return SYSTEM_ERROR_NONE;
}

void BleListeningModeHandler::onBleAdvEvents(const hal_ble_adv_evt_t *event, void* context) {
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

#endif /* HAL_PLATFORM_BLE */
