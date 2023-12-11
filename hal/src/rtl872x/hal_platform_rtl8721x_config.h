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

#define HAL_PLATFORM_LWIP (1)

#define HAL_PLATFORM_FILESYSTEM (1)

#define HAL_IPv6 (1)

#define HAL_PLATFORM_BLE (1)

#define HAL_PLATFORM_BLE_SETUP (1)

#define HAL_PLATFORM_RTL872X (1)

/* 25 seconds */
#define HAL_PLATFORM_DEFAULT_CLOUD_KEEPALIVE_INTERVAL (25000)

/* XXX: hardcoded 23 minutes for now */
#define HAL_PLATFORM_CELLULAR_CLOUD_KEEPALIVE_INTERVAL (23 * 60 * 1000)

#define HAL_PLATFORM_IFAPI (1)

// Allow detection of ethernet on SPI 
#define HAL_PLATFORM_ETHERNET (1)

#define HAL_PLATFORM_USART2 (1)

#define HAL_PLATFORM_USB_VENDOR_REQUEST (1)

#define HAL_PLATFORM_USB_CDC (1)

#define HAL_PLATFORM_USB_HID (1)

#define HAL_PLATFORM_USB_CONTROL_INTERFACE (1)

#define HAL_PLATFORM_RNG (1)

#define HAL_PLATFORM_SPI_DMA_SOURCE_RAM_ONLY (1)

#define HAL_PLATFORM_SPI_HAL_THREAD_SAFETY (1)

#define HAL_PLATFORM_FILESYSTEM (1)

#define HAL_PLATFORM_CORE_ENTER_PANIC_MODE (1)

#define HAL_PLATFORM_DCT_SETUP_DONE (0)

#define HAL_PLATFORM_NETWORK_MULTICAST (1)

#define HAL_PLATFORM_BUTTON_DEBOUNCE_IN_SYSTICK (1)

#define HAL_PLATFORM_IPV6 (1)

#define HAL_PLATFORM_RADIO_STACK (1)

#define HAL_PLATFORM_BACKUP_RAM (1)

#define HAL_PLATFORM_BACKUP_RAM_NEED_SYNC (1)

#define HAL_PLATFORM_PACKET_BUFFER_FLOW_CONTROL_THRESHOLD (2)

#define HAL_PLATFORM_COMPRESSED_OTA (1)

#define HAL_PLATFORM_FILE_MAXIMUM_FD (999)

#define HAL_PLATFORM_SOCKET_IOCTL_NOTIFY (1)

#define HAL_PLATFORM_NEWLIB (1)

#define HAL_PLATFORM_EXPORT_STDLIB_RT_DYNALIB (1)

#define HAL_PLATFORM_OTA_PROTOCOL_V3 (1)

#define HAL_PLATFORM_RESUMABLE_OTA (1)

#define HAL_PLATFORM_ERROR_MESSAGES (1)

#define HAL_PLATFORM_USB_DFU_INTERFACES (2)

#define HAL_PLATFORM_BOOTLOADER_USB_PROCESS_IN_MAIN_THREAD (1)

#if defined(MODULE_FUNCTION) && MODULE_FUNCTION == 2 // MOD_FUNC_BOOTLOADER
#define HAL_PLATFORM_USB_COMPOSITE (0)
#else
#define HAL_PLATFORM_USB_COMPOSITE (1)
#endif // defined(MODULE_FUNCTION) && MODULE_FUNCTION == 2 // MOD_FUNC_BOOTLOADER

// FIXME: variable suffix size causes problems right now, some refatoring will have to be done
// #if defined(MODULE_FUNCTION) && MODULE_FUNCTION == 5 // MOD_FUNC_USER_PART
#define HAL_PLATFORM_MODULE_DYNAMIC_LOCATION (1)
// #endif // defined(MODULE_FUNCTION) && MODULE_FUNCTION == 5 // MOD_FUNC_USER_PART

// No SOF support :(
#define HAL_PLATFORM_USB_SOF (0)

#define HAL_PLATFORM_SYSTEM_POOL_SIZE 8192

#define HAL_PLATFORM_MODULE_SUFFIX_EXTENSIONS (1)

#define HAL_PLATFORM_POWER_MANAGEMENT_STACK_SIZE (1536)

#define HAL_PLATFORM_BLE_ACTIVE_EVENT (1)

#define HAL_PLATFORM_SHARED_INTERRUPT (1)

#define HAL_PLATFORM_HW_WATCHDOG (1)

#define HAL_PLATFORM_HW_WATCHDOG_COUNT (1)

// We have plenty of SRAM/PSRAM, this avoids overflows in BLE stack initialization among other things
#define HAL_PLATFORM_SYSTEM_THREAD_STACK_SIZE (10 * 1024)

#define HAL_PLATFORM_BACKUP_RAM_SIZE (4096)

#define HAL_PLATFORM_HEAP_REGIONS (2)

// IMPORTANT: the region addresses should be in increasing order
// NOTE: These are not default in hal_platform.h
#define HAL_PLATFORM_HEAP_REGION_SRAM (0)
#define HAL_PLATFORM_HEAP_REGION_PSRAM (1)

#define HAL_PLATFORM_ASSETS (1)
