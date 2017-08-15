/**
 ******************************************************************************
 * @file    system_control.h
 * @author  Andrey Tolstoy
 * @version V1.0.0
 * @date    12-June-2016
 * @brief   Header for system_contro.cpp module
 ******************************************************************************
  Copyright (c) 2013-2016 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

#ifndef SYSTEM_CONTROL_H_
#define SYSTEM_CONTROL_H_

#include <stddef.h>

#include "usb_hal.h"

typedef enum DataFormat { // TODO: Move to appropriate header
  DATA_FORMAT_INVALID = 0,
  DATA_FORMAT_BINARY = 10, // Generic binary format
  DATA_FORMAT_TEXT = 20, // Generic text format
  DATA_FORMAT_JSON = 30
} DataFormat;

#ifdef USB_VENDOR_REQUEST_ENABLE

// Maximum supported size of USB request/reply data
#define USB_REQUEST_BUFFER_SIZE 512

#ifdef __cplusplus
extern "C" {
#endif

typedef enum USBRequestType {
  USB_REQUEST_INVALID = 0,
  USB_REQUEST_CUSTOM = 10, // Customizable request processed in application thread
  USB_REQUEST_DEVICE_ID = 20,
  USB_REQUEST_SYSTEM_VERSION = 30,
  USB_REQUEST_RESET = 40,
  USB_REQUEST_DFU_MODE = 50,
  USB_REQUEST_SAFE_MODE = 60,
  USB_REQUEST_LISTENING_MODE = 70,
  USB_REQUEST_LOG_CONFIG = 80,
  USB_REQUEST_MODULE_INFO = 90,
  USB_REQUEST_GET_DIAGNOSTIC = 100,
  USB_REQUEST_UPDATE_DIAGNOSTIC = 101
} USBRequestType;

typedef enum USBRequestResult {
  USB_REQUEST_RESULT_OK = 0,
  USB_REQUEST_RESULT_ERROR = 10
} USBRequestResult;

typedef struct USBRequest {
  size_t size; // Structure size
  int type; // Request type (as defined by USBRequestType enum)
  char* data; // Data buffer
  size_t request_size; // Request size
  size_t reply_size; // Reply size (initialized to 0)
  int format; // Data format (as defined by DataFormat enum)
  uint16_t value; // wValue field
} USBRequest;

// Callback invoked for USB requests that should be processed at application side
typedef bool(*usb_request_app_handler_type)(USBRequest* req, void* reserved);

// Sets application callback for USB requests
void system_set_usb_request_app_handler(usb_request_app_handler_type handler, void* reserved);

// Signals that processing of the USB request has finished
void system_set_usb_request_result(USBRequest* req, int result, void* reserved);

#ifdef __cplusplus
} // extern "C"

class SystemControlInterface {
public:
  SystemControlInterface();
  ~SystemControlInterface();

  static void setRequestResult(USBRequest* req, USBRequestResult result);

private:
  struct USBRequestData {
    USBRequest req; // Externally accessible part of the request data
    USBRequestResult result;
    bool active;
    volatile bool ready;

    USBRequestData();
    ~USBRequestData();
  };

  USBRequestData usbReq_;

  uint8_t handleVendorRequest(HAL_USB_SetupRequest* req);

  uint8_t enqueueRequest(HAL_USB_SetupRequest* req, DataFormat fmt = DATA_FORMAT_BINARY, bool use_isr = false);
  uint8_t fetchRequestResult(HAL_USB_SetupRequest* req);

  static void processSystemRequest(void* data); // Called by SystemThread
  static void processAppRequest(void* data); // Called by ApplicationThread

  static uint8_t vendorRequestCallback(HAL_USB_SetupRequest* req, void* data); // Called by HAL

  static bool setRequestResult(USBRequest* req, const char* data, size_t size);
};

#endif // __cplusplus

#endif // USB_VENDOR_REQUEST_ENABLE

#endif // SYSTEM_CONTROL_H_
