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

#define DIAG_NAME_INVALID "invalid"
#define DIAG_NAME_SYSTEM_LAST_RESET_REASON "sys:reset"
#define DIAG_NAME_SYSTEM_FREE_MEMORY "mem:free"
#define DIAG_NAME_SYSTEM_BATTERY_CHARGE "batt:soc"
#define DIAG_NAME_SYSTEM_SYSTEM_LOOPS "sys:loop"
#define DIAG_NAME_SYSTEM_APPLICATION_LOOPS "app:loop"
#define DIAG_NAME_SYSTEM_UPTIME "sys:uptime"
#define DIAG_NAME_SYSTEM_BATTERY_STATE "batt:state"
#define DIAG_NAME_SYSTEM_POWER_SOURCE "pwr:src"
#define DIAG_NAME_NETWORK_CONNECTION_STATUS "net:stat"
#define DIAG_NAME_NETWORK_CONNECTION_ERROR_CODE "net:err"
#define DIAG_NAME_NETWORK_DISCONNECTS "net:dconn"
#define DIAG_NAME_NETWORK_CONNECTION_ATTEMPTS "net:connatt"
#define DIAG_NAME_NETWORK_DISCONNECTION_REASON "net:dconnrsn"
#define DIAG_NAME_NETWORK_IPV4_ADDRESS "net:ip:addr"
#define DIAG_NAME_NETWORK_IPV4_GATEWAY "net:ip:gw"
#define DIAG_NAME_NETWORK_FLAGS "net:flags"
#define DIAG_NAME_NETWORK_COUNTRY_CODE "net:cntry"
#define DIAG_NAME_NETWORK_RSSI "net:rssi"
#define DIAG_NAME_NETWORK_SIGNAL_STRENGTH "net:sigstr"
#define DIAG_NAME_NETWORK_SIGNAL_STRENGTH_VALUE "net:sigstrv"
#define DIAG_NAME_NETWORK_SIGNAL_QUALITY "net:sigqual"
#define DIAG_NAME_NETWORK_SIGNAL_QUALITY_VALUE "net:sigqualv"
#define DIAG_NAME_NETWORK_ACCESS_TECNHOLOGY "net:at"
#define DIAG_NAME_NETWORK_CELLULAR_CELL_GLOBAL_IDENTITY_MOBILE_COUNTRY_CODE "net:cell:cgi:mcc"
#define DIAG_NAME_NETWORK_CELLULAR_CELL_GLOBAL_IDENTITY_MOBILE_NETWORK_CODE "net:cell:cgi:mnc"
#define DIAG_NAME_NETWORK_CELLULAR_CELL_GLOBAL_IDENTITY_LOCATION_AREA_CODE "net:cell:cgi:lac"
#define DIAG_NAME_NETWORK_CELLULAR_CELL_GLOBAL_IDENTITY_CELL_ID "net:cell:cgi:ci"
#define DIAG_NAME_CLOUD_CONNECTION_STATUS "cloud:stat"
#define DIAG_NAME_CLOUD_CONNECTION_ERROR_CODE "cloud:err"
#define DIAG_NAME_CLOUD_DISCONNECTS "cloud:dconn"
#define DIAG_NAME_CLOUD_CONNECTION_ATTEMPTS "cloud:connatt"
#define DIAG_NAME_CLOUD_DISCONNECTION_REASON "cloud:dconnrsn"
#define DIAG_NAME_CLOUD_REPEATED_MESSAGES "coap:resend"
#define DIAG_NAME_CLOUD_UNACKNOWLEDGED_MESSAGES "coap:unack"
#define DIAG_NAME_CLOUD_RATE_LIMITED_EVENTS "pub:limit"
#define DIAG_NAME_SYSTEM_TOTAL_RAM "sys:tram"
#define DIAG_NAME_SYSTEM_USED_RAM "sys:uram"

#ifdef __cplusplus
extern "C" {
#endif

// System data sources
typedef enum diag_id {
    DIAG_ID_INVALID = 0, // Invalid source ID
    DIAG_ID_SYSTEM_LAST_RESET_REASON = 1, // sys:reset
    DIAG_ID_SYSTEM_FREE_MEMORY = 2, // mem:free
    DIAG_ID_SYSTEM_BATTERY_CHARGE = 3, // batt:soc
    DIAG_ID_SYSTEM_SYSTEM_LOOPS = 4, // sys:loops
    DIAG_ID_SYSTEM_APPLICATION_LOOPS = 5, // app:loops
    DIAG_ID_SYSTEM_UPTIME = 6, // sys:uptime
    DIAG_ID_SYSTEM_BATTERY_STATE = 7, // batt:state
    DIAG_ID_SYSTEM_POWER_SOURCE = 24, // pwr::src
    DIAG_ID_NETWORK_CONNECTION_STATUS = 8, // net:stat
    DIAG_ID_NETWORK_CONNECTION_ERROR_CODE = 9, // net:err
    DIAG_ID_NETWORK_DISCONNECTS = 12, // net:dconn
    DIAG_ID_NETWORK_CONNECTION_ATTEMPTS = 27, // net:connatt
    DIAG_ID_NETWORK_DISCONNECTION_REASON = 28, // net:dconnrsn
    DIAG_ID_NETWORK_IPV4_ADDRESS = 15, // net:ip:addr
    DIAG_ID_NETWORK_IPV4_GATEWAY = 16, // net.ip:gw
    DIAG_ID_NETWORK_FLAGS = 17, // net:flags
    DIAG_ID_NETWORK_COUNTRY_CODE = 18, // net:cntry
    DIAG_ID_NETWORK_RSSI = 19, // net:rssi
    DIAG_ID_NETWORK_SIGNAL_STRENGTH_VALUE = 37, // net:sigstrv
    DIAG_ID_NETWORK_SIGNAL_STRENGTH = 33, // net:sigstr
    DIAG_ID_NETWORK_SIGNAL_QUALITY = 34, // net:sigqual
    DIAG_ID_NETWORK_SIGNAL_QUALITY_VALUE = 35, // net:sigqualv
    DIAG_ID_NETWORK_ACCESS_TECNHOLOGY = 36, // net:at
    DIAG_ID_NETWORK_CELLULAR_CELL_GLOBAL_IDENTITY_MOBILE_COUNTRY_CODE = 40, // net:cell:cgi:mcc
    DIAG_ID_NETWORK_CELLULAR_CELL_GLOBAL_IDENTITY_MOBILE_NETWORK_CODE = 41, // net:cell:cgi:mnc
    DIAG_ID_NETWORK_CELLULAR_CELL_GLOBAL_IDENTITY_LOCATION_AREA_CODE = 42, // net:cell:cgi:lac
    DIAG_ID_NETWORK_CELLULAR_CELL_GLOBAL_IDENTITY_CELL_ID = 43, // net:cell:cgi:ci
    DIAG_ID_CLOUD_CONNECTION_STATUS = 10, // cloud:stat
    DIAG_ID_CLOUD_CONNECTION_ERROR_CODE = 13, // cloud:err
    DIAG_ID_CLOUD_DISCONNECTS = 14, // cloud:dconn
    DIAG_ID_CLOUD_CONNECTION_ATTEMPTS = 29, // cloud:connatt
    DIAG_ID_CLOUD_DISCONNECTION_REASON = 30, // cloud:dconnrsn
    DIAG_ID_CLOUD_REPEATED_MESSAGES = 21, // coap:resend
    DIAG_ID_CLOUD_UNACKNOWLEDGED_MESSAGES = 22, // coap:unack
    DIAG_ID_CLOUD_RATE_LIMITED_EVENTS = 20, // pub:throttle
    DIAG_ID_SYSTEM_TOTAL_RAM = 25, // sys:tram
    DIAG_ID_SYSTEM_USED_RAM = 26, // sys:uram
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

struct diag_source {
    uint16_t size; // Size of this structure
    uint16_t flags; // Source flags
    uint16_t id; // Source ID
    uint16_t type; // Data type
    const char* name; // Source name
    void* data; // User data
    diag_source_cmd_callback callback; // Source callback
};

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
