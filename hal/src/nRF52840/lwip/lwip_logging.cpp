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

#include "lwip_logging.h"
#include <cstring>
#include "logging.h"

uint32_t g_lwip_debug_flags = 0xffffffff;

void lwip_log_message(const char* fmt, ...) {
    LogAttributes attr = {};
    va_list args;
    va_start(args, fmt);

    char tmp[LOG_MAX_STRING_LENGTH] = {};
    strncpy(tmp, fmt, sizeof(tmp));
    tmp[strcspn(tmp, "\r\n")] = 0;

    log_message_v(1, "lwip", &attr, nullptr /* reserved */, tmp, args);
    va_end(args);
}
