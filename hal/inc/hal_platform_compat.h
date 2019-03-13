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

#ifndef HAL_PLATFORM_COMPAT_H
#define HAL_PLATFORM_COMPAT_H

#include "platforms.h"

#if PLATFORM_ID<9
    #define HAL_PLATFORM_WIFI 1
#endif

#if PLATFORM_ID==10
#define HAL_PLATFORM_CELLULAR 1
#define PANIC_BUT_KEEP_CALM 1
#endif

#if PLATFORM_ID==10 || PLATFORM_ID==3
    #define HAL_PLATFORM_CLOUD_UDP 1
        #define HAL_PLATFORM_CLOUD_TCP 1
#else
    #define HAL_PLATFORM_CLOUD_TCP 1
#endif

#ifndef HAL_PLATFORM_WIFI
#define HAL_PLATFORM_WIFI 0
#endif

#ifndef HAL_PLATFORM_CELLULAR
#define HAL_PLATFORM_CELLULAR 0
#endif

#ifndef HAL_PLATFORM_CLOUD_UDP
#define HAL_PLATFORM_CLOUD_UDP 0
#endif

#ifndef HAL_PLATFORM_CLOUD_TCP
#define HAL_PLATFORM_CLOUD_TCP 0
#endif

#if PLATFORM_ID == 6 || PLATFORM_ID == 8 || PLATFORM_ID == 10
#define HAL_PLATFORM_DCT 1
#define HAL_PLATFORM_RNG 1
#endif

#define HAL_PLATFORM_NCP 	(0)
#define HAL_PLATFORM_NCP_AT (0)

#define HAL_PLATFORM_DCT_NO_DEPRECATED (0)

#define HAL_PLATFORM_USB_CDC (1)

#if PLATFORM_ID >= PLATFORM_PHOTON_PRODUCTION && PLATFORM_ID != PLATFORM_NEWHAL
#define HAL_PLATFORM_USB_HID (1)
#define HAL_PLATFORM_USB_COMPOSITE (1)
#define HAL_PLATFORM_USB_CONTROL_INTERFACE (1)
// system_set_usb_request_app_handler() and system_set_usb_request_result() should be present
// in system dynalib
#define HAL_PLATFORM_KEEP_DEPRECATED_APP_USB_REQUEST_HANDLERS (1)
#endif // PLATFORM_ID >= PLATFORM_PHOTON_PRODUCTION

#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
#define HAL_PLATFORM_POWER_MANAGEMENT (1)
#define HAL_PLATFORM_PMIC_BQ24195 (1)
#define HAL_PLATFORM_PMIC_BQ24195_I2C (HAL_I2C_INTERFACE3)
#define HAL_PLATFORM_PMIC_BQ24195_FAULT_COUNT_THRESHOLD (5)
#define HAL_PLATFORM_FUELGAUGE_MAX17043 (1)
#define HAL_PLATFORM_FUELGAUGE_MAX17043_I2C (HAL_I2C_INTERFACE3)
#endif // PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION

#if HAL_PLATFORM_WIFI
#define HAL_PLATFORM_NETWORK_MULTICAST (1)
#endif // HAL_PLATFORM_WIFI

#endif /* HAL_PLATFORM_COMPAT_H */
