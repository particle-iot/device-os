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

#ifndef SYSTEM_LISTENING_MODE_H
#define SYSTEM_LISTENING_MODE_H

#include "hal_platform.h"

#if HAL_PLATFORM_IFAPI

#include <atomic>
#include "system_network.h"
#include "active_object.h"
#include "system_setup.h"
#include <memory>
#include "system_tick_hal.h"

#if HAL_PLATFORM_BLE
#include "ble_hal.h"
#endif /* HAL_PLATFORM_BLE */

namespace particle { namespace system {\

#if HAL_PLATFORM_BLE
class BleListeningModeHandler {
public:
    BleListeningModeHandler();
    ~BleListeningModeHandler();

    int enter();
    int exit();

private:
    uint8_t* preAdvData_;
    size_t preAdvDataLen_;
    uint8_t* preSrData_;
    size_t preSrDataLen_;
    hal_ble_adv_params_t preAdvParams_;
    hal_ble_conn_params_t prePpcp_;
    bool preAdvertising_;
    bool preConnected_;
    hal_ble_auto_adv_cfg_t preAutoAdv_;
};
#endif

class ListeningModeHandler {
public:
    ListeningModeHandler();
    ~ListeningModeHandler();

    static ListeningModeHandler* instance();

    int enter(unsigned int timeout = 0);
    int exit();
    bool isActive() const;

    int run();

    int setTimeout(unsigned int timeout);
    unsigned int getTimeout() const;

    int command(network_listen_command_t com, void* arg);
    int enqueueCommand(network_listen_command_t com, void* arg);

private:
    struct Task : ISRTaskQueue::Task {
        network_listen_command_t command;
        void* arg;
    };

    static void executeEnqueuedCommand(Task* task);

    int clearNetworkConfiguration() const;

private:
    std::atomic_bool active_;

    std::unique_ptr<SystemSetupConsoleBase> console_;
    system_tick_t timestampStarted_;
    system_tick_t timestampUpdate_;

#if HAL_PLATFORM_BLE
    BleListeningModeHandler bleHandler_;
#endif
};

} } /* particle::system */

#endif /* HAL_PLATFORM_IFAPI */

#endif /* SYSTEM_LISTENING_MODE_H */
