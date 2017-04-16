#include "bootloader_dct.h"

#include <stdint.h>
#include <stddef.h>

#ifdef LOAD_DCT_FUNCTIONS

#include "flash_mal.h"
#include "usbd_dct_if.h"

#define MODULAR_FIRMWARE 1
#include "../../../hal/src/photon/ota_module_bounds.c"

static const void*(*HAL_DCT_Read_App_Data)(uint32_t, void*) = NULL;
static int(*HAL_DCT_Write_App_Data)(const void*, uint32_t, uint32_t, void*) = NULL;
static uint8_t dct_func_loaded = 0;

static void load_dct_functions() {
    HAL_DCT_Read_App_Data = NULL;
    HAL_DCT_Write_App_Data = NULL;
    // Get module info
    const module_info_t* module = FLASH_ModuleInfo(FLASH_INTERNAL, module_system_part2.start_address);
    if (!module || module->module_function != MODULE_FUNCTION_SYSTEM_PART || module->module_index != 2 ||
        module->platform_id != PLATFORM_ID || module->module_version < 107 /* 0.7.0-rc.1 */) {
        return;
    }
    // Check module boundaries
    const uintptr_t startAddr = (uintptr_t)module->module_start_address;
    const uintptr_t endAddr = (uintptr_t)module->module_end_address;
    if (endAddr < startAddr || startAddr != module_system_part2.start_address ||
            endAddr > module_system_part2.end_address) {
        return;
    }
    // Verify checksum
    if (!FLASH_VerifyCRC32(FLASH_INTERNAL, startAddr, endAddr - startAddr)) {
        return;
    }
    // Get addresses of the DCT functions
    void*** dynalib = (void***)((const char*)module + sizeof(module_info_t));
    void** dynalib_hal_core = dynalib[7];
    HAL_DCT_Read_App_Data = dynalib_hal_core[33];
    HAL_DCT_Write_App_Data = dynalib_hal_core[34];
}

void reset_dct_functions() {
    dct_func_loaded = 0;
}

const void* dct_read_app_data(uint32_t offset) {
    if (!dct_func_loaded) {
        load_dct_functions();
        dct_func_loaded = 1;
    }
    if (HAL_DCT_Read_App_Data) {
        return HAL_DCT_Read_App_Data(offset, NULL /* reserved */);
    }
    return NULL;
}

int dct_write_app_data(const void* data, uint32_t offset, uint32_t size) {
    if (!dct_func_loaded) {
        load_dct_functions();
        dct_func_loaded = 1;
    }
    if (HAL_DCT_Write_App_Data) {
        return HAL_DCT_Write_App_Data(data, offset, size, NULL /* reserved */);
    }
    return -1;
}

#else // !defined(LOAD_DCT_FUNCTIONS)

#ifndef CRC_INIT_VALUE
#define CRC_INIT_VALUE 0xffffffff
#endif

#ifndef CRC_TYPE
#define CRC_TYPE uint32_t
#endif

#include "../hal/src/photon/dct_hal.c"

#endif
