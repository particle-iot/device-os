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
#pragma once

#define HAL_PLATFORM_CLOUD_UDP (1)

#define HAL_PLATFORM_DCT (1)

#define HAL_USE_SOCKET_HAL_COMPAT (0)

/** Only for HAL_IPAddress */
#define HAL_USE_INET_HAL_COMPAT (1)

#define HAL_USE_SOCKET_HAL_POSIX (1)
#define HAL_USE_INET_HAL_POSIX (1)

#define HAL_PLATFORM_MESH_DEPRECATED (1)

#define HAL_PLATFORM_LWIP (1)

#define HAL_PLATFORM_FILESYSTEM (1)

#define HAL_IPv6 (1)

#define HAL_PLATFORM_BLE (1)

#define HAL_PLATFORM_BLE_SETUP (1)

#define HAL_PLATFORM_NFC (1)

#define HAL_PLATFORM_NRF52840 (1)

/* 25 seconds */
#define HAL_PLATFORM_DEFAULT_CLOUD_KEEPALIVE_INTERVAL (25000)

/* XXX: hardcoded 23 minutes for now */
#define HAL_PLATFORM_CELLULAR_CLOUD_KEEPALIVE_INTERVAL (23 * 60 * 1000)

#define HAL_PLATFORM_IFAPI (1)

#define HAL_PLATFORM_ETHERNET (1)

#define HAL_PLATFORM_I2C2 (1)

#define HAL_PLATFORM_USB_VENDOR_REQUEST (1)

#define HAL_PLATFORM_USB_CDC (1)

#define HAL_PLATFORM_USB_HID (1)

#define HAL_PLATFORM_USB_COMPOSITE (1)

#define HAL_PLATFORM_USB_CONTROL_INTERFACE (1)

#define HAL_PLATFORM_RNG (1)

#define HAL_PLATFORM_SPI_DMA_SOURCE_RAM_ONLY (1)

#define HAL_PLATFORM_SPI_HAL_THREAD_SAFETY (1)

#define HAL_PLATFORM_FILESYSTEM (1)

#define HAL_PLATFORM_CORE_ENTER_PANIC_MODE (1)

#define HAL_PLATFORM_NETWORK_MULTICAST (1)

#define HAL_PLATFORM_BUTTON_DEBOUNCE_IN_SYSTICK (1)

#define HAL_PLATFORM_IPV6 (1)

#define HAL_PLATFORM_POWER_WORKAROUND_USB_HOST_VIN_SOURCE (1)

#define HAL_PLATFORM_RADIO_STACK (1)

#define HAL_PLATFORM_BACKUP_RAM (1)

#define HAL_PLATFORM_PACKET_BUFFER_FLOW_CONTROL_THRESHOLD (2)

#define HAL_PLATFORM_COMPRESSED_OTA (1)

#define HAL_PLATFORM_FILE_MAXIMUM_FD (999)

#define HAL_PLATFORM_SOCKET_IOCTL_NOTIFY (1)

#define HAL_PLATFORM_NEWLIB (1)

#define HAL_PLATFORM_EXPORT_STDLIB_RT_DYNALIB (1)

#define HAL_PLATFORM_OTA_PROTOCOL_V3 (1)

#define HAL_PLATFORM_RESUMABLE_OTA (1)

#define HAL_PLATFORM_ERROR_MESSAGES (1)

#define HAL_PLATFORM_PROHIBIT_XIP (1)

// hardware counter for System.ticks() supported
#define HAL_PLATFORM_SYSTEM_HW_TICKS (1)

#define HAL_PLATFORM_HW_WATCHDOG (1)
#define HAL_PLATFORM_HW_WATCHDOG_COUNT (1)

#define HAL_PLATFORM_ASSETS (1)

#if MODULE_FUNCTION != 2 // MOD_FUNC_BOOTLOADER
#define HAL_PLATFORM_INFLATE_USE_FILESYSTEM (1)
#endif
