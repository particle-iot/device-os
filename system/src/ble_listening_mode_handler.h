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

#ifndef BLE_LISTENING_MODE_HANDLER_H
#define BLE_LISTENING_MODE_HANDLER_H

#include "hal_platform.h"

#if HAL_PLATFORM_BLE

#include <memory>
#include "ble_hal.h"

namespace particle { namespace system {

class BleListeningModeHandler {
public:
    BleListeningModeHandler();
    ~BleListeningModeHandler();

    int enter();
    int exit();

private:
    std::unique_ptr<uint8_t[]> preAdvData_;
    size_t preAdvDataLen_;
    std::unique_ptr<uint8_t[]> preSrData_;
    size_t preSrDataLen_;
    hal_ble_adv_params_t preAdvParams_;
    hal_ble_conn_params_t prePpcp_;
    bool preAdvertising_;
    bool preConnected_;
    hal_ble_auto_adv_cfg_t preAutoAdv_;
};

} } /* particle::system */

#endif /* HAL_PLATFORM_BLE */

#endif /* BLE_LISTENING_MODE_HANDLER_H */
