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
#include "system_error.h"
#include "check.h"

namespace {

log_config_callback g_logConfigCallback = nullptr;
void* g_logConfigCallbackData = nullptr;

} // unnamed

void log_config_set_callback(log_config_callback callback, void* user_data, void* reserved) {
    g_logConfigCallback = callback;
    g_logConfigCallbackData = user_data;
}

int log_config(int command, const void* command_data, void* command_result) {
    CHECK_TRUE(g_logConfigCallback, SYSTEM_ERROR_NOT_SUPPORTED);
    return g_logConfigCallback(command, command_data, command_result, g_logConfigCallbackData);
}
