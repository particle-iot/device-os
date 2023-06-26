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

#ifndef BLE_PROVISIONING_MODE_HANDLER_H
#define BLE_PROVISIONING_MODE_HANDLER_H

#include "hal_platform.h"

#if HAL_PLATFORM_BLE_SETUP

#include <memory>
#include "ble_hal.h"
#include "spark_wiring_vector.h"

namespace particle { namespace system {

class BleProvisioningModeHandler {
public:
    static BleProvisioningModeHandler* instance();

    int enter();
    int exit();

    bool getProvModeStatus() const;
    void setProvModeStatus(bool enabled);

    int setCompanyId(uint16_t companyId);
    uint16_t getCompanyId() const;

protected:
    BleProvisioningModeHandler();
    ~BleProvisioningModeHandler();

private:
    int constructControlRequestAdvData();
    int cacheUserConfigurations();
    int restoreUserConfigurations();
    int applyControlRequestConfigurations();
    int applyUserAdvData();
    int applyControlRequestAdvData();
    static void onBleAdvEvents(const hal_ble_adv_evt_t *event, void* context);

    const uint16_t BLE_CTRL_REQ_MIN_CONN_INTERVAL = BLE_MSEC_TO_UNITS(30, BLE_UNIT_1_25_MS);
    const uint16_t BLE_CTRL_REQ_MAX_CONN_INTERVAL = BLE_MSEC_TO_UNITS(50, BLE_UNIT_1_25_MS);
    const uint16_t BLE_CTRL_REQ_SLAVE_LATENCY = 0;
    const uint16_t BLE_CTRL_REQ_CONN_SUP_TIMEOUT = BLE_MSEC_TO_UNITS(5000, BLE_UNIT_10_MS);

    const uint16_t BLE_CTRL_REQ_ADV_INTERVAL = BLE_MSEC_TO_UNITS(20, BLE_UNIT_0_625_MS); // Advertising interval: 20ms
    const uint16_t BLE_CTRL_REQ_ADV_TIMEOUT = BLE_MSEC_TO_UNITS(1000, BLE_UNIT_10_MS); // Advertising timeout: 1000ms

    const int8_t BLE_CTRL_REQ_TX_POWER = 0; //0dBm

    const uint8_t BLE_CTRL_REQ_SVC_UUID[BLE_SIG_UUID_128BIT_LEN] = {0xfc,0x36,0x6f,0x54,0x30,0x80,0xf4,0x94,0xa8,0x48,0x4e,0x5c,0x01,0x00,0xa9,0x6f};

    Vector<uint8_t> preAdvData_;
    Vector<uint8_t> preSrData_;
    hal_ble_adv_params_t preAdvParams_;
    int8_t preTxPower_;
    hal_ble_conn_params_t prePpcp_;
    bool preAdvertising_;
    bool preConnected_;
    hal_ble_auto_adv_cfg_t preAutoAdv_;
    bool userAdv_;
    bool restoreUserConfig_;
    static bool exited_;
    bool provMode_;
    uint16_t customCompanyId_;
    bool btStackInitialized_;

    Vector<uint8_t> ctrlReqAdvData_;
    Vector<uint8_t> ctrlReqSrData_;
};

} } /* particle::system */

#endif /* HAL_PLATFORM_BLE_SETUP */

#endif /* BLE_PROVISIONING_MODE_HANDLER_H */
