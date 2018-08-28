#include "ota_flash_hal.h"
#include "spark_macros.h"
#include "hal_platform.h"

#if defined(MODULAR_FIRMWARE) && MODULAR_FIRMWARE
#error "Modular firmware is not supported"
#endif

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

    };

// Factory firmware
const module_bounds_t module_factory_modular = {
        .maximum_size = 0x00020000, // module_user_app.maximum_size
        .start_address = 0x12200000, // XIP start address (0x12000000) + 2M
        .end_address = 0x12220000,
        .module_function = MODULE_FUNCTION_MONO_FIRMWARE,
        .module_index = 0,
        .store = MODULE_STORE_FACTORY
#if HAL_PLATFORM_NCP
		,.mcu_identifier = HAL_PLATFORM_MCU_DEFAULT
#endif
    };

// placeholder for unused space
const module_bounds_t module_xip_code = {
        .maximum_size = 0x69000,		// 430k
        .start_address = 0x12220000, // module_factory_modular.end_address
        .end_address = 0x12289000, // module_ota.start_address
        .module_function = MODULE_FUNCTION_NONE,
        .module_index = 0,
        .store = MODULE_STORE_SCRATCHPAD
#if HAL_PLATFORM_NCP
		,.mcu_identifier = HAL_PLATFORM_MCU_DEFAULT
#endif

    };


// OTA region
const module_bounds_t module_ota = {
        .maximum_size = 1500*1024,
        .start_address = 0x12289000, // XiP base+4M - maximum-size
        .end_address = 0x12400000,	// XiP base+4M
        .module_function = MODULE_FUNCTION_NONE,
        .module_index = 0,
        .store = MODULE_STORE_SCRATCHPAD
#if HAL_PLATFORM_NCP
		,.mcu_identifier = HAL_PLATFORM_MCU_ANY
#endif

    };

#if HAL_PLATFORM_NCP
const module_bounds_t module_ncp_mono = {
		.maximum_size = 1500*1024,
		.start_address = 0,
		.end_address = 1500*1024,
		.module_function = MODULE_FUNCTION_MONO_FIRMWARE,
		.module_index = 0,
		.store = MODULE_STORE_MAIN
#if HAL_PLATFORM_NCP
		,.mcu_identifier = HAL_PLATFORM_MCU_ANY
#endif
};
#endif

const module_bounds_t* const module_bounds[] = { &module_bootloader, &module_user_mono, &module_factory_modular
#if HAL_PLATFORM_NCP
		,&module_ncp_mono
#endif /* HAL_PLATFORM_NCP */
};

const unsigned module_bounds_length = arraySize(module_bounds);
