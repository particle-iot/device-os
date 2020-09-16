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

#include "logging.h"
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
        (FLASH_IO, "Flash IO error", -219), \
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
        (DEPRECATED, "Deprecated", -300), \
        (COAP, "CoAP error", -1000), /* -1199 ... -1000: CoAP errors */ \
        (COAP_4XX, "CoAP: 4xx", -1100), \
        (COAP_5XX, "CoAP: 5xx", -1132), \
        (AT_NOT_OK, "AT command failure", -1200), /* -1399 ... -1200: AT command errors */ \
        (AT_RESPONSE_UNEXPECTED, "Failed to parse AT response", -1210), \
        (OTA_MODULE_NOT_FOUND, "Module not found", -1300), /* -1499 ... -1300: OTA update errors */ \
        (OTA_UNSUPPORTED_MODULE, "Unsupported module", -1310), \
        (OTA_VALIDATION_FAILED, "Module validation failed", -1320), \
        (OTA_INTEGRITY_CHECK_FAILED, "Module integrity check failed", -1330), \
        (OTA_RESUMED_UPDATE_FAILED, "Resumed update failed", -1331), \
        (OTA_DEPENDENCY_CHECK_FAILED, "Module dependency check failed", -1340), \
        (OTA_INVALID_ADDRESS, "Invalid module address", -1350), \
        (OTA_INVALID_SIZE, "Invalid module size", -1351), \
        (OTA_INVALID_PLATFORM, "Invalid module platform", -1360), \
        (OTA_INVALID_FORMAT, "Invalid module format", -1370), \
        (OTA_UPDATES_DISABLED, "Firmware updates are disabled", -1380), \
        (CRYPTO, "Crypto error", -1400) /* -1599 ... -1400: Crypto errors */ \

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

/**
 * Set the last error message.
 *
 * This macro also logs the error message under the current logging category.
 */
#define ERROR_MESSAGE(_fmt, ...) \
        do { \
            LOG(ERROR, _fmt, ##__VA_ARGS__); \
            set_error_message(_fmt, ##__VA_ARGS__); \
        } while (false)

/**
 * Error codes.
 */
typedef enum system_error_t {
    // SYSTEM_ERROR_NONE = 0,
    // SYSTEM_ERROR_UNKNOWN = -100,
    // ...
    SYSTEM_ERROR_ENUM_VALUES(SYSTEM_ERROR_)
} system_error_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Set the last error message.
 *
 * @param fmt Format string.
 * @param ... Formatting arguments.
 */
// TODO: Perhaps it would be better to not allow overriding the currently set error message until
// it's explicitly cleared
void set_error_message(const char* fmt, ...);

/**
 * Clear the last error message.
 */
void clear_error_message();

/**
 * Get the last error message.
 *
 * @param error Error code.
 * @return Error message.
 *
 * If `error` is negative and the last error message is not set, this function will return the
 * default message for the error code.
 */
const char* get_error_message(int error);

/**
 * Get the default error message for the error code.
 *
 * @param error Error code.
 * @return Error message.
 */
const char* get_default_error_message(int error, void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SERVICES_SYSTEM_ERROR_H
