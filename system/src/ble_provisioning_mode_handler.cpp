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

#include "ble_provisioning_mode_handler.h"

#if HAL_PLATFORM_BLE_SETUP

#include "system_error.h"
#include "check.h"
#include "scope_guard.h"
#include "device_code.h"
#include "service_debug.h"
#include "core_hal.h"
#include "system_control_internal.h"

namespace {

using namespace particle::system;
using namespace particle::ble;

} // unnamed

BleProvisioningModeHandler::BleProvisioningModeHandler()
        : preAdvParams_(),
          preTxPower_(0),
          prePpcp_(),
          preAdvertising_(false),
          preConnected_(false),
          preAutoAdv_(BLE_AUTO_ADV_ALWAYS),
          userAdv_(false),
          restoreUserConfig_(false),
          provMode_(false),
          customCompanyId_(PARTICLE_COMPANY_ID),
          btStackInitialized_(false) {
}

BleProvisioningModeHandler::~BleProvisioningModeHandler() {
}

BleProvisioningModeHandler* BleProvisioningModeHandler::instance() {
  static BleProvisioningModeHandler blelstmodehndlr;
  return &blelstmodehndlr;
}

bool BleProvisioningModeHandler::getProvModeStatus() const {
    return provMode_;
}

void BleProvisioningModeHandler::setProvModeStatus(bool enabled) {
    provMode_ = enabled;
}

int BleProvisioningModeHandler::setCompanyId(uint16_t companyId) {
    customCompanyId_ = companyId;
    return SYSTEM_ERROR_NONE;
}

uint16_t BleProvisioningModeHandler::getCompanyId() const {
    return customCompanyId_;
}

int BleProvisioningModeHandler::constructControlRequestAdvData() {
    CHECK_FALSE(exited_, SYSTEM_ERROR_INVALID_STATE);

    Vector<uint8_t> tempAdvData;
    Vector<uint8_t> tempSrData;

    // AD Flag
    CHECK_TRUE(tempAdvData.append(0x02), SYSTEM_ERROR_NO_MEMORY);
    CHECK_TRUE(tempAdvData.append(BLE_SIG_AD_TYPE_FLAGS), SYSTEM_ERROR_NO_MEMORY);
    CHECK_TRUE(tempAdvData.append(BLE_SIG_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE), SYSTEM_ERROR_NO_MEMORY);

    // Advertising data is limited to 31 bytes, which means the true maximum advertising device name length is only 14 bytes (accounting for all of the below data fields)
    char devName[BLE_MAX_DEV_NAME_LEN + 1] = {};
    CHECK(hal_ble_gap_get_device_name(devName, sizeof(devName), nullptr));

    // Complete local name
    CHECK_TRUE(tempAdvData.append(strlen(devName) + 1), SYSTEM_ERROR_NO_MEMORY);
    CHECK_TRUE(tempAdvData.append(BLE_SIG_AD_TYPE_COMPLETE_LOCAL_NAME), SYSTEM_ERROR_NO_MEMORY);
    CHECK_TRUE(tempAdvData.append((uint8_t*)devName, strlen(devName)), SYSTEM_ERROR_NO_MEMORY);

    uint16_t platformID = PLATFORM_ID;
    uint16_t companyID = getCompanyId();

    // Manufacturing specific data
    char code[HAL_SETUP_CODE_SIZE] = {};
    CHECK(get_device_setup_code(code, sizeof(code)));
    CHECK_TRUE(tempAdvData.append(sizeof(platformID) + sizeof(companyID) + sizeof(code) + 1), SYSTEM_ERROR_NO_MEMORY);
    CHECK_TRUE(tempAdvData.append(BLE_SIG_AD_TYPE_MANUFACTURER_SPECIFIC_DATA), SYSTEM_ERROR_NO_MEMORY);
    CHECK_TRUE(tempAdvData.append((uint8_t*)&companyID, 2), SYSTEM_ERROR_NO_MEMORY);
    CHECK_TRUE(tempAdvData.append((uint8_t*)&platformID, sizeof(platformID)), SYSTEM_ERROR_NO_MEMORY);
    CHECK_TRUE(tempAdvData.append((uint8_t*)&code, sizeof(code)), SYSTEM_ERROR_NO_MEMORY);

    // Particle Control Request Service UUID. This will be overwritten by user's provisioning service ID if available
    // FIXME: Addressing only 128-bit complete and 16-bit complete UUIDs
    hal_ble_uuid_t bleCrtlReqSvcUuid = SystemControl::instance()->getBleCtrlRequestChannel()->getBleCtrlSvcUuid();
    if (bleCrtlReqSvcUuid.type == BLE_UUID_TYPE_128BIT) {
        CHECK_TRUE(tempSrData.append(sizeof(bleCrtlReqSvcUuid.uuid128) + 1), SYSTEM_ERROR_NO_MEMORY);
        CHECK_TRUE(tempSrData.append(BLE_SIG_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE), SYSTEM_ERROR_NO_MEMORY);
        CHECK_TRUE(tempSrData.append(bleCrtlReqSvcUuid.uuid128, sizeof(bleCrtlReqSvcUuid.uuid128)), SYSTEM_ERROR_NO_MEMORY);
    } else {
        CHECK_TRUE(tempSrData.append(sizeof(bleCrtlReqSvcUuid.uuid16) + 1), SYSTEM_ERROR_NO_MEMORY);
        CHECK_TRUE(tempSrData.append(BLE_SIG_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE), SYSTEM_ERROR_NO_MEMORY);
        CHECK_TRUE(tempSrData.append((uint8_t*)&bleCrtlReqSvcUuid.uuid16, BLE_SIG_UUID_16BIT_LEN), SYSTEM_ERROR_NO_MEMORY);
    }
    ctrlReqAdvData_ = std::move(tempAdvData);
    ctrlReqSrData_ = std::move(tempSrData);
    return SYSTEM_ERROR_NONE;
}

int BleProvisioningModeHandler::cacheUserConfigurations() {
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
    prePpcp_.size = sizeof(hal_ble_conn_params_t);
    CHECK(hal_ble_gap_get_ppcp(&prePpcp_, nullptr));
    CHECK(hal_ble_gap_get_auto_advertise(&preAutoAdv_, nullptr));

    // Current BLE status in user application
    preAdvertising_ = hal_ble_gap_is_advertising(nullptr);
    preConnected_ = hal_ble_gap_is_connected(nullptr, nullptr);

    preAdvData_ = std::move(tempAdvData);
    preSrData_ = std::move(tempSrData);
    return SYSTEM_ERROR_NONE;
}

int BleProvisioningModeHandler::restoreUserConfigurations() {
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

    // It is advertising when entering listening/provisioning mode.
    if (preAdvertising_) {
        if (!hal_ble_gap_is_connected(nullptr, nullptr)) {
            CHECK(hal_ble_gap_start_advertising(nullptr));
        }
        // Currently it is connected. When disconnected, use previous automatic advertising scheme.
        return SYSTEM_ERROR_NONE;
    }
    // It is connected but not advertising when entering listening/provisioning mode.
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
    // It is neither connected nor advertising when entering listening/provisioning mode.
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

int BleProvisioningModeHandler::applyControlRequestConfigurations() {
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
    advParams.timeout = BLE_CTRL_REQ_ADV_TIMEOUT;
    advParams.inc_tx_power = false;
    CHECK(hal_ble_gap_set_advertising_parameters(&advParams, nullptr));
    CHECK(hal_ble_gap_set_auto_advertise(BLE_AUTO_ADV_ALWAYS, nullptr));

    // TX power
    CHECK(hal_ble_gap_set_tx_power(BLE_CTRL_REQ_TX_POWER, nullptr));

    CHECK(hal_ble_enter_locked_mode(nullptr));

    success = true;
    return SYSTEM_ERROR_NONE;
}

int BleProvisioningModeHandler::applyUserAdvData() {
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

int BleProvisioningModeHandler::applyControlRequestAdvData() {
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

int BleProvisioningModeHandler::enter() {
    exited_ = false;

    // Do not allow other thread to modify the BLE configurations.
    BleLock lk;

    bool success = false;
    SCOPE_GUARD ({
        if (!success) {
            exit();
        }
    });

    btStackInitialized_ = hal_ble_is_initialized(nullptr);
    if (!btStackInitialized_) {
        SPARK_ASSERT(hal_ble_stack_init(nullptr) == SYSTEM_ERROR_NONE);
    }

    // Now the BLE configurations are non-modifiable.
    CHECK(hal_ble_enter_locked_mode(nullptr));
    CHECK(constructControlRequestAdvData());
    CHECK(cacheUserConfigurations());
    // Restore the user's BLE configurations on exited.
    restoreUserConfig_ = true;
    CHECK(applyControlRequestAdvData());
    CHECK(applyControlRequestConfigurations());
    CHECK(hal_ble_set_callback_on_adv_events(onBleAdvEvents, this, nullptr));
    if (!preAdvertising_ && !preConnected_) {
        // Start advertising if it is neither connected nor advertising.
        CHECK(hal_ble_gap_start_advertising(nullptr));
    }

    success = true;
    return SYSTEM_ERROR_NONE;
}

int BleProvisioningModeHandler::exit() {
    if (exited_) {
        return SYSTEM_ERROR_NONE;
    }

    // FIXME: BLE is still connected to finalize the mobile setup when this function is called. In that case,
    // there is a chance that the mobile setup process is terminated by other thread or user application.

    // It should not CHECK() in this function, in case that the BLE HAL status cannot be restored correctly.

    // Do not allow other thread to modify the BLE configurations until this function exits.
    BleLock lk;

    hal_ble_cancel_callback_on_adv_events(onBleAdvEvents, this, nullptr);

    SCOPE_GUARD ({
        preAdvData_.clear();
        preSrData_.clear();
        ctrlReqAdvData_.clear();
        ctrlReqSrData_.clear();
#if HAL_PLATFORM_RTL872X
        if (!btStackInitialized_) {
            hal_ble_stack_deinit(nullptr);
        }
#endif
    });

    // Now the BLE configurations are modifiable.
    hal_ble_exit_locked_mode(nullptr);
    if (restoreUserConfig_) {
        restoreUserConfig_ = false;
        if (restoreUserConfigurations() != SYSTEM_ERROR_NONE) {
            LOG(ERROR, "Failed to restore user configuration.");
        }
    }

    exited_ = true;
    return SYSTEM_ERROR_NONE;
}

void BleProvisioningModeHandler::onBleAdvEvents(const hal_ble_adv_evt_t *event, void* context) {
    if (BleProvisioningModeHandler::exited_) {
        return;
    }
    const auto handler = (BleProvisioningModeHandler*)context;
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

bool BleProvisioningModeHandler::exited_ = true;

#endif /* HAL_PLATFORM_BLE_SETUP */
