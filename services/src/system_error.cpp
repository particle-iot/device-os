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
        PP_FOR_EACH(_SYSTEM_ERROR_MESSAGE_SWITCH_CASE, SYSTEM_ERRORS)

#define _SYSTEM_ERROR_MESSAGE_SWITCH_CASE(tuple) \
        __SYSTEM_ERROR_MESSAGE_SWITCH_CASE(PP_ARGS(tuple))

// Intermediate macro expanding PP_ARGS(tuple)
#define __SYSTEM_ERROR_MESSAGE_SWITCH_CASE(...) \
        ___SYSTEM_ERROR_MESSAGE_SWITCH_CASE(__VA_ARGS__)

#define ___SYSTEM_ERROR_MESSAGE_SWITCH_CASE(name, msg, code) \
        case code: return msg;

const char* system_error_message(int error, void* reserved) {
    switch (error) {
        SYSTEM_ERROR_MESSAGE_SWITCH_CASES()
        default: return ""; // Unknown error
    }
}
