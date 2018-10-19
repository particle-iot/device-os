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

#pragma once

#include "spark_protocol_functions.h"
#include "logging.h"
#include "file_queue.h"
#include "hal_platform.h"

namespace particle {
namespace system {

struct SystemCommand {
    enum Enum {
#if HAL_PLATFORM_MESH
        NOTIFY_MESH_NETWORK,
        NOTIFY_MESH_JOINED,
#endif
    };

    Enum commandType;
};


void fetchAndExecuteCommand(system_tick_t currentTime);
void handleCommandComplete(int error, const void* data, void* callback_data, void* reserved);

int system_command_enqueue(SystemCommand& cmd, uint16_t size);
int system_command_clear();


#if HAL_PLATFORM_MESH

using namespace MeshCommand;

struct NotifyMeshNetworkUpdated : SystemCommand {
    NetworkInfo ni;

    NotifyMeshNetworkUpdated() :
            ni() {
        ni.update.size = sizeof(ni);
        commandType = NOTIFY_MESH_NETWORK;
    }

    int execute() {
        LOG(INFO, "Invoking network update command");
        completion_handler_data ch = { .size=sizeof(ch), .handler_callback = handleCommandComplete, .handler_data = nullptr };
        return spark_protocol_mesh_command(spark_protocol_instance(), NETWORK_UPDATED, 0, &ni, &ch);
    }
};

struct NotifyMeshNetworkJoined : SystemCommand {
    NetworkUpdate nu;
    bool joined;

    NotifyMeshNetworkJoined() :
            nu(),
            joined(false) {
        nu.size = sizeof(nu);
        commandType = NOTIFY_MESH_JOINED;
    }

    int execute() {
        LOG(INFO, "Invoking network joined command, joined=%d", joined);
        completion_handler_data ch = { .size=sizeof(ch), .handler_callback = handleCommandComplete, .handler_data = nullptr };
        return spark_protocol_mesh_command(spark_protocol_instance(), DEVICE_MEMBERSHIP, joined, &nu, &ch);
    }
};

#endif // PLATFORM_MESH

union AllCommands {
#if HAL_PLATFORM_MESH
    SystemCommand base;
    NotifyMeshNetworkJoined joined;
    NotifyMeshNetworkUpdated created;
#endif
    AllCommands() {
    }
    int execute();
};


} // system
} // particle
