#include "ota_flash_hal.h"
#include "spark_macros.h"
#include "hal_platform.h"
#include "flash_mal.h"
#include "static_assert.h"
#include "nrf_mbr.h"

// Bootloader
const module_bounds_t module_bootloader = {
        .maximum_size = 0x0000c000, // bootloader_flash_length
        .start_address = 0x000f4000, // bootloader_flash_origin
        .end_address = 0x00100000,
        .module_function = MODULE_FUNCTION_BOOTLOADER,
        .module_index = 0,
        .store = MODULE_STORE_MAIN
#if HAL_PLATFORM_NCP
        ,.mcu_identifier = HAL_PLATFORM_MCU_DEFAULT
#endif
        ,.location = MODULE_BOUNDS_LOC_INTERNAL_FLASH
    };

// OTA region
const module_bounds_t module_ota = {
        .maximum_size = EXTERNAL_FLASH_OTA_LENGTH,
        .start_address = EXTERNAL_FLASH_OTA_ADDRESS,
        .end_address = EXTERNAL_FLASH_OTA_ADDRESS + EXTERNAL_FLASH_OTA_LENGTH,
        .module_function = MODULE_FUNCTION_NONE,
        .module_index = 0,
        .store = MODULE_STORE_SCRATCHPAD
#if HAL_PLATFORM_NCP
        ,.mcu_identifier = HAL_PLATFORM_MCU_ANY
#endif
        ,.location = MODULE_BOUNDS_LOC_SERIAL_FLASH
    };

STATIC_ASSERT(Expected_OTA_region_at_end_of_external_flash, (EXTERNAL_FLASH_OTA_ADDRESS + EXTERNAL_FLASH_OTA_LENGTH) == EXTERNAL_FLASH_SIZE);


// Modular firmware
const module_bounds_t module_system_part1 = {
        .maximum_size = 0x000A4000, // 1M - APP_CODE_BASE - bootloader_flash_length - user_flash_length
        .start_address = 0x00030000, // APP_CODE_BASE
        .end_address = 0x000D4000,
        .module_function = MODULE_FUNCTION_SYSTEM_PART,
        .module_index = 1,
        .store = MODULE_STORE_MAIN
#if HAL_PLATFORM_NCP
        ,.mcu_identifier = HAL_PLATFORM_MCU_DEFAULT
#endif
        ,.location = MODULE_BOUNDS_LOC_INTERNAL_FLASH
    };

const module_bounds_t module_user = {
        .maximum_size = 0x00020000, // 128K
        .start_address = 0x000D4000,
        .end_address = 0x000f4000,
        .module_function = MODULE_FUNCTION_USER_PART,
        .module_index = 1,
        .store = MODULE_STORE_MAIN
#if HAL_PLATFORM_NCP
        ,.mcu_identifier = HAL_PLATFORM_MCU_DEFAULT
#endif
        ,.location = MODULE_BOUNDS_LOC_INTERNAL_FLASH
    };

// Factory firmware
const module_bounds_t module_factory = {
        .maximum_size = EXTERNAL_FLASH_FAC_LENGTH, // module_user_app.maximum_size
        .start_address = EXTERNAL_FLASH_FAC_ADDRESS,
        .end_address = EXTERNAL_FLASH_FAC_ADDRESS + EXTERNAL_FLASH_FAC_LENGTH,
        .module_function = MODULE_FUNCTION_USER_PART,
        .module_index = 1,
        .store = MODULE_STORE_FACTORY
#if HAL_PLATFORM_NCP
        ,.mcu_identifier = HAL_PLATFORM_MCU_DEFAULT
#endif
        ,.location = MODULE_BOUNDS_LOC_SERIAL_FLASH
    };


// Monolithic firmware
const module_bounds_t module_user_mono = {
        .maximum_size = 0x000c4000, // 1M - APP_CODE_BASE - bootloader_flash_length
        .start_address = 0x00030000, // APP_CODE_BASE
        .end_address = 0x000f4000, // APP_CODE_BASE + module_user_mono.maximum_size
        .module_function = MODULE_FUNCTION_MONO_FIRMWARE,
        .module_index = 0,
        .store = MODULE_STORE_MAIN
#if HAL_PLATFORM_NCP
        ,.mcu_identifier = HAL_PLATFORM_MCU_DEFAULT
#endif
        ,.location = MODULE_BOUNDS_LOC_INTERNAL_FLASH
    };

const module_bounds_t module_factory_mono = {
        .maximum_size = EXTERNAL_FLASH_FAC_LENGTH, // module_user_app.maximum_size
        .start_address = EXTERNAL_FLASH_FAC_ADDRESS,
        .end_address = EXTERNAL_FLASH_FAC_ADDRESS + EXTERNAL_FLASH_FAC_LENGTH,
        .module_function = MODULE_FUNCTION_MONO_FIRMWARE,
        .module_index = 0,
        .store = MODULE_STORE_FACTORY
#if HAL_PLATFORM_NCP
        ,.mcu_identifier = HAL_PLATFORM_MCU_DEFAULT
#endif
        ,.location = MODULE_BOUNDS_LOC_SERIAL_FLASH
    };

// placeholder for unused space
const module_bounds_t module_xip_code = {
        .maximum_size = EXTERNAL_FLASH_RESERVED_LENGTH,
        .start_address = EXTERNAL_FLASH_RESERVED_XIP_ADDRESS, // module_factory_modular.end_address
        .end_address = EXTERNAL_FLASH_RESERVED_XIP_ADDRESS + EXTERNAL_FLASH_RESERVED_LENGTH,
        .module_function = MODULE_FUNCTION_NONE,
        .module_index = 0,
        .store = MODULE_STORE_SCRATCHPAD
#if HAL_PLATFORM_NCP
        ,.mcu_identifier = HAL_PLATFORM_MCU_DEFAULT
#endif
        ,.location = MODULE_BOUNDS_LOC_INTERNAL_FLASH
    };

#if HAL_PLATFORM_NCP_UPDATABLE
const module_bounds_t module_ncp_mono = {
        .maximum_size = 1500*1024,
        .start_address = 0,
        .end_address = 1500*1024,
        .module_function = MODULE_FUNCTION_NCP_FIRMWARE,
        .module_index = 0,
        .store = MODULE_STORE_MAIN
#if HAL_PLATFORM_NCP
        ,.mcu_identifier = HAL_PLATFORM_MCU_ANY
#endif
        ,.location = MODULE_BOUNDS_LOC_NCP_FLASH
};
#endif

#define MODULE_RADIO_STACK_START_ADDRESS (0x1000)
const module_bounds_t module_radio_stack = {
        .maximum_size = 0x30000 - MODULE_RADIO_STACK_START_ADDRESS, // APP_CODE_BASE - MBR_SIZE
        .start_address = MODULE_RADIO_STACK_START_ADDRESS, // MBR_SIZE
        .end_address = 0x30000, // APP_CODE_BASE in softdevice.ld
        .module_function = MODULE_FUNCTION_RADIO_STACK,
        .module_index = 0,
        .store = MODULE_STORE_MAIN
#if HAL_PLATFORM_NCP
        ,.mcu_identifier = HAL_PLATFORM_MCU_DEFAULT
#endif
        ,.location = MODULE_BOUNDS_LOC_INTERNAL_FLASH
};
PARTICLE_STATIC_ASSERT(radio_stack_start_address, MODULE_RADIO_STACK_START_ADDRESS == MBR_SIZE);

#if defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE
const module_bounds_t* const module_bounds[] = { &module_bootloader, &module_system_part1, &module_user, &module_factory
#if defined(HYBRID_BUILD) // include the mono module so that the hybrid module validates. The reason to not do this all the time is because the
        // list of modules becomes somewhat confusing (with the same address range covered by 2 distinct definitions.)
,&module_user_mono
#endif // defined(HYBRID_BUILD)
#else
const module_bounds_t* const module_bounds[] = { &module_bootloader, &module_user_mono
#endif /* defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE */
#if HAL_PLATFORM_NCP_UPDATABLE
        ,&module_ncp_mono
#endif /* HAL_PLATFORM_NCP */
        , &module_radio_stack
};


const unsigned module_bounds_length = arraySize(module_bounds);
