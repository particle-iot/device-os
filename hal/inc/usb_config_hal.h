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

#ifndef USB_CONFIG_HAL_H
#define	USB_CONFIG_HAL_H

// NOTE: this is a compatibility header, the code using the macros defined here
// should be refactored to use hal_platform configuration options.
// New code should not rely on these macros at all.

#include "hal_platform.h"

#if HAL_PLATFORM_USB
#define SPARK_USB_SERIAL
#endif // HAL_PLATFORM_USB

#if HAL_PLATFORM_USB_CDC
#define USB_CDC_ENABLE
#endif // HAL_PLATFORM_USB_CDC

#if HAL_PLATFORM_USB_HID
#define SPARK_USB_MOUSE
#define SPARK_USB_KEYBOARD
#define USB_HID_ENABLE
#endif // USB_HID_ENABLE

#if HAL_PLATFORM_USB_CONTROL_INTERFACE || defined(UNIT_TEST)
#define USB_VENDOR_REQUEST_ENABLE
#endif // HAL_PLATFORM_USB_CONTROL_INTERFACE || defined(UNIT_TEST)

#endif	/* USB_CONFIG_HAL_H */

