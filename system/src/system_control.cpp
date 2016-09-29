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
#include "spark_wiring.h"

#ifdef USB_VENDOR_REQUEST_ENABLE

static SystemControlInterface controlInterface;

enum ControlRequest {
  REQUEST_GET_DEVICE_ID               = 0x0000,
  REQUEST_GET_SYSTEM_VERSION          = 0x0001,
  REQUEST_RESET                       = 0x0002,
  // Perhaps could be combined into a signle request and a mode could be supplied in wValue?
  REQUEST_ENTER_DFU_MODE              = 0x0003,
  REQUEST_ENTER_LISTENING_MODE        = 0x0004
  //
};

SystemControlInterface::SystemControlInterface() {
  HAL_USB_Set_Vendor_Request_Callback(&SystemControlInterface::vendorRequestCallback, static_cast<void*>(this));
}

SystemControlInterface::~SystemControlInterface() {
  HAL_USB_Set_Vendor_Request_Callback(nullptr, nullptr);
}

uint8_t SystemControlInterface::vendorRequestCallback(HAL_USB_SetupRequest* req, void* ptr) {
  SystemControlInterface* self = static_cast<SystemControlInterface*>(ptr);
  if (self)
    return self->handleVendorRequest(req);

  return 1;
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
   * The request type itself (enum ControlRequest) should be in wIndex field.
   */
  if (req->bRequest != 0x50)
    return 1;

  if (req->bmRequestTypeDirection == 0) {
    // Host -> Device

    if (req->wLength && req->data == NULL) {
      /*
       * Currently not handled.
       * See [1]
       */
      return 1;
    }

    switch (req->wIndex) {
      case REQUEST_RESET: {
        // FIXME: We probably shouldn't reset from an ISR.
        // The host will probably get an error that control request has timed out since we
        // didn't respond to it.
        System.reset(req->wValue);
        break;
      }

      case REQUEST_ENTER_DFU_MODE: {
        // FIXME: We probably shouldn't enter DFU mode from an ISR.
        // The host will probably get an error that control request has timed out since we
        // didn't respond to it.
        System.dfu(false);
        break;
      }

      case REQUEST_ENTER_LISTENING_MODE: {
        // FIXME: We probably shouldn't enter listening mode from an ISR.
        // The host will probably get an error that control request has timed out since we
        // didn't respond to it.
        system_set_flag(SYSTEM_FLAG_STARTUP_SAFE_LISTEN_MODE, 1, nullptr);
        System.enterSafeMode();
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
      case REQUEST_GET_DEVICE_ID: {
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
          String id = System.deviceID();
          if (req->wLength < (id.length() + 1))
            return 1;
          strncpy((char*)req->data, id.c_str(), req->wLength);
          req->wLength = id.length() + 1;
        }
        break;
      }

      case REQUEST_GET_SYSTEM_VERSION: {
        if (req->wLength == 0 || req->data == NULL) {
          // No data stage or requested > 64 bytes
          return 1;
        }

        strncpy((char*)req->data, __XSTRING(SYSTEM_VERSION_STRING), req->wLength);
        req->wLength = sizeof(__XSTRING(SYSTEM_VERSION_STRING)) + 1;
        break;
      }

      default: {
        // Unknown request
        return 1;
      }
    }
  }

  return 0;
}

#endif // USB_VENDOR_REQUEST_ENABLE
