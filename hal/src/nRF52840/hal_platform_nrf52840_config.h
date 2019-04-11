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

#define HAL_PLATFORM_MESH (1)

#define HAL_PLATFORM_OPENTHREAD (1)

#define HAL_PLATFORM_OPENTHREAD_MAX_TX_POWER (8) // dBm

#define HAL_PLATFORM_LWIP (1)

#define HAL_PLATFORM_FILESYSTEM (1)

#define HAL_IPv6 (1)

#define HAL_PLATFORM_BLE (1)

#define HAL_PLATFORM_NRF52840 (1)

/* 30 seconds */
#define HAL_PLATFORM_DEFAULT_CLOUD_KEEPALIVE_INTERVAL (30000)

/* XXX: hardcoded 23 minutes for now */
#define HAL_PLATFORM_BORON_CLOUD_KEEPALIVE_INTERVAL (23 * 60 * 1000)

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

#define HAL_PLATFORM_FILESYSTEM (1)

#define HAL_PLATFORM_CORE_ENTER_PANIC_MODE (1)

#define HAL_PLATFORM_DCT_SETUP_DONE (1)

#define HAL_PLATFORM_COMPRESSED_BINARIES (1)

#define HAL_PLATFORM_NETWORK_MULTICAST (1)

#define HAL_PLATFORM_BUTTON_DEBOUNCE_IN_SYSTICK (1)

#define HAL_PLATFORM_IPV6 (1)
