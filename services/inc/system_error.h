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
        (NONE, "", 0), /* -999 ... 0: Generic result codes */ \
        (UNKNOWN, "Unknown error", -100), \
        (BUSY, "Resource busy", -110), \
        (NOT_SUPPORTED, "Not supported", -120), \
        (NOT_ALLOWED, "Not allowed", -130), \
        (CANCELLED, "Operation cancelled", -140), \
        (ABORTED, "Operation aborted", -150), \
        (TIMEOUT, "Timeout error", -160), \
        (NOT_FOUND, "Not found", -170), \
        (ALREADY_EXISTS, "Already exists", -180), \
        (TOO_LARGE, "Too large data", -190), \
        (NOT_ENOUGH_DATA, "Not enough data", -191), \
        (LIMIT_EXCEEDED, "Limit exceeded", -200), \
        (END_OF_STREAM, "End of stream", -201), \
        (INVALID_STATE, "Invalid state", -210), \
        (IO, "IO error", -220), \
        (WOULD_BLOCK, "Would block", -221), \
        (FILE, "File error", -225), \
        (NETWORK, "Network error", -230), \
        (PROTOCOL, "Protocol error", -240), \
        (INTERNAL, "Internal error", -250), \
        (NO_MEMORY, "Memory allocation error", -260), \
        (INVALID_ARGUMENT, "Invalid argument", -270), \
        (BAD_DATA, "Invalid data format", -280), \
        (OUT_OF_RANGE, "Out of range", -290), \
        (COAP, "CoAP error", -1000), /* -1199 ... -1000: CoAP errors */ \
        (COAP_4XX, "CoAP: 4xx", -1100), \
        (COAP_5XX, "CoAP: 5xx", -1132), \
        (AT_NOT_OK, "AT command failure", -1200), /* -1399 ... -1200: AT command errors */ \
        (AT_RESPONSE_UNEXPECTED, "Failed to parse AT response", -1210)

// Expands to enum values for all errors
#define SYSTEM_ERROR_ENUM_VALUES(prefix) \
        PP_FOR_EACH(_SYSTEM_ERROR_ENUM_VALUE, prefix, SYSTEM_ERRORS)

#define _SYSTEM_ERROR_ENUM_VALUE(prefix, tuple) \
        _SYSTEM_ERROR_ENUM_VALUE_(prefix, PP_ARGS(tuple))

// Intermediate macro used to expand PP_ARGS(tuple)
#define _SYSTEM_ERROR_ENUM_VALUE_(...) \
        _SYSTEM_ERROR_ENUM_VALUE__(__VA_ARGS__)

#define _SYSTEM_ERROR_ENUM_VALUE__(prefix, name, msg, code) \
        PP_CAT(prefix, name) = code,

#ifdef __cplusplus
extern "C" {
#endif

typedef enum system_error_t {
    // SYSTEM_ERROR_NONE = 0,
    // SYSTEM_ERROR_UNKNOWN = -100,
    // ...
    SYSTEM_ERROR_ENUM_VALUES(SYSTEM_ERROR_)
} system_error_t;

// Returns default error message
const char* system_error_message(int error, void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SERVICES_SYSTEM_ERROR_H
