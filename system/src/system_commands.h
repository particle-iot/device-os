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

#include "hal_platform.h"

#if HAL_PLATFORM_FILESYSTEM

#include "spark_protocol_functions.h"
#include "logging.h"
#include "file_queue.h"
#include "hal_platform.h"

namespace particle {
namespace system {

struct SystemCommand {
    enum Enum {
#if HAL_PLATFORM_MESH
        /* 0 */ NOTIFY_MESH_NETWORK,
        /* 1 */ NOTIFY_MESH_JOINED,
		/* 2 */ NOTIFY_MESH_GATEWAY
#endif
    };

    Enum commandType;
};


void fetchAndExecuteCommand(system_tick_t currentTime);

int system_command_enqueue(SystemCommand& cmd, uint16_t size);
int system_command_clear();


#if HAL_PLATFORM_MESH

void handleMeshNetworkJoinedComplete(int error, const void* data, void* callback_data, void* reserved);
void handleMeshNetworkUpdatedComplete(int error, const void* data, void* callback_data, void* reserved);
void handleMeshNetworkGatewayComplete(int error, const void* data, void* callback_data, void* reserved);

using namespace MeshCommand;

struct NotifyMeshNetworkUpdated : SystemCommand {
    NetworkInfo ni;

    NotifyMeshNetworkUpdated() :
            ni() {
        ni.update.size = sizeof(ni);
        commandType = NOTIFY_MESH_NETWORK;
    }

    int execute() {
        LOG(INFO, "Invoking network update command for networkId=%s", ni.update.id);
        completion_handler_data ch = {
            .size = sizeof(ch),
            .handler_callback = handleMeshNetworkUpdatedComplete,
            .handler_data = this
        };
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
        LOG(INFO, "Invoking network joined command, networkId=%s, joined=%d", nu.id, joined);
        completion_handler_data ch = {
            .size = sizeof(ch),
            .handler_callback = handleMeshNetworkJoinedComplete,
            .handler_data = this
        };
        return spark_protocol_mesh_command(spark_protocol_instance(), DEVICE_MEMBERSHIP, joined, &nu, &ch);
    }
};

struct NotifyMeshNetworkGateway : SystemCommand {
	NetworkUpdate nu;
	bool active;

	NotifyMeshNetworkGateway(bool active = true) :
            nu(),
            active(active) {
        nu.size = sizeof(nu);
        commandType = NOTIFY_MESH_GATEWAY;
    }

    int execute() {
        LOG(INFO, "Invoking network gateway command, networkId=%s, active=%d", nu.id, active);
        completion_handler_data ch = { .size=sizeof(ch), .handler_callback = handleMeshNetworkGatewayComplete, .handler_data = nullptr };
        return spark_protocol_mesh_command(spark_protocol_instance(), DEVICE_BORDER_ROUTER, active, &nu, &ch);
    }

};

#endif // PLATFORM_MESH

union AllCommands {
#if HAL_PLATFORM_MESH
    SystemCommand base;
    NotifyMeshNetworkJoined joined;
    NotifyMeshNetworkUpdated created;
    NotifyMeshNetworkGateway gateway;
#endif
    AllCommands() {
    }
    int execute();
};


} // system
} // particle

#endif // HAL_PLATFORM_FILESYSTEM
