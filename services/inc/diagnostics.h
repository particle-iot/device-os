/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
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
#include <stddef.h>

// System data sources
#define DIAG_SOURCE_INVALID 0 // Invalid source ID
#define DIAG_SOURCE_USER 1024 // Base value for application-specific source IDs

#ifdef __cplusplus
extern "C" {
#endif

// Data types
typedef enum diag_type {
    DIAG_TYPE_INT = 1 // 32-bit integer
} diag_type;

// Data source commands
typedef enum diag_source_cmd {
    DIAG_SOURCE_CMD_GET = 1 // Get current data
} diag_source_cmd;

// Service commands
typedef enum diag_service_cmd {
    DIAG_SERVICE_CMD_RESET = 1, // Reset the service (this command is used only for testing purposes)
    DIAG_SERVICE_CMD_START = 2 // Start the service
} diag_service_cmd;

typedef struct diag_source diag_source;

typedef int(*diag_source_cmd_callback)(const diag_source* src, int cmd, void* data);
typedef void(*diag_enum_sources_callback)(const diag_source* src, void* data);

typedef struct diag_source {
    size_t size; // Size of this structure
    uint16_t id; // Source ID
    uint16_t type; // Data type
    const char* name; // Source name
    uint32_t flags; // Reserved (should be set to 0)
    void* data; // User data
    diag_source_cmd_callback callback; // Source callback
} diag_source;

typedef struct diag_source_get_cmd_data {
    size_t size; // Size of this structure
    void* data; // Data buffer
    size_t data_size; // Buffer size
} diag_source_get_cmd_data;

// Registers a new data source. Note that in order for the data source to be registered, the service
// needs to be in its initial stopped state
int diag_register_source(const diag_source* src, void* reserved);

// Enumerates all registered data sources. The `callback` and `count` arguments can be set to NULL.
// This function returns an error if the service is not started
int diag_enum_sources(diag_enum_sources_callback callback, size_t* count, void* data, void* reserved);

// Retrieves a registered data source with a given ID. This function returns an error if the service
// is not started
int diag_get_source(uint16_t id, const diag_source** src, void* reserved);

// Issues a service command
int diag_command(int cmd, void* data, void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif
