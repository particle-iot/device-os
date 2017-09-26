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

#ifdef __cplusplus
extern "C" {
#endif

// System data sources
typedef enum diag_id {
    DIAG_ID_INVALID = 0, // Invalid source ID
    DIAG_ID_SYSTEM_LAST_RESET_REASON = 1, // sys.lastReset
    DIAG_ID_SYSTEM_FREE_MEMORY = 2, // sys.freeMem
    DIAG_ID_SYSTEM_BATTERY_CHARGE = 3, // sys.battCharge
    DIAG_ID_SYSTEM_SYSTEM_LOOPS = 4, // sys.loops
    DIAG_ID_SYSTEM_APPLICATION_LOOPS = 5, // sys.appLoops
    DIAG_ID_SYSTEM_UPTIME = 6, // sys.uptime
    DIAG_ID_SYSTEM_BATTERY_STATE = 7, // sys.battState
    DIAG_ID_NETWORK_CONNECTION_STATUS = 8, // net.status
    DIAG_ID_NETWORK_CONNECTION_ERROR_CODE = 9, // net.error
    DIAG_ID_NETWORK_DISCONNECTS = 12, // net.disconn
    DIAG_ID_NETWORK_IPV4_ADDRESS = 15, // net.ipAddr
    DIAG_ID_NETWORK_IPV4_GATEWAY = 16, // net.ipGateway
    DIAG_ID_NETWORK_FLAGS = 17, // net.flags
    DIAG_ID_NETWORK_COUNTRY_CODE = 18, // net.country
    DIAG_ID_NETWORK_RSSI = 19, // net.rssi
    DIAG_ID_CLOUD_CONNECTION_STATUS = 10, // cloud.status
    DIAG_ID_CLOUD_CONNECTION_ERROR_CODE = 13, // cloud.error
    DIAG_ID_CLOUD_DISCONNECTS = 14, // cloud.disconn
    DIAG_ID_CLOUD_REPEATED_MESSAGES = 21, // cloud.protoRepeat
    DIAG_ID_CLOUD_UNACKNOWLEDGED_MESSAGES = 22, // cloud.protoUnack
    DIAG_ID_CLOUD_RATE_LIMITED_EVENTS = 20, // cloud.rateLimited
    DIAG_ID_USER = 32768 // Base value for application-specific source IDs
} diag_id;

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
typedef int(*diag_enum_sources_callback)(const diag_source* src, void* data);

typedef struct diag_source {
    uint16_t size; // Size of this structure
    uint16_t flags; // Source flags
    uint16_t id; // Source ID
    uint16_t type; // Data type
    const char* name; // Source name
    void* data; // User data
    diag_source_cmd_callback callback; // Source callback
} diag_source;

typedef struct diag_source_get_cmd_data {
    uint16_t size; // Size of this structure
    uint16_t reserved; // Reserved (should be set to 0)
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
