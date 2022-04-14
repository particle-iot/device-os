#include "ota_flash_hal.h"
#include "spark_macros.h"

const module_bounds_t module_bootloader = { 0x4000, 0x8000000, 0x8004000, MODULE_FUNCTION_BOOTLOADER, 0, MODULE_STORE_MAIN, 0, MODULE_BOUNDS_LOC_INTERNAL_FLASH };

// Modular firmware
const module_bounds_t module_system_part1 = { 0x20000, 0x8020000, 0x8040000, MODULE_FUNCTION_SYSTEM_PART, 1, MODULE_STORE_MAIN, 0, MODULE_BOUNDS_LOC_INTERNAL_FLASH };
const module_bounds_t module_system_part2 = { 0x20000, 0x8040000, 0x8060000, MODULE_FUNCTION_SYSTEM_PART, 2, MODULE_STORE_MAIN, 0, MODULE_BOUNDS_LOC_INTERNAL_FLASH };
const module_bounds_t module_system_part3 = { 0x20000, 0x8060000, 0x8080000, MODULE_FUNCTION_SYSTEM_PART, 3, MODULE_STORE_MAIN, 0, MODULE_BOUNDS_LOC_INTERNAL_FLASH };
const module_bounds_t module_user = { 0x20000, 0x8080000, 0x80A0000, MODULE_FUNCTION_USER_PART, 1, MODULE_STORE_MAIN, 0, MODULE_BOUNDS_LOC_INTERNAL_FLASH };
const module_bounds_t module_factory = { 0x20000, 0x80A0000, 0x80C0000, MODULE_FUNCTION_USER_PART, 1, MODULE_STORE_FACTORY, 0, MODULE_BOUNDS_LOC_INTERNAL_FLASH };
const module_bounds_t module_ota = { 0x20000, 0x80C0000, 0x80E0000, MODULE_FUNCTION_NONE, 0, MODULE_STORE_SCRATCHPAD, 0, MODULE_BOUNDS_LOC_INTERNAL_FLASH };

// Monolithic firmware
const module_bounds_t module_user_mono = { 0x60000, 0x8020000, 0x8080000, MODULE_FUNCTION_MONO_FIRMWARE, 0, MODULE_STORE_MAIN, 0, MODULE_BOUNDS_LOC_INTERNAL_FLASH };
const module_bounds_t module_factory_mono = { 0x60000, 0x8080000, 0x80E0000, MODULE_FUNCTION_MONO_FIRMWARE, 0, MODULE_STORE_FACTORY, 0, MODULE_BOUNDS_LOC_INTERNAL_FLASH };
const module_bounds_t module_ota_mono = { 0x60000, 0x8080000, 0x80E0000, MODULE_FUNCTION_NONE, 0, MODULE_STORE_SCRATCHPAD, 0, MODULE_BOUNDS_LOC_INTERNAL_FLASH };

// XXX: careful when adding new modules to this array without double checking
// HAL_System_Info() behavior or any other function that relies in module_bounds_length and
// the array itself.
#if defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE
const module_bounds_t* module_bounds[] = { &module_bootloader, &module_system_part1, &module_system_part2, &module_system_part3, &module_user, &module_factory, 0, MODULE_BOUNDS_LOC_INTERNAL_FLASH };
#else
const module_bounds_t* module_bounds[] = { &module_bootloader, &module_user_mono, &module_factory_mono };
#endif /* defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE */

const unsigned module_bounds_length = arraySize(module_bounds);
