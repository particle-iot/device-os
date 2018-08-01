#include "system_commands.h"
#include "spark_wiring_ticks.h"
#include "system_cloud_connection.h"
#include "system_cloud.h"

namespace particle {
namespace system {

using particle::fs::FileQueue;

static FileQueue persistCommands("commands.bin");

system_tick_t nextCommandTime = 0;
const system_tick_t COMMAND_EXECUTION_TIMEOUT = 60*3*1000;
const system_tick_t DELAY_BETWEEN_COMMAND_CHECKS = 5000;

/**
 * Completion handler callback. The current command is popped off the queue and the nexxt one attempted.
 */
void handleCommandComplete(int error, const void* data, void* callback_data, void* reserved) {
	LOG(INFO, "system command complete, result %d", error);
	if (!error) {
        persistCommands.popFront();
        nextCommandTime = millis();
    }
    else {
        nextCommandTime = millis()+DELAY_BETWEEN_COMMAND_CHECKS;
    }
}

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
    default:
        LOG(ERROR, "Ignoring unknown command type %d", base.commandType);
        return 0;
    }
}


} // namespace system
} // namespace particle
