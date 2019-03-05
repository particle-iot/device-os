/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include "system_commands.h"

#if HAL_PLATFORM_FILESYSTEM

#include "spark_wiring_ticks.h"
#include "system_cloud_connection.h"
#include "system_cloud.h"
#include "system_network.h"

#if HAL_PLATFORM_MESH
#include "system_openthread.h"
#include "border_router_manager.h"
#include "system_network_manager.h"
#endif // HAL_PLATFORM_MESH

namespace particle {

namespace system {

namespace {

using particle::fs::FileQueue;

const system_tick_t DELAY_BETWEEN_COMMAND_CHECKS = 5000;

FileQueue persistCommands("commands.bin");

system_tick_t nextCommandTime = 0;

AllCommands g_cmd; // maybe this should be dynamically allocated
bool g_cmdPending = false;

void scheduleNextCommand(system_tick_t timeout = 0) {
    nextCommandTime = millis() + timeout;
}

inline bool isCoap4xxError(int error) {
    return (error == SYSTEM_ERROR_COAP_4XX);
}

bool handleCommandComplete(int error, void* data = nullptr, size_t size = 0) {
    int r = 0;
    if (data) {
        FileQueue::QueueEntry e;
        r = persistCommands.front(e, data, size);
    }
    if (!error || isCoap4xxError(error)) {
        persistCommands.popFront();
        scheduleNextCommand();
    } else {
        scheduleNextCommand(DELAY_BETWEEN_COMMAND_CHECKS);
    }
    g_cmdPending = false;
    return (r == 0);
}

} // unnamed

#if HAL_PLATFORM_MESH

void destroyMeshCredentialsIfNetworkIdMatches(const char* networkId) {
    char nId[MESH_NETWORK_ID_LENGTH + 1] = {};
    size_t sz = sizeof(nId);
    if (!ot_get_network_id(threadInstance(), nId, &sz)) {
        if (!memcmp(nId, networkId, sizeof(nId))) {
            LOG(ERROR, "Destroying Mesh credentials due to rejected system command");
            network_disconnect(NETWORK_INTERFACE_MESH, NETWORK_DISCONNECT_REASON_ERROR, nullptr);
            network_clear_credentials(NETWORK_INTERFACE_MESH, 0, nullptr, nullptr);
        }
    }
}

void handleMeshNetworkJoinedComplete(int error, const void* data, void* callback_data, void* reserved) {
    if (!handleCommandComplete(error, &g_cmd, sizeof(g_cmd))) {
        return;
    }
    const auto cmd = (const NotifyMeshNetworkJoined*)&g_cmd;
    LOG(INFO, "Network %s command complete, result %d", cmd->joined ? "joined" : "left", error);
    if (isCoap4xxError(error)) {
        destroyMeshCredentialsIfNetworkIdMatches(cmd->nu.id);
    }
}

void handleMeshNetworkUpdatedComplete(int error, const void* data, void* callback_data, void* reserved) {
    if (!handleCommandComplete(error, &g_cmd, sizeof(g_cmd))) {
        return;
    }
    const auto cmd = (const NotifyMeshNetworkUpdated*)&g_cmd;
    LOG(INFO, "Network updated command complete, result %d", error);
    if (isCoap4xxError(error)) {
        destroyMeshCredentialsIfNetworkIdMatches(cmd->ni.id);
    }
}

void handleMeshNetworkGatewayComplete(int error, const void* data, void* callback_data, void* reserved) {
    handleCommandComplete(error);
    LOG(INFO, "Gateway status command complete, result %d", error);
	if (error) {
        LOG(WARN, "Gateway operation vetoed");
		setBorderRouterPermitted(false);
	} else {
		LOG(INFO, "Gateway operation confirmed");
		setBorderRouterPermitted(true);
	}
}

#endif // HAL_PLATFORM_MESH

void fetchAndExecuteCommand(system_tick_t currentTime) {
    if (g_cmdPending || (int)currentTime - (int)nextCommandTime < 0) {
        return;
    }

    FileQueue::QueueEntry entry;

    // execution is asynchronous. The CallbackHandler is invoked to deliver the asynchronous result.
    if (spark_cloud_flag_connected() && !persistCommands.front(entry, &g_cmd, sizeof(g_cmd)) && !g_cmd.execute()) {
        g_cmdPending = true;
    } else {
        nextCommandTime = currentTime + DELAY_BETWEEN_COMMAND_CHECKS;
    }
}

int system_command_enqueue(SystemCommand& enqueue, uint16_t size) {

	FileQueue::QueueEntry entry;

	int error = persistCommands.front(entry, &g_cmd, sizeof(g_cmd));
	// skip enqueuing duplicate commands. Ideally each command itself should be able to filter the queue, but for now this will do.
	if (!error && !memcmp(&enqueue, &g_cmd, size)) {
		LOG(INFO, "Command %d size %d skipped because it is a duplicate", enqueue.commandType, size);
		return 0;
	}

	int result = persistCommands.pushBack(&enqueue, size);
	if (result<0) {
		LOG(ERROR, "Unable to enqueue command %d size %d to system command queue. Error=%d", enqueue.commandType, size, result);
	}
	else {
		LOG(INFO, "Added command %d size %d to system command queue", enqueue.commandType, size);
	}
	return result;
}

int system_command_clear() {
	LOG(INFO, "clearing persistent command queue");
	return persistCommands.clear();
}

int AllCommands::execute() {
    switch (base.commandType) {
    case SystemCommand::NOTIFY_MESH_NETWORK: return created.execute();
    case SystemCommand::NOTIFY_MESH_JOINED: return joined.execute();
    case SystemCommand::NOTIFY_MESH_GATEWAY: return gateway.execute();
    default:
        LOG(ERROR, "Ignoring unknown command type %d", base.commandType);
        return 0;
    }
}


} // namespace system
} // namespace particle

#endif // HAL_PLATFORM_FILESYSTEM
