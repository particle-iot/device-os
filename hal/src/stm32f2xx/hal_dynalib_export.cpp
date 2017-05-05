/**
 ******************************************************************************
 * @file    hal_dynalib_export.c
 * @author  Matthew McGowan
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

#define DYNALIB_EXPORT
#include "hal_dynalib.h"
#include "hal_dynalib_core.h"
#include "hal_dynalib_gpio.h"
#include "hal_dynalib_i2c.h"
#include "hal_dynalib_ota.h"
#include "hal_dynalib_peripherals.h"
#include "hal_dynalib_socket.h"
#include "hal_dynalib_spi.h"
#include "hal_dynalib_usart.h"
#include "hal_dynalib_wlan.h"
#include "hal_dynalib_concurrent.h"
#include "hal_dynalib_cellular.h"
#include "hal_dynalib_can.h"
#include "hal_dynalib_rgbled.h"
#include "hal_dynalib_dct.h"

#ifndef HAL_USB_EXCLUDE
#include "hal_dynalib_usb.h"
#endif

#ifndef HAL_BOOTLOADER_EXCLUDE
#include "hal_dynalib_bootloader.h"
#endif
