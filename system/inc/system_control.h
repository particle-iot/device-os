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

#include "usb_hal.h"

#ifdef USB_VENDOR_REQUEST_ENABLE

class SystemControlInterface {
public:
  SystemControlInterface();
  ~SystemControlInterface();
  uint8_t handleVendorRequest(HAL_USB_SetupRequest* req);

private:
  static uint8_t vendorRequestCallback(HAL_USB_SetupRequest* req, void* ptr);
};

#endif // USB_VENDOR_REQUEST_ENABLE

#endif // SYSTEM_CONTROL_H_