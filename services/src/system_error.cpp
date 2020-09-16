/*
 * Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.
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

#include "system_error.h"

#include "hal_platform.h"

#if HAL_PLATFORM_ERROR_MESSAGES

// The system threading API cannot be accessed directly on platforms where this module
// is linked dynamically
static_assert(!HAL_PLATFORM_MULTIPART_SYSTEM, "This module cannot be linked dynamically");

#include "system_threading.h"

#include <cstdio>

#endif // HAL_PLATFORM_ERROR_MESSAGES

#define SYSTEM_ERROR_MESSAGE_SWITCH_CASES() \
        PP_FOR_EACH(_SYSTEM_ERROR_MESSAGE_SWITCH_CASE, /* data */, SYSTEM_ERRORS)

#define _SYSTEM_ERROR_MESSAGE_SWITCH_CASE(data, tuple) \
        _SYSTEM_ERROR_MESSAGE_SWITCH_CASE_(PP_ARGS(tuple))

// Intermediate macro used to expand PP_ARGS(tuple)
#define _SYSTEM_ERROR_MESSAGE_SWITCH_CASE_(...) \
        _SYSTEM_ERROR_MESSAGE_SWITCH_CASE__(__VA_ARGS__)

#define _SYSTEM_ERROR_MESSAGE_SWITCH_CASE__(name, msg, code) \
        case code: return msg;

namespace {

#if HAL_PLATFORM_ERROR_MESSAGES

const size_t ERROR_MESSAGE_BUFFER_SIZE = 128;

// TODO: Use thread-local storage
char g_errorMsg[ERROR_MESSAGE_BUFFER_SIZE] = {};

#endif // HAL_PLATFORM_ERROR_MESSAGES

} // namespace

void set_error_message(const char* fmt, ...) {
#if HAL_PLATFORM_ERROR_MESSAGES
    if (SYSTEM_THREAD_CURRENT()) {
        va_list args;
        va_start(args, fmt);
        const int n = vsnprintf(g_errorMsg, ERROR_MESSAGE_BUFFER_SIZE, fmt, args);
        va_end(args);
        if (n < 0) {
            g_errorMsg[0] = '\0';
        }
    }
#endif
}

void clear_error_message() {
#if HAL_PLATFORM_ERROR_MESSAGES
    if (SYSTEM_THREAD_CURRENT()) {
        g_errorMsg[0] = '\0';
    }
#endif
}

const char* get_error_message(int error) {
#if HAL_PLATFORM_ERROR_MESSAGES
    if (!SYSTEM_THREAD_CURRENT() || !g_errorMsg[0]) {
        return get_default_error_message(error, nullptr /* reserved */);
    }
    return g_errorMsg;
#else
    return get_default_error_message(error, nullptr);
#endif
}

const char* get_default_error_message(int error, void* reserved) {
    if (error < 0) {
#if HAL_PLATFORM_ERROR_MESSAGES
        switch (error) {
        SYSTEM_ERROR_MESSAGE_SWITCH_CASES()
        default:
            return "Unknown error";
        }
#else
        return "Unknown error";
#endif
    }
    return "";
}
