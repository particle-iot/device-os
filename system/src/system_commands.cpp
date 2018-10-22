#include "system_commands.h"
#include "spark_wiring_ticks.h"
#include "system_cloud_connection.h"
#include "system_cloud.h"
#include "system_network.h"
#if HAL_PLATFORM_MESH
#include "system_openthread.h"
#include "border_router_manager.h"
#endif // HAL_PLATFORM_MESH

namespace particle {
namespace system {

using particle::fs::FileQueue;

static FileQueue persistCommands("commands.bin");

system_tick_t nextCommandTime = 0;
const system_tick_t COMMAND_EXECUTION_TIMEOUT = 60*3*1000;
const system_tick_t DELAY_BETWEEN_COMMAND_CHECKS = 5000;

void scheduleNextCommand(system_tick_t timeout = 0) {
    nextCommandTime = millis() + timeout;
}

#if HAL_PLATFORM_MESH

void destroyMeshCredentialsIfNetworkIdMatches(const char* networkId) {
    char nId[MESH_NETWORK_ID_LENGTH + 1] = {};
    uint16_t sz = sizeof(nId);
    if (!threadGetNetworkId(threadInstance(), nId, &sz)) {
        if (!memcmp(nId, networkId, sizeof(nId))) {
            LOG(ERROR, "Destroying Mesh credentials due to rejected system command");
            network_disconnect(NETWORK_INTERFACE_MESH, NETWORK_DISCONNECT_REASON_ERROR, nullptr);
            network_clear_credentials(NETWORK_INTERFACE_MESH, 0, nullptr, nullptr);
        }
    }
}

inline bool isCoap4xxError(int error) {
    return (error == SYSTEM_ERROR_COAP_4XX);
}

void handleMeshNetworkJoinedComplete(int error, const void* data, void* callback_data, void* reserved) {
    auto cmd = static_cast<NotifyMeshNetworkJoined*>(callback_data);
    LOG(INFO, "Network %s command complete, result %d", cmd->joined ? "joined" : "left", error);
    if (!error) {
        persistCommands.popFront();
        scheduleNextCommand();
    } else if (isCoap4xxError(error)) {
        persistCommands.popFront();
        destroyMeshCredentialsIfNetworkIdMatches(cmd->nu.id);
        scheduleNextCommand();
    } else {
        scheduleNextCommand(DELAY_BETWEEN_COMMAND_CHECKS);
    }
}

void handleMeshNetworkUpdatedComplete(int error, const void* data, void* callback_data, void* reserved) {
    auto cmd = static_cast<NotifyMeshNetworkUpdated*>(callback_data);
    LOG(INFO, "Network updated command complete, result %d", error);
    if (!error) {
        persistCommands.popFront();
        scheduleNextCommand();
    } else if (isCoap4xxError(error)) {
        persistCommands.popFront();
        destroyMeshCredentialsIfNetworkIdMatches(cmd->ni.id);
        scheduleNextCommand();
    } else {
        scheduleNextCommand(DELAY_BETWEEN_COMMAND_CHECKS);
    }
}

void handleCommandComplete(int error, const void* data, void* callback_data, void* reserved) {
	if (!error) {
		persistCommands.popFront();
		scheduleNextCommand();
	} else if (isCoap4xxError(error)) {
		persistCommands.popFront();
		scheduleNextCommand();
	} else {
		scheduleNextCommand(DELAY_BETWEEN_COMMAND_CHECKS);
	}
}

void handleMeshNetworkGatewayComplete(int error, const void* data, void* callback_data, void* reserved) {
	handleCommandComplete(error, data, callback_data, reserved);
	if (error) {
		particle::net::BorderRouterManager::instance()->stop();
		LOG(WARN, "Gateway operation vetoed.");
	}
	else {
		LOG(INFO, "Gateway operation confirmed.");
	}
}

#endif // HAL_PLATFORM_MESH

void fetchAndExecuteCommand(system_tick_t currentTime) {
    if ((int(currentTime)-int(nextCommandTime))<0)
        return;

    FileQueue::QueueEntry entry;
    static AllCommands cmd;
    // execution is asynchronous. The CallbackHandler is invoked to deliver the asynchronous result.
    if (spark_cloud_flag_connected() && !persistCommands.front(entry, &cmd, sizeof(cmd)) && !cmd.execute()) {
        nextCommandTime = currentTime + COMMAND_EXECUTION_TIMEOUT;
    } else {
        nextCommandTime = currentTime + DELAY_BETWEEN_COMMAND_CHECKS;
    }
}

int system_command_enqueue(SystemCommand& cmd, uint16_t size) {
    int result = persistCommands.pushBack(&cmd, size);
	if (result<0) {
		LOG(ERROR, "Unable to enqueue command %d size %d to system command queue. Error=%d", cmd.commandType, size, result);
	}
	else {
		LOG(INFO, "Added command %d size %d to system command queue.", cmd.commandType, size);
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
