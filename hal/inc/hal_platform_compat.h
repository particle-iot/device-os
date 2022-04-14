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

#ifndef HAL_PLATFORM_USART_9BIT_SUPPORTED
#define HAL_PLATFORM_USART_9BIT_SUPPORTED (1)
#endif // HAL_PLATFORM_USART_9BIT_SUPPORTED

#if PLATFORM_ID <= PLATFORM_P1 || PLATFORM_ID == PLATFORM_NEWHAL
    #define HAL_PLATFORM_WIFI 1
    #define HAL_PLATFORM_WIFI_COMPAT 1
#endif

#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
    #define HAL_PLATFORM_CELLULAR 1
    #define HAL_PLATFORM_CELLULAR_SERIAL (HAL_USART_SERIAL3)
    #define PANIC_BUT_KEEP_CALM 1
    #define HAL_PLATFORM_SETUP_BUTTON_UX 1
    #define HAL_PLATFORM_MAY_LEAK_SOCKETS (1)
    #define HAL_PLATFORM_INTERNAL_LOW_SPEED_CLOCK (1)
#endif

#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION || PLATFORM_ID == PLATFORM_GCC
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

#if PLATFORM_ID == PLATFORM_PHOTON_PRODUCTION || PLATFORM_ID == PLATFORM_P1 || PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
#define HAL_PLATFORM_STM32F2XX 1
#define HAL_PLATFORM_DCT 1
#define HAL_PLATFORM_RNG 1
#define HAL_PLATFORM_NEWLIB (1)
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

#define HAL_PLATFORM_BACKUP_RAM (1)

#endif // PLATFORM_ID >= PLATFORM_PHOTON_PRODUCTION && PLATFORM_ID != PLATFORM_NEWHAL

#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
#define HAL_PLATFORM_POWER_MANAGEMENT (1)
#define HAL_PLATFORM_PMIC_BQ24195 (1)
#define HAL_PLATFORM_PMIC_BQ24195_I2C (HAL_I2C_INTERFACE3)
#define HAL_PLATFORM_PMIC_BQ24195_FAULT_COUNT_THRESHOLD (5)
#define HAL_PLATFORM_FUELGAUGE_MAX17043 (1)
#define HAL_PLATFORM_FUELGAUGE_MAX17043_I2C (HAL_I2C_INTERFACE3)
#define HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME (9*60*1000)
#else
#define HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME (1*60*1000)
#endif // PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION

#if HAL_PLATFORM_WIFI
#define HAL_PLATFORM_NETWORK_MULTICAST (1)
#endif // HAL_PLATFORM_WIFI

#if PLATFORM_ID == PLATFORM_GCC
#define PRODUCT_SERIES                      "gcc"
#endif

#if PLATFORM_ID == PLATFORM_NEWHAL
#define PRODUCT_SERIES                      "newhal"
#endif

#if PLATFORM_ID == PLATFORM_PHOTON_PRODUCTION || PLATFORM_ID == PLATFORM_P1
#define PRODUCT_SERIES                      "photon"
#endif

#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
#define PRODUCT_SERIES                      "electron"
#endif

#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
#define HAL_PLATFORM_SPI_NUM (3)
#define HAL_PLATFORM_I2C_NUM (3)
#define HAL_PLATFORM_USART_NUM (5)
#else
#define HAL_PLATFORM_SPI_NUM (2)
#define HAL_PLATFORM_I2C_NUM (1)
#define HAL_PLATFORM_USART_NUM (5)
#endif // PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION

#if defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE
#define HAL_PLATFORM_MULTIPART_SYSTEM 1
#endif

#endif /* HAL_PLATFORM_COMPAT_H */
