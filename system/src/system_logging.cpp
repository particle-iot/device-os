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

#include "system_logging.h"
#include "check.h"

namespace {

log_command_handler_fn g_logCommandHandler = nullptr;
void* g_logCommandHandlerData = nullptr;

} // unnamed

void log_set_command_handler(log_command_handler_fn handler, void* user_data, void* reserved) {
    g_logCommandHandler = handler;
    g_logCommandHandlerData = user_data;
}

int log_process_command(const log_command* cmd, log_command_result** result) {
    if (!g_logCommandHandler) {
        return SYSTEM_ERROR_DISABLED;
    }
    return g_logCommandHandler(cmd, result, g_logCommandHandlerData);
}
