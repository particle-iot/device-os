#include "system_commands.h"
#include "spark_wiring_ticks.h"
#include "system_cloud_connection.h"

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
    AllCommands cmd;
    if (system_cloud_is_connected(nullptr) && !persistCommands.front(entry, &cmd, sizeof(cmd)) && !cmd.execute()) {
        nextCommandTime = currentTime + COMMAND_EXECUTION_TIMEOUT;
    } else {
        nextCommandTime = currentTime + DELAY_BETWEEN_COMMAND_CHECKS;
    }
}

int system_command_enqueue(SystemCommand& cmd, uint16_t size) {
    return persistCommands.pushBack(&cmd, size);
}

} // namespace system
} // namespace particle
