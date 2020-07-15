/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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

#include "error_message.h"

#include <cstdio>
#include <cstring>
#include <cstdarg>

namespace particle::system {

namespace {

const size_t MESSAGE_BUFFER_SIZE = 128;

char g_msg[MESSAGE_BUFFER_SIZE] = {};

} // namespace

void setErrorMessage(const char* msg) {
    const size_t n = strnlen(msg, MESSAGE_BUFFER_SIZE - 1);
    memcpy(g_msg, msg, n);
    g_msg[n] = '\0';
}

void formatErrorMessage(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const int n = vsnprintf(g_msg, MESSAGE_BUFFER_SIZE, fmt, args);
    va_end(args);
    if (n < 0) {
        g_msg[0] = '\0';
    }
}

const char* errorMessage() {
    return g_msg;
}


const char* errorMessage(int code) {
    if (!g_msg[0]) {
        formatErrorMessage("%s (%d)", system_error_message(code, nullptr), code);
    }
    return g_msg;
}

void clearErrorMessage() {
    g_msg[0] = '\0';
}

} // namespace particle::system
