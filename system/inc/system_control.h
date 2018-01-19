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

#include "system_error.h"

#include <stdint.h>
#include <stddef.h>

// By default, the control requests functionality is available only if the platform supports USB
// vendor requests, since, at the moment, it's the only control interface supported by the system
#ifndef SYSTEM_CONTROL_ENABLED
#ifdef USB_VENDOR_REQUEST_ENABLE
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
    CTRL_REQUEST_SYSTEM_VERSION = 30,
    CTRL_REQUEST_RESET = 40,
    CTRL_REQUEST_FACTORY_RESET = 41,
    CTRL_REQUEST_DFU_MODE = 50,
    CTRL_REQUEST_SAFE_MODE = 60,
    CTRL_REQUEST_START_LISTENING = 70,
    CTRL_REQUEST_STOP_LISTENING = 71,
    CTRL_REQUEST_LOG_CONFIG = 80,
    CTRL_REQUEST_MODULE_INFO = 90,
    CTRL_REQUEST_DIAGNOSTIC_INFO = 100,
    CTRL_REQUEST_WIFI_SET_ANTENNA = 110,
    CTRL_REQUEST_WIFI_GET_ANTENNA = 111,
    CTRL_REQUEST_WIFI_SCAN = 112,
    CTRL_REQUEST_WIFI_SET_CREDENTIALS = 113,
    CTRL_REQUEST_WIFI_GET_CREDENTIALS = 114,
    CTRL_REQUEST_WIFI_CLEAR_CREDENTIALS = 115,
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
    CTRL_REQUEST_GET_SECTION_DATA_SIZE = 264
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
