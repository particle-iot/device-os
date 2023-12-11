/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#pragma once

#include <stdint.h>
#include "appender.h"
#include "ota_flash_hal.h"

#ifdef __cplusplus

namespace particle {
namespace system {

inline const char* module_function_string(module_function_t func) {
    switch (func) {
        case MODULE_FUNCTION_NONE: return "n";
        case MODULE_FUNCTION_RESOURCE: return "r";
        case MODULE_FUNCTION_BOOTLOADER: return "b";
        case MODULE_FUNCTION_MONO_FIRMWARE: return "m";
        case MODULE_FUNCTION_SYSTEM_PART: return "s";
        case MODULE_FUNCTION_USER_PART: return "u";
        case MODULE_FUNCTION_NCP_FIRMWARE: return "c";
        case MODULE_FUNCTION_RADIO_STACK: return "a";
        default: return "_";
    }
}

inline const char* module_store_string(module_store_t store) {
    switch (store) {
        case MODULE_STORE_MAIN: return "m";
        case MODULE_STORE_BACKUP: return "b";
        case MODULE_STORE_FACTORY: return "f";
        case MODULE_STORE_SCRATCHPAD: return "t";
        default: return "_";
    }
}

inline bool is_module_function_valid(module_function_t func) {
    switch (func) {
        case MODULE_FUNCTION_RESOURCE:
        case MODULE_FUNCTION_BOOTLOADER:
        case MODULE_FUNCTION_MONO_FIRMWARE:
        case MODULE_FUNCTION_SYSTEM_PART:
        case MODULE_FUNCTION_USER_PART:
        case MODULE_FUNCTION_NCP_FIRMWARE:
        case MODULE_FUNCTION_RADIO_STACK: {
            return true;
        }
        case MODULE_FUNCTION_NONE:
        default: {
            return false;
        }
    }
}

inline int module_function_from_string(const char* str) {
    switch (*str) {
        case 'n': return MODULE_FUNCTION_NONE;
        case 'r': return MODULE_FUNCTION_RESOURCE;
        case 'b': return MODULE_FUNCTION_BOOTLOADER;
        case 'm': return MODULE_FUNCTION_MONO_FIRMWARE;
        case 's': return MODULE_FUNCTION_SYSTEM_PART;
        case 'u': return MODULE_FUNCTION_USER_PART;
        case 'c': return MODULE_FUNCTION_NCP_FIRMWARE;
        case 'a': return MODULE_FUNCTION_RADIO_STACK;
        case '_':
        // fall-through
        default: return SYSTEM_ERROR_UNKNOWN;
    }
}

inline int module_store_from_string(const char* str) {
    switch (*str) {
        case 'm': return MODULE_STORE_MAIN;
        case 'b': return MODULE_STORE_BACKUP;
        case 'f': return MODULE_STORE_FACTORY;
        case 't': return MODULE_STORE_SCRATCHPAD;
        case '_':
        // fall-through
        default: return SYSTEM_ERROR_UNKNOWN;
    }
}


} // system
} // particle

extern "C" {
#endif // __cplusplus

typedef enum {
    MODULE_INFO_JSON_INCLUDE_PLATFORM_ID = 0x0001
} module_info_json_flags_t;

bool append_system_version_info(particle::Appender* appender);

bool system_module_info(appender_fn appender, void* append_data, void* reserved);
bool system_module_info_pb(appender_fn appender, void* append_data, void* reserved);

bool system_app_info(appender_fn appender, void* append_data, void* reserved);

#if !defined(PARTICLE_USER_MODULE) || defined(PARTICLE_USE_UNSTABLE_API)
// These functions are exported through dynalib but may be unstable due to the usage
// of internal structures.
int system_info_get_unstable(hal_system_info_t* info, uint32_t flags, void* reserved);
int system_info_free_unstable(hal_system_info_t* info, void* reserved);
#endif // !defined(PARTICLE_USER_MODULE) || defined(PARTICLE_USE_UNSTABLE_API)

/**
 * Formats the diagnostic data using an appender function.
 *
 * @param id Array of data source IDs. This argument can be set to NULL to format all registered data sources.
 * @param count Number of data source IDs in the array.
 * @param flags Formatting flags.
 * @param append Appender function.
 * @param append_data Opaque data passed to the appender function.
 * @param reserved Reserved argument (should be set to NULL).
 */
int system_format_diag_data(const uint16_t* id, size_t count, unsigned flags, appender_fn append, void* append_data,
        void* reserved);

bool system_metrics(appender_fn appender, void* append_data, uint32_t flags, uint32_t page, void* reserved);

#ifdef __cplusplus
}
#endif // __cplusplus
