#include "ota_flash_hal.h"
#include "spark_macros.h"
#include "hal_platform.h"
#include "flash_mal.h"
#include "static_assert.h"

// Bootloader
const module_bounds_t module_bootloader = {
    .maximum_size = 0x00010000,
    .start_address = 0x08004000,
    .end_address = 0x08014000,
    .module_function = MODULE_FUNCTION_BOOTLOADER,
    .module_index = 0,
    .store = MODULE_STORE_MAIN
#if HAL_PLATFORM_NCP
    ,.mcu_identifier = HAL_PLATFORM_MCU_DEFAULT
#endif
    ,.location = MODULE_BOUNDS_LOC_INTERNAL_FLASH // XIP
};

// mbr
const module_bounds_t module_mbr = {
    .maximum_size = 0x00002000,
    .start_address = 0x08000000,
    .end_address = 0x08002000,
    .module_function = MODULE_FUNCTION_BOOTLOADER,
    .module_index = 1,
    .store = MODULE_STORE_MAIN
#if HAL_PLATFORM_NCP
    ,.mcu_identifier = HAL_PLATFORM_MCU_DEFAULT
#endif
    ,.location = MODULE_BOUNDS_LOC_INTERNAL_FLASH // XIP
};

// km0 part1
const module_bounds_t module_km0_part1 = {
    .maximum_size = 0x4A000,
    .start_address = 0x08014000,
    .end_address = 0x0805E000,
    .module_function = MODULE_FUNCTION_BOOTLOADER,
    .module_index = 2,
    .store = MODULE_STORE_MAIN
#if HAL_PLATFORM_NCP
    ,.mcu_identifier = HAL_PLATFORM_MCU_DEFAULT
#endif
    ,.location = MODULE_BOUNDS_LOC_INTERNAL_FLASH // XIP
};

// OTA region, to be updated.
module_bounds_t module_ota = {
    .maximum_size = 0x180000, // 1.5MB
    .start_address = 0x08260000,
    .end_address = 0x08260000 + 0x180000,
    .module_function = MODULE_FUNCTION_NONE,
    .module_index = 0,
    .store = MODULE_STORE_SCRATCHPAD
#if HAL_PLATFORM_NCP
    ,.mcu_identifier = HAL_PLATFORM_MCU_ANY
#endif
    ,.location = MODULE_BOUNDS_LOC_INTERNAL_FLASH // XIP
};


// Modular firmware
const module_bounds_t module_system_part1 = {
    .maximum_size = 0x180000, // 1.5MB
    .start_address = 0x08060000,
    .end_address = 0x081E0000,
    .module_function = MODULE_FUNCTION_SYSTEM_PART,
    .module_index = 1,
    .store = MODULE_STORE_MAIN
#if HAL_PLATFORM_NCP
    ,.mcu_identifier = HAL_PLATFORM_MCU_DEFAULT
#endif
    ,.location = MODULE_BOUNDS_LOC_INTERNAL_FLASH // XIP
};

// To be updated
module_bounds_t module_user = {
    .maximum_size = 0x180000, // 1.5MB
    .start_address = 0x08600000 - 0x180000,
    .end_address = 0x08600000,
    .module_function = MODULE_FUNCTION_USER_PART,
    .module_index = 1,
    .store = MODULE_STORE_MAIN
#if HAL_PLATFORM_NCP
    ,.mcu_identifier = HAL_PLATFORM_MCU_DEFAULT
#endif
    ,.location = MODULE_BOUNDS_LOC_INTERNAL_FLASH // XIP
};

// Monolithic firmware
const module_bounds_t module_user_mono = {
    .maximum_size = 0x400000, // 4M
    .start_address = 0x08060000,
    .end_address = 0x08460000,
    .module_function = MODULE_FUNCTION_MONO_FIRMWARE,
    .module_index = 0,
    .store = MODULE_STORE_MAIN
#if HAL_PLATFORM_NCP
    ,.mcu_identifier = HAL_PLATFORM_MCU_DEFAULT
#endif
    ,.location = MODULE_BOUNDS_LOC_INTERNAL_FLASH // XIP
};

#if defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE
const module_bounds_t* const module_bounds[] = { &module_bootloader, &module_mbr, &module_km0_part1, &module_system_part1, &module_user
#else
const module_bounds_t* const module_bounds[] = { &module_bootloader, &module_mbr, &module_km0_part1, &module_user_mono
#endif /* defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE */
};


const unsigned module_bounds_length = arraySize(module_bounds);
