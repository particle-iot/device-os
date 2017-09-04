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
    CTRL_REQUEST_DFU_MODE = 50,
    CTRL_REQUEST_SAFE_MODE = 60,
    CTRL_REQUEST_LISTENING_MODE = 70,
    CTRL_REQUEST_LOG_CONFIG = 80,
    CTRL_REQUEST_MODULE_INFO = 90
} ctrl_request_type;

// Control request data
typedef struct ctrl_request {
    size_t size; // Size of this structure
    char* request_data; // Request data
    size_t request_size; // Size of the request data
    char* reply_data; // Reply data
    size_t reply_size; // Size of the reply data
    uint16_t type; // Request type
} ctrl_request;

// Callback invoked for control requests that should be processed in the application thread
typedef void(*ctrl_request_handler_fn)(ctrl_request* req);

// Sets the application callback for control requests
int system_ctrl_set_app_request_handler(ctrl_request_handler_fn handler, void* reserved);

// Allocates a buffer for reply data
int system_ctrl_alloc_reply_data(ctrl_request* req, size_t size, void* reserved);

// Releases a buffer with reply data
void system_ctrl_free_reply_data(ctrl_request* req, void* reserved);

// Releases a buffer with request data
void system_ctrl_free_request_data(ctrl_request* req, void* reserved);

// Completes the processing of a request. The `result` argument specifies a result code as defined
// by the `system_error_t` enum
void system_ctrl_set_result(ctrl_request* req, int result, void* reserved);

#ifdef USB_VENDOR_REQUEST_ENABLE

// These functions are deprecated and exported only for compatibility
void system_set_usb_request_app_handler(void*, void*);
void system_set_usb_request_result(void*, int, void*);

#endif // defined(USB_VENDOR_REQUEST_ENABLE)

#ifdef __cplusplus
} // extern "C"
#endif
