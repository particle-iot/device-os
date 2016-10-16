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

#ifndef SERVICES_SYSTEM_ERROR_H
#define SERVICES_SYSTEM_ERROR_H

#include "preprocessor.h"

// List of all defined system errors
#define SYSTEM_ERRORS \
        (NONE, "", 0), \
        (UNKNOWN, "Unknown error", 100), \
        (TIMEOUT, "Timeout", 200), \
        (BUSY, "Busy", 300), \
        (CANCELLED, "Cancelled", 400), \
        (ABORTED, "Aborted", 500), \
        (NOT_SUPPORTED, "Not supported", 600), \
        (NOT_ALLOWED, "Not allowed", 700), \
        (QUOTA_EXCEEDED, "Quota exceeded", 800), \
        (NETWORK_ERROR, "Network error", 900)

// Expands to enum values for all errors
#define SYSTEM_ERROR_ENUM_VALUES(prefix) \
        PP_FOR_EACH(_SYSTEM_ERROR_ENUM_VALUE, prefix, SYSTEM_ERRORS)

#define _SYSTEM_ERROR_ENUM_VALUE(prefix, tuple) \
        _SYSTEM_ERROR_ENUM_VALUE_(prefix, PP_ARGS(tuple))

// Intermediate macro expanding PP_ARGS(tuple)
#define _SYSTEM_ERROR_ENUM_VALUE_(...) \
        _SYSTEM_ERROR_ENUM_VALUE__(__VA_ARGS__)

#define _SYSTEM_ERROR_ENUM_VALUE__(prefix, name, msg, code) \
        PP_CAT(prefix, name) = code,

#ifdef __cplusplus
extern "C" {
#endif

typedef enum system_error {
    // SYSTEM_ERROR_NONE = 0,
    // SYSTEM_ERROR_UNKNOWN = 100,
    // ...
    SYSTEM_ERROR_ENUM_VALUES(SYSTEM_ERROR_)
} system_error;

// Returns default error message
const char* system_error_message(int error, void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SERVICES_SYSTEM_ERROR_H
