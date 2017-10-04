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

#define SYSTEM_ERROR_MESSAGE_SWITCH_CASES() \
        PP_FOR_EACH(_SYSTEM_ERROR_MESSAGE_SWITCH_CASE, /* data */, SYSTEM_ERRORS)

#define _SYSTEM_ERROR_MESSAGE_SWITCH_CASE(data, tuple) \
        _SYSTEM_ERROR_MESSAGE_SWITCH_CASE_(PP_ARGS(tuple))

// Intermediate macro used to expand PP_ARGS(tuple)
#define _SYSTEM_ERROR_MESSAGE_SWITCH_CASE_(...) \
        _SYSTEM_ERROR_MESSAGE_SWITCH_CASE__(__VA_ARGS__)

#define _SYSTEM_ERROR_MESSAGE_SWITCH_CASE__(name, msg, code) \
        case code: return msg;

const char* system_error_message(int error, void* reserved) {
    switch (error) {
#if PLATFORM_ID == 3
        SYSTEM_ERROR_MESSAGE_SWITCH_CASES()
#endif
        default:
            return "Unknown error";
    }
}
