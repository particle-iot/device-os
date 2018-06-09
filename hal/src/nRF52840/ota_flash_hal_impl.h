#ifndef OTA_FLASH_HAL_IMPL_H
#define	OTA_FLASH_HAL_IMPL_H

#include "ota_flash_hal.h"
#include "platforms.h"

typedef enum security_key_type {
    INVALID_SECURITY_KEY = 0,
    TCP_DEVICE_PRIVATE_KEY = 1,
    TCP_DEVICE_PUBLIC_KEY = 2,
    TCP_SERVER_PUBLIC_KEY = 3,
    UDP_DEVICE_PRIVATE_KEY = 4,
    UDP_DEVICE_PUBLIC_KEY = 5,
    UDP_SERVER_PUBLIC_KEY = 6
} security_key_type;

typedef enum server_protocol_type {
    INVALID_SERVER_PROTOCOL = 0,
    TCP_SERVER_PROTOCOL = 1,
    UDP_SERVER_PROTOCOL = 2
} server_protocol_type;

extern const module_bounds_t* module_bounds[];
extern const unsigned module_bounds_length;

extern const module_bounds_t module_bootloader;

// Modular firmware
extern const module_bounds_t module_system_part1;
extern const module_bounds_t module_system_part2;
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
extern const module_bounds_t module_system_part3;
#endif
extern const module_bounds_t module_user;
extern const module_bounds_t module_ota;
extern const module_bounds_t module_factory;

// Monolithic firmware
extern const module_bounds_t module_user_mono;
extern const module_bounds_t module_factory_mono;

void HAL_OTA_Add_System_Info(hal_system_info_t* info, bool create, void* reserved);

const uint8_t* fetch_server_public_key(uint8_t lock);
const uint8_t* fetch_device_private_key(uint8_t lock);
const uint8_t* fetch_device_public_key(uint8_t lock);

int lock_security_key_data(security_key_type type, const char** data, size_t* size);
void unlock_security_key_data(security_key_type type);
int store_security_key_data(security_key_type type, const char* data, size_t size);

// TODO: Update HAL_Set_System_Config() and implement HAL_Get_System_Config()
int load_server_address(server_protocol_type type, ServerAddress* addr);
int store_server_address(server_protocol_type type, const ServerAddress* addr);

void set_key_value(key_value* kv, const char* key, const char* value);

#endif

