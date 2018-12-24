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

#include "usb_hal.h"
#include "hal_platform.h"

#include "system_error.h"

#include <stdint.h>
#include <stddef.h>

#ifndef SYSTEM_CONTROL_ENABLED
#if defined(USB_VENDOR_REQUEST_ENABLE) || HAL_PLATFORM_BLE
#define SYSTEM_CONTROL_ENABLED 1
#else
#define SYSTEM_CONTROL_ENABLED 0
#endif
#endif // !defined(SYSTEM_CONTROL_ENABLED)

#ifdef __cplusplus
extern "C" {
#endif

// Control request types
typedef enum ctrl_request_type {
    CTRL_REQUEST_INVALID = 0,
    CTRL_REQUEST_APP_CUSTOM = 10,
    CTRL_REQUEST_DEVICE_ID = 20,
    CTRL_REQUEST_SERIAL_NUMBER = 21,
    CTRL_REQUEST_SYSTEM_VERSION = 30,
    CTRL_REQUEST_NCP_FIRMWARE_VERSION = 31,
    CTRL_REQUEST_GET_SYSTEM_CAPABILITIES = 32,
    CTRL_REQUEST_SET_FEATURE = 33,
    CTRL_REQUEST_GET_FEATURE = 34,
    CTRL_REQUEST_RESET = 40,
    CTRL_REQUEST_FACTORY_RESET = 41,
    CTRL_REQUEST_DFU_MODE = 50,
    CTRL_REQUEST_SAFE_MODE = 60,
    CTRL_REQUEST_START_LISTENING = 70,
    CTRL_REQUEST_STOP_LISTENING = 71,
    CTRL_REQUEST_GET_DEVICE_MODE = 72,
    CTRL_REQUEST_SET_DEVICE_SETUP_DONE = 73,
    CTRL_REQUEST_IS_DEVICE_SETUP_DONE = 74,
    CTRL_REQUEST_SET_STARTUP_MODE = 75,
    CTRL_REQUEST_LOG_CONFIG = 80,
    CTRL_REQUEST_GET_MODULE_INFO = 90,
    CTRL_REQUEST_DIAGNOSTIC_INFO = 100,
    CTRL_REQUEST_WIFI_SET_ANTENNA = 110,
    CTRL_REQUEST_WIFI_GET_ANTENNA = 111,
    CTRL_REQUEST_WIFI_SCAN = 112, // Deprecated
    CTRL_REQUEST_WIFI_SET_CREDENTIALS = 113, // Deprecated
    CTRL_REQUEST_WIFI_GET_CREDENTIALS = 114, // Deprecated
    CTRL_REQUEST_WIFI_CLEAR_CREDENTIALS = 115, // Deprecated
    CTRL_REQUEST_NETWORK_SET_CONFIGURATION = 120,
    CTRL_REQUEST_NETWORK_GET_CONFIGURATION = 121,
    CTRL_REQUEST_NETWORK_GET_STATUS = 122,
    CTRL_REQUEST_SET_CLAIM_CODE = 200,
    CTRL_REQUEST_IS_CLAIMED = 201,
    CTRL_REQUEST_SET_SECURITY_KEY = 210,
    CTRL_REQUEST_GET_SECURITY_KEY = 211,
    CTRL_REQUEST_SET_SERVER_ADDRESS = 220,
    CTRL_REQUEST_GET_SERVER_ADDRESS = 221,
    CTRL_REQUEST_SET_SERVER_PROTOCOL = 222,
    CTRL_REQUEST_GET_SERVER_PROTOCOL = 223,
    CTRL_REQUEST_START_NYAN_SIGNAL = 230,
    CTRL_REQUEST_STOP_NYAN_SIGNAL = 231,
    CTRL_REQUEST_SET_SOFTAP_SSID = 240,
    CTRL_REQUEST_START_FIRMWARE_UPDATE = 250,
    CTRL_REQUEST_FINISH_FIRMWARE_UPDATE = 251,
    CTRL_REQUEST_CANCEL_FIRMWARE_UPDATE = 252,
    CTRL_REQUEST_FIRMWARE_UPDATE_DATA = 253,
    CTRL_REQUEST_DESCRIBE_STORAGE = 260,
    CTRL_REQUEST_READ_SECTION_DATA = 261,
    CTRL_REQUEST_WRITE_SECTION_DATA = 262,
    CTRL_REQUEST_CLEAR_SECTION_DATA = 263,
    CTRL_REQUEST_GET_SECTION_DATA_SIZE = 264,
    // Cloud connectivity
    CTRL_REQUEST_CLOUD_GET_CONNECTION_STATUS = 300,
    CTRL_REQUEST_CLOUD_CONNECT = 301,
    CTRL_REQUEST_CLOUD_DISCONNECT = 302,
    // Network management
    CTRL_REQUEST_NETWORK_GET_INTERFACE_LIST = 400,
    CTRL_REQUEST_NETWORK_GET_INTERFACE = 401,
    // WiFi network management
    CTRL_REQUEST_WIFI_JOIN_NEW_NETWORK = 500,
    CTRL_REQUEST_WIFI_JOIN_KNOWN_NETWORK = 501,
    CTRL_REQUEST_WIFI_GET_KNOWN_NETWORKS = 502,
    CTRL_REQUEST_WIFI_REMOVE_KNOWN_NETWORK = 503,
    CTRL_REQUEST_WIFI_CLEAR_KNOWN_NETWORKS = 504,
    CTRL_REQUEST_WIFI_GET_CURRENT_NETWORK = 505,
    CTRL_REQUEST_WIFI_SCAN_NETWORKS = 506,
    // Cellular network management
    CTRL_REQUEST_CELLULAR_SET_ACCESS_POINT = 550,
    CTRL_REQUEST_CELLULAR_GET_ACCESS_POINT = 551,
    CTRL_REQUEST_CELLULAR_SET_ACTIVE_SIM = 552,
    CTRL_REQUEST_CELLULAR_GET_ACTIVE_SIM = 553,
    CTRL_REQUEST_CELLULAR_GET_ICCID = 554,
    // Mesh network management
    CTRL_REQUEST_MESH_AUTH = 1001,
    CTRL_REQUEST_MESH_CREATE_NETWORK = 1002,
    CTRL_REQUEST_MESH_START_COMMISSIONER = 1003,
    CTRL_REQUEST_MESH_STOP_COMMISSIONER = 1004,
    CTRL_REQUEST_MESH_PREPARE_JOINER = 1005,
    CTRL_REQUEST_MESH_ADD_JOINER = 1006,
    CTRL_REQUEST_MESH_REMOVE_JOINER = 1007,
    CTRL_REQUEST_MESH_JOIN_NETWORK = 1008,
    CTRL_REQUEST_MESH_LEAVE_NETWORK = 1009,
    CTRL_REQUEST_MESH_GET_NETWORK_INFO = 1010,
    CTRL_REQUEST_MESH_SCAN_NETWORKS = 1011,
    CTRL_REQUEST_MESH_GET_NETWORK_DIAGNOSTICS = 1012,
    CTRL_REQUEST_MESH_TEST = 1111 // FIXME
} ctrl_request_type;

// Control request data
typedef struct ctrl_request {
    uint16_t size; // Size of this structure
    uint16_t type; // Request type
    char* request_data; // Request data
    size_t request_size; // Size of the request data
    char* reply_data; // Reply data
    size_t reply_size; // Size of the reply data
    void* channel; // Request channel (used internally)
} ctrl_request;

// Callback invoked for control requests that should be processed in the application thread
typedef void(*ctrl_request_handler_fn)(ctrl_request* req);

// Completion handler callback for a control request. The `result` argument will be set to 0 if
// the sender of a request has received the reply successfully
typedef void(*ctrl_completion_handler_fn)(int result, void* data);

// Sets the application callback for control requests
int system_ctrl_set_app_request_handler(ctrl_request_handler_fn handler, void* reserved);

// Allocates or frees the reply data buffer. This function acts as realloc(), i.e. it changes the
// size of an already allocated buffer, or frees it if `size` is set to 0
int system_ctrl_alloc_reply_data(ctrl_request* req, size_t size, void* reserved);

// Frees the request data buffer
void system_ctrl_free_request_data(ctrl_request* req, void* reserved);

// Completes the processing of a request. The `result` argument specifies a result code as defined
// by the `system_error_t` enum. The function also takes an optional callback that will be invoked
// once the sender of the request has received the reply successfully, or an error has occured
// while sending the reply
void system_ctrl_set_result(ctrl_request* req, int result, ctrl_completion_handler_fn handler, void* data, void* reserved);

#ifdef USB_VENDOR_REQUEST_ENABLE

// These functions are deprecated and exported only for compatibility
void system_set_usb_request_app_handler(void*, void*);
void system_set_usb_request_result(void*, int, void*);

#endif // defined(USB_VENDOR_REQUEST_ENABLE)

#ifdef __cplusplus
} // extern "C"
#endif
