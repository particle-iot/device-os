/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "ble_hal_impl.h"

#include "system_error.h"

#include <stdint.h>
#include <stddef.h>

// TODO: This API is not ready for general purpose usage

// API version
#define BLE_API_VERSION 1

// Default maximum size of an ATT packet in bytes (ATT_MTU)
#define BLE_MIN_ATT_MTU_SIZE 23

// Size of the ATT opcode field in bytes
#define BLE_ATT_OPCODE_SIZE 1

// Size of the ATT handle field in bytes
#define BLE_ATT_HANDLE_SIZE 2

// Minimum and maximum number of bytes that can be sent in a single write command, read response,
// notification or indication packet
#define BLE_MIN_ATTR_VALUE_PACKET_SIZE (BLE_MIN_ATT_MTU_SIZE - BLE_ATT_OPCODE_SIZE - BLE_ATT_HANDLE_SIZE)
#define BLE_MAX_ATTR_VALUE_PACKET_SIZE (BLE_MAX_ATT_MTU_SIZE - BLE_ATT_OPCODE_SIZE - BLE_ATT_HANDLE_SIZE)

#ifdef __cplusplus
extern "C" {
#endif

// BLE error codes
typedef enum ble_error {
    BLE_ERROR_UNKNOWN = SYSTEM_ERROR_UNKNOWN,
    BLE_ERROR_INVALID_PARAM = SYSTEM_ERROR_INVALID_ARGUMENT,
    BLE_ERROR_INVALID_STATE = SYSTEM_ERROR_INVALID_STATE,
    BLE_ERROR_NO_MEMORY = SYSTEM_ERROR_NO_MEMORY,
    BLE_ERROR_BUSY = SYSTEM_ERROR_BUSY
} ble_error;

// BLE events
typedef enum ble_event {
    BLE_EVENT_ADVERT_STARTED = 1,
    BLE_EVENT_ADVERT_STOPPED = 2,
    BLE_EVENT_CONNECTED = 3,
    BLE_EVENT_DISCONNECTED = 4,
    BLE_EVENT_CONN_PARAM_CHANGED = 5,
    BLE_EVENT_CHAR_PARAM_CHANGED = 6,
    BLE_EVENT_DATA_SENT = 7,
    BLE_EVENT_DATA_RECEIVED = 8
} ble_event;

// Profile flags
typedef enum ble_profile_flag {
    BLE_PROFILE_FLAG_ENABLE_PAIRING = 0x01
} ble_profile_flag;

// Characteristic flags
typedef enum ble_char_flag {
    BLE_CHAR_FLAG_REQUIRE_PAIRING = 0x01
} ble_char_flag;

// Flags for ble_set_char_value()
typedef enum ble_set_char_value_flag {
    BLE_SET_CHAR_VALUE_FLAG_NOTIFY = 0x01
} ble_set_char_value_flag;

// Characteristic types
// TODO: Provide an API for custom characteristics and permissions
typedef enum ble_char_type {
    BLE_CHAR_TYPE_TX = 1,
    BLE_CHAR_TYPE_RX = 2,
    BLE_CHAR_TYPE_VAL = 3
} ble_char_type;

// UUID
typedef struct ble_uuid {
    uint8_t type; // UUID type obtained via ble_add_base_uuid()
    uint8_t reserved;
    uint16_t uuid;
} ble_uuid;

// Characteristic
typedef struct ble_char {
    ble_uuid uuid;
    uint16_t type; // See `ble_char_type` enum
    uint16_t handle; // TODO: Use typedefs for all handle types defined by the HAL
    const char* data; // Initial value
    uint16_t size;
    uint16_t flags;
} ble_char;

// Service
typedef struct ble_service {
    ble_uuid uuid;
    ble_char* chars;
    uint16_t char_count;
} ble_service;

// Manufacturer-specific data
typedef struct ble_manuf_data {
    uint16_t company_id;
    uint16_t size;
    const char* data;
} ble_manuf_data;

// BLE_EVENT_CONNECTED event data
typedef struct ble_connected_event_data {
    uint16_t conn_handle;
} ble_connected_event_data;

// BLE_EVENT_DISCONNECTED event data
typedef struct ble_disconnected_event_data {
    uint16_t conn_handle;
} ble_disconnected_event_data;

// BLE_EVENT_CONN_PARAM_CHANGED event data
typedef struct ble_conn_param_changed_event_data {
    uint16_t conn_handle;
} ble_conn_param_changed_event_data;

// BLE_EVENT_CHAR_PARAM_CHANGED event data
typedef struct ble_char_param_changed_event_data {
    uint16_t conn_handle;
    uint16_t char_handle;
} ble_char_param_changed_event_data;

// BLE_EVENT_DATA_SENT event data
typedef struct ble_data_sent_event_data {
    uint16_t conn_handle;
} ble_data_sent_event_data;

// BLE_EVENT_DATA_RECEIVED event data
typedef struct ble_data_received_event_data {
    uint16_t conn_handle;
    uint16_t char_handle;
    const char* data;
    uint16_t size;
} ble_data_received_event_data;

// Event handler callback
typedef void(*ble_event_callback)(int event, const void* event_data, void* user_data);

// Profile
typedef struct ble_profile {
    uint16_t version; // API version
    uint16_t reserved;
    uint16_t flags;
    uint16_t service_count;
    ble_service* services;
    const char* device_name;
    const ble_manuf_data* manuf_data;
    ble_event_callback callback;
    void* user_data;
} ble_profile;

// Connection parameters
typedef struct ble_conn_param {
    uint16_t version; // API version
    uint16_t att_mtu_size; // Maximum size of an ATT packet (ATT_MTU)
} ble_conn_param;

// Characteristic parameters
typedef struct ble_char_param {
    uint16_t version; // API version
    bool notif_enabled;
} ble_char_param;

int ble_init(void* reserved);

int ble_add_base_uuid(const char* uuid, uint8_t* uuid_type, void* reserved);

int ble_init_profile(ble_profile* profile, void* reserved);

int ble_start_advert(void* reserved);
void ble_stop_advert(void* reserved);

int ble_get_char_param(uint16_t conn_handle, uint16_t char_handle, ble_char_param* param, void* reserved);
int ble_set_char_value(uint16_t conn_handle, uint16_t char_handle, const char* data, uint16_t size, unsigned flags, void* reserved);

int ble_get_conn_param(uint16_t conn_handle, ble_conn_param* param, void* reserved);
void ble_disconnect(uint16_t conn_handle, void* reserved);

#ifdef __cplusplus
} // extern "C"
#endif
