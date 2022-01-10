#ifndef OTA_FLASH_HAL_IMPL_H
#define	OTA_FLASH_HAL_IMPL_H

#include "ota_flash_hal.h"
#include "platforms.h"
#include "hal_platform.h"

extern const module_bounds_t* module_bounds[];
extern const unsigned module_bounds_length;

extern const module_bounds_t module_bootloader;
extern const module_bounds_t module_ota;

// Modular firmware
extern const module_bounds_t module_system_part1;
extern const module_bounds_t module_user;
extern const module_bounds_t module_user_compat;
extern const module_bounds_t module_factory;

// Monolithic firmware
extern const module_bounds_t module_user_mono;
extern const module_bounds_t module_factory_mono;

#if HAL_PLATFORM_NCP_UPDATABLE
extern const module_bounds_t module_ncp_mono;
#endif // HAL_PLATFORM_NCP_UPDATABLE

extern const module_bounds_t module_radio_stack;

const uint8_t* fetch_server_public_key(uint8_t lock);
const uint8_t* fetch_device_private_key(uint8_t lock);
const uint8_t* fetch_device_public_key(uint8_t lock);

void set_key_value(key_value* kv, const char* key, const char* value);

/**
 * Fetches the key-value instances and the count of system properties for Gen 3 devices.
 * @param storage	When not-null, points to storage with keyCount free entries.
 * 					When null, is used to retrieve the number of keys available.
 * @return The number of keys available, when storage is null, or the number of keys
 * copied to storage when not null.
 */
int fetch_system_properties(key_value* storage, int keyCount);

/**
 * Adds the system properties for Gen 3 devices to the system info, allocating the storage required
 * plus an additional amount for platform-specific properties.
 */
int add_system_properties(hal_system_info_t* info, bool create, size_t additional);

#endif

