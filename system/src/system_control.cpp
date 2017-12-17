/**
 ******************************************************************************
 * @file    system_control.cpp
 * @author  Andrey Tolstoy
 * @version V1.0.0
 * @date    12-June-2016
 * @brief
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

#include "system_control.h"

#include "deviceid_hal.h"

#include "system_task.h"
#include "system_threading.h"

#include "spark_wiring_system.h"
#include "system_network_internal.h"
#include "bytes2hexbuf.h"
#include "system_update.h"
#include "tracer_service.h"

#ifdef USB_VENDOR_REQUEST_ENABLE

namespace {

SystemControlInterface systemControl;

usb_request_app_handler_type usbReqAppHandler = nullptr;

} // namespace

// ::
void system_set_usb_request_app_handler(usb_request_app_handler_type handler, void* reserved) {
  usbReqAppHandler = handler;
}

void system_set_usb_request_result(USBRequest* req, int result, void* reserved) {
  SystemControlInterface::setRequestResult(req, (USBRequestResult)result);
}

// SystemControlInterface::USBRequestData
SystemControlInterface::USBRequestData::USBRequestData() :
    result(USB_REQUEST_RESULT_ERROR),
    active(false),
    ready(false) {
  req.size = sizeof(USBRequest);
  req.type = USB_REQUEST_INVALID;
  req.data = (char*)malloc(USB_REQUEST_BUFFER_SIZE);
  req.request_size = 0;
  req.reply_size = 0;
  req.format = DATA_FORMAT_INVALID;
}

SystemControlInterface::USBRequestData::~USBRequestData() {
  free(req.data);
}

// SystemControlInterface
SystemControlInterface::SystemControlInterface() {
  HAL_USB_Set_Vendor_Request_Callback(vendorRequestCallback, static_cast<void*>(this));
}

SystemControlInterface::~SystemControlInterface() {
  HAL_USB_Set_Vendor_Request_Callback(nullptr, nullptr);
}

uint8_t SystemControlInterface::handleVendorRequest(HAL_USB_SetupRequest* req) {
  /*
   * This callback should process vendor-specific SETUP requests from the host.
   * NOTE: This callback is called from an ISR.
   *
   * Each request contains the following fields:
   * - bmRequestType - request type bit mask. Since only vendor-specific device requests are forwarded
   *                   to this callback, the only bit that should be of intereset is
   *                   "Data Phase Transfer Direction", easily accessed as req->bmRequestTypeDirection:
   *                   0 - Host to Device
   *                   1 - Device to Host
   * - bRequest - 1 byte, request identifier. The only reserved request that will not be forwarded to this callback
   *              is 0xee, which is used for Microsoft-specific vendor requests.
   * - wIndex - 2 byte index, any value between 0x0000 and 0xffff.
   * - wValue - 2 byte value, any value between 0x0000 and 0xffff.
   * - wLength - each request might have an optional data stage of up to 0xffff (65535) bytes.
   *             Host -> Device requests contain data sent by the host to the device.
   *             Device -> Host requests request the device to send up to wLength bytes of data.
   *
   * This callback should return 0 if the request has been correctly handled or 1 otherwise.
   *
   * When handling Device->Host requests with data stage, this callback should fill req->data buffer with
   * up to wLength bytes of data if req->data != NULL (which should be the case when wLength <= 64)
   * or set req->data to point to some buffer which contains up to wLength bytes of data.
   * wLength may be safely modified to notify that there is less data in the buffer than requested by the host.
   *
   * [1] Host -> Device requests with data stage containing up to 64 bytes can be handled by
   * an internal buffer in HAL. For requests larger than 64 bytes, the vendor request callback
   * needs to provide a buffer of an appropriate size:
   *
   * req->data = buffer; // sizeof(buffer) >= req->wLength
   * return 0;
   *
   * The callback will be called again once the data stage completes
   * It will contain the same bmRequest, bRequest, wIndex, wValue and wLength fields.
   * req->data should contain wLength bytes received from the host.
   */

  /*
   * We are handling only bRequest = 0x50 ('P') requests.
   * The request type itself (enum USBRequestType) should be in wIndex field.
   */
  if (req->bRequest != 0x50)
    return 1;

  if (req->bmRequestTypeDirection == 0) {
    // Host -> Device
    switch (req->wIndex) {
      // Process these requests on system thread
      case USB_REQUEST_RESET:
      case USB_REQUEST_DFU_MODE:
      case USB_REQUEST_LISTENING_MODE:
      case USB_REQUEST_SAFE_MODE:
        network.connect_cancel(true);
      case USB_REQUEST_MODULE_INFO: {
        return enqueueRequest(req);
        break;
      }

      case USB_REQUEST_LOG_CONFIG: {
        return enqueueRequest(req, DATA_FORMAT_JSON);
      }

      case USB_REQUEST_GET_TRACE: {
        return enqueueRequest(req, DATA_FORMAT_BINARY, true);
      }
      case USB_REQUEST_CUSTOM: {
        return enqueueRequest(req);
      }

      case USB_REQUEST_UPDATE_TRACE: {
        TRACER_UPDATE();
        break;
      }

      default: {
        // Unknown request
        return 1;
      }
    }
  } else {
    // Device -> Host
    switch (req->wIndex) {
      case USB_REQUEST_DEVICE_ID: {
        if (req->wLength == 0 || req->data == NULL) {
          // No data stage or requested > 64 bytes
          return 1;
        }
        if (req->wValue == 0x0001) {
          // Return as buffer
          if (req->wLength < 12)
            return 1;

          HAL_device_ID(req->data, req->wLength);
          req->wLength = 12;
        } else {
          // Return as string
          uint8_t deviceId[16];
          int sz = HAL_device_ID(deviceId, sizeof(deviceId));
          if (req->wLength < (sz * 2 + 1))
            return 1;
          bytes2hexbuf(deviceId, sz, (char*)req->data);
          req->data[sz * 2] = '\0';
          req->wLength = sz * 2 + 1;
        }
        break;
      }

      case USB_REQUEST_SYSTEM_VERSION: {
        if (req->wLength == 0 || req->data == NULL) {
          // No data stage or requested > 64 bytes
          return 1;
        }

        strncpy((char*)req->data, __XSTRING(SYSTEM_VERSION_STRING), req->wLength);
        req->wLength = sizeof(__XSTRING(SYSTEM_VERSION_STRING)) + 1;
        break;
      }

      case USB_REQUEST_MODULE_INFO:
      case USB_REQUEST_LOG_CONFIG:
      case USB_REQUEST_CUSTOM:
      case USB_REQUEST_GET_TRACE: {
        return fetchRequestResult(req);
      }

      default: {
        // Unknown request
        return 1;
      }
    }
  }

  return 0;
}

uint8_t SystemControlInterface::enqueueRequest(HAL_USB_SetupRequest* req, DataFormat fmt, bool use_isr) {
  SPARK_ASSERT(req->bmRequestTypeDirection == 0); // Host to device
  if (usbReq_.active && !usbReq_.ready) {
    return 1; // // There is an active request already
  }
  if (req->wLength > 0) {
    if (!usbReq_.req.data) {
      return 1; // Initialization error
    }
    if (req->wLength > USB_REQUEST_BUFFER_SIZE) {
      return 1; // Too large request
    }
    if (req->wLength <= 64) {
      if (!req->data) {
        return 1; // Invalid request info
      }
      memcpy(usbReq_.req.data, req->data, req->wLength);
    } else if (!req->data) {
      req->data = (uint8_t*)usbReq_.req.data; // Provide buffer for request data
      return 0; // OK
    }
  }
  if (!use_isr) {
    // Schedule request for processing in the system thread's context
    if (!SystemISRTaskQueue.enqueue(processSystemRequest, &usbReq_.req)) {
      return 1;
    }
  }
  usbReq_.req.type = (USBRequestType)req->wIndex;
  usbReq_.req.value = req->wValue;
  usbReq_.req.request_size = req->wLength;
  usbReq_.req.reply_size = 0;
  usbReq_.req.format = fmt;
  usbReq_.ready = false;
  usbReq_.active = true;
  if (use_isr) {
    processSystemRequest(&usbReq_.req);
  }
  return 0;
}

uint8_t SystemControlInterface::fetchRequestResult(HAL_USB_SetupRequest* req) {
  SPARK_ASSERT(req->bmRequestTypeDirection == 1); // Device to host
  if (!usbReq_.ready) {
    return 1; // No reply data available
  }
  if (usbReq_.req.type != (USBRequestType)req->wIndex) {
    return 1; // Unexpected request type
  }
  if (usbReq_.result != USB_REQUEST_RESULT_OK) {
    return 1; // Request has failed (TODO: Reply with a result code?)
  }
  if (req->wLength > 0) {
    if (!usbReq_.req.data) {
      return 1; // Initialization error
    }
    if (req->wLength < usbReq_.req.reply_size) {
      return 1; // Too large reply
    }
    if (req->wLength <= 64) {
      if (!req->data) {
        return 1; // Invalid request info
      }
      memcpy(req->data, usbReq_.req.data, usbReq_.req.reply_size);
    } else {
      req->data = (uint8_t*)usbReq_.req.data; // Provide buffer with reply data
    }
    req->wLength = usbReq_.req.reply_size;
  }
  usbReq_.active = false;
  usbReq_.ready = false;
  return 0;
}

void SystemControlInterface::setRequestResult(USBRequest* req, USBRequestResult result) {
  USBRequestData *r = (USBRequestData*)req;
  r->result = result;
  r->ready = true;
}

void SystemControlInterface::processSystemRequest(void* data) {
  USBRequest* req = static_cast<USBRequest*>(data);
  USBRequestResult result = USB_REQUEST_RESULT_OK;
  switch (req->type) {
  // Handle requests that should be processed by the system modules here
  case USB_REQUEST_RESET: {
    System.reset(req->value);
    break;
  }

  case USB_REQUEST_DFU_MODE: {
    System.dfu(false);
    break;
  }

  case USB_REQUEST_LISTENING_MODE: {
    network.listen(req->value);
    break;
  }

  case USB_REQUEST_SAFE_MODE: {
    System.enterSafeMode();
    break;
  }

  case USB_REQUEST_MODULE_INFO: {
    BufferAppender appender((uint8_t*)req->data, USB_REQUEST_BUFFER_SIZE);
    system_module_info(append_instance, &appender);
    req->reply_size = appender.size();
    break;
  }

  case USB_REQUEST_GET_TRACE: {
    if (req->value) {
      req->reply_size = tracer_dump_current(req->data, USB_REQUEST_BUFFER_SIZE);
    } else {
      req->reply_size = tracer_dump_saved(req->data, USB_REQUEST_BUFFER_SIZE);
    }
    if (req->reply_size > USB_REQUEST_BUFFER_SIZE) {
      req->reply_size = USB_REQUEST_BUFFER_SIZE;
    }
    break;
  }

  default:
    if (usbReqAppHandler) {
      processAppRequest(data); // Forward request to the application thread
      return;
    } else {
      result = USB_REQUEST_RESULT_ERROR;
    }
  }

  setRequestResult(req, result);
}

void SystemControlInterface::processAppRequest(void* data) {
  // FIXME: Request leak may occur if underlying asynchronous event cannot be queued
  APPLICATION_THREAD_CONTEXT_ASYNC(processAppRequest(data));
  USBRequest* req = static_cast<USBRequest*>(data);
  SPARK_ASSERT(usbReqAppHandler); // Checked in processSystemRequest()
  if (!usbReqAppHandler(req, nullptr)) {
    setRequestResult(req, USB_REQUEST_RESULT_ERROR);
  }
}

uint8_t SystemControlInterface::vendorRequestCallback(HAL_USB_SetupRequest* req, void* data) {
  SystemControlInterface* self = static_cast<SystemControlInterface*>(data);
  if (self) {
    return self->handleVendorRequest(req);
  }
  return 1;
}

#endif // USB_VENDOR_REQUEST_ENABLE
