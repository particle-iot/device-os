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

#ifndef HAL_PLATFORM_H
#define	HAL_PLATFORM_H

#if PLATFORM_ID <= 10 || PLATFORM_ID == 60000
/* FIXME: create platform-specific hal_platform_config.h header for each of these platforms */
#include "hal_platform_compat.h"
#else
/* Include platform-specific configuration header */
#include "hal_platform_config.h"
#endif /* PLATFORM_ID <= 10 || PLATFORM_ID == 60000 */

/* Define the defaults */
#ifndef HAL_PLATFORM_WIFI
#define HAL_PLATFORM_WIFI 0
#endif /* HAL_PLATFORM_WIFI */

#ifndef HAL_PLATFORM_CELLULAR
#define HAL_PLATFORM_CELLULAR 0
#endif /* HAL_PLATFORM_CELLULAR */

#ifndef HAL_PLATFORM_MESH
#define HAL_PLATFORM_MESH 0
#endif /* HAL_PLATFORM_MESH */

#ifndef HAL_PLATFORM_CLOUD_UDP
#define HAL_PLATFORM_CLOUD_UDP 0
#endif /* HAL_PLATFORM_CLOUD_UDP */

#ifndef HAL_PLATFORM_CLOUD_TCP
#define HAL_PLATFORM_CLOUD_TCP 0
#endif /* HAL_PLATFORM_CLOUD_TCP */

#ifndef HAL_PLATFORM_DCT
#define HAL_PLATFORM_DCT 0
#endif /* HAL_PLATFORM_DCT */

#ifndef HAL_PLATFORM_DCT_NO_DEPRECATED
#define HAL_PLATFORM_DCT_NO_DEPRECATED (1)
#endif /* HAL_PLATFORM_DCT_NO_DEPRECATED */

#ifndef PANIC_BUT_KEEP_CALM
#define PANIC_BUT_KEEP_CALM 0
#endif /* PANIC_BUT_KEEP_CALM */

#ifndef HAL_USE_SOCKET_HAL_COMPAT
#define HAL_USE_SOCKET_HAL_COMPAT (1)
#endif /* HAL_USE_SOCKET_HAL_COMPAT */

#ifndef HAL_USE_INET_HAL_COMPAT
#define HAL_USE_INET_HAL_COMPAT (1)
#endif /* HAL_USE_INET_HAL_COMPAT */

#ifndef HAL_USE_SOCKET_HAL_POSIX
#define HAL_USE_SOCKET_HAL_POSIX (0)
#endif /* HAL_USE_SOCKET_HAL_POSIX */

#ifndef HAL_USE_INET_HAL_POSIX
#define HAL_USE_INET_HAL_POSIX (0)
#endif /* HAL_USE_INET_HAL_POSIX */

#ifndef HAL_PLATFORM_OPENTHREAD
#define HAL_PLATFORM_OPENTHREAD (0)
#endif /* HAL_PLATFORM_OPENTHREAD */

#ifndef HAL_PLATFORM_OPENTHREAD_MAX_TX_POWER
#define HAL_PLATFORM_OPENTHREAD_MAX_TX_POWER (0) // dBm
#endif /* HAL_PLATFORM_OPENTHREAD_MAX_TX_POWER */

#ifndef HAL_PLATFORM_BLE
#define HAL_PLATFORM_BLE (0)
#endif /* HAL_PLATFORM_BLE */

#ifndef HAL_PLATFORM_LWIP
#define HAL_PLATFORM_LWIP (0)
#endif /* HAL_PLATFORM_LWIP */

#ifndef HAL_PLATFORM_FILESYSTEM
#define HAL_PLATFORM_FILESYSTEM (0)
#endif /* HAL_PLATFORM_FILESYSTEM */

#ifndef HAL_PLATFORM_IFAPI
#define HAL_PLATFORM_IFAPI (0)
#endif /* HAL_PLATFORM_IFAPI */

#ifndef HAL_PLATFORM_NRF52840
#define HAL_PLATFORM_NRF52840 (0)
#endif /* HAL_PLATFORM_NRF52840 */

#ifndef HAL_PLATFORM_NCP
#define HAL_PLATFORM_NCP (0)
#endif /* HAL_PLATFORM_NCP */

#ifndef HAL_PLATFORM_NCP_AT
#define HAL_PLATFORM_NCP_AT (0)
#endif /* HAL_PLATFORM_NCP_AT */

#ifndef HAL_PLATFORM_NCP_UPDATABLE
#define HAL_PLATFORM_NCP_UPDATABLE (0)
#endif /* HAL_PLATFORM_NCP_UPDATABLE */

#define HAL_PLATFORM_MCU_ANY (0xFF)
#define HAL_PLATFORM_MCU_DEFAULT (0)

#ifndef HAL_PLATFORM_ETHERNET
#define HAL_PLATFORM_ETHERNET (0)
#endif /* HAL_PLATFORM_ETHERNET */

#ifndef HAL_PLATFORM_I2C1
#define HAL_PLATFORM_I2C1 (1)
#endif /* HAL_PLATFORM_I2C1 */

#ifndef HAL_PLATFORM_I2C2
#define HAL_PLATFORM_I2C2 (0)
#endif /* HAL_PLATFORM_I2C2 */

#ifndef HAL_PLATFORM_I2C3
#define HAL_PLATFORM_I2C3 (0)
#endif /* HAL_PLATFORM_I2C3 */

#ifndef HAL_PLATFORM_USART2
#define HAL_PLATFORM_USART2 (0)
#endif /* HAL_PLATFORM_USART2 */

#ifndef HAL_PLATFORM_POWER_MANAGEMENT
#define HAL_PLATFORM_POWER_MANAGEMENT (0)
#endif /* HAL_PLATFORM_POWER_MANAGEMENT */

#ifndef HAL_PLATFORM_PMIC_BQ24195
#define HAL_PLATFORM_PMIC_BQ24195 (0)
#endif /* HAL_PLATFORM_PMIC_BQ24195 */

#ifndef HAL_PLATFORM_PMIC_BQ24195_FAULT_COUNT_THRESHOLD
#define HAL_PLATFORM_PMIC_BQ24195_FAULT_COUNT_THRESHOLD (5)
#endif /* HAL_PLATFORM_PMIC_BQ24195_FAULT_COUNT_THRESHOLD */

#if HAL_PLATFORM_PMIC_BQ24195
# ifndef HAL_PLATFORM_PMIC_BQ24195_I2C
#  error "HAL_PLATFORM_PMIC_BQ24195_I2C is not defined"
# endif /* HAL_PLATFORM_PMIC_BQ24195_I2C */
#endif /* HAL_PLATFORM_PMIC_BQ24195 */

#ifndef HAL_PLATFORM_FUELGAUGE_MAX17043
#define HAL_PLATFORM_FUELGAUGE_MAX17043 (0)
#endif /* HAL_PLATFORM_FUELGAUGE_MAX17043 */

#if HAL_PLATFORM_FUELGAUGE_MAX17043
# ifndef HAL_PLATFORM_FUELGAUGE_MAX17043_I2C
#  error "HAL_PLATFORM_FUELGAUGE_MAX17043_I2C is not defined"
# endif /* HAL_PLATFORM_FUELGAUGE_MAX17043_I2C */
#endif /* HAL_PLATFORM_FUELGAUGE_MAX17043 */

#ifndef HAL_OPENTHREAD_USE_LWIP_LOCK
#define HAL_OPENTHREAD_USE_LWIP_LOCK (1)
#endif // HAL_OPENTHREAD_USE_LWIP_LOCK

#ifndef HAL_PLATFORM_USB
#define HAL_PLATFORM_USB (1)
#endif // HAL_PLATFORM_USB

#ifndef HAL_PLATFORM_USB_CDC
#define HAL_PLATFORM_USB_CDC (0)
#endif // HAL_PLATFORM_USB_CDC

#ifndef HAL_PLATFORM_USB_HID
#define HAL_PLATFORM_USB_HID (0)
#endif // HAL_PLATFORM_USB_HID

#ifndef HAL_PLATFORM_USB_COMPOSITE
#define HAL_PLATFORM_USB_COMPOSITE (0)
#endif // HAL_PLATFORM_USB_COMPOSITE

#ifndef HAL_PLATFORM_USB_CONTROL_INTERFACE
#define HAL_PLATFORM_USB_CONTROL_INTERFACE (0)
#endif // HAL_PLATFORM_USB_CONTROL_INTERFACE

#ifndef HAL_PLATFORM_RNG
#define HAL_PLATFORM_RNG (0)
#endif

#endif /* HAL_PLATFORM_H */
