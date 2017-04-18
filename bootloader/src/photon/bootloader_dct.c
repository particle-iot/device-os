#include "bootloader_dct.h"

#include "flash_mal.h"

#include <stdint.h>
#include <stddef.h>

#define MODULAR_FIRMWARE 1
#include "../../../hal/src/photon/ota_module_bounds.c"

#define SYSTEM_PART2_MIN_MODULE_VERSION 107 // 0.7.0-rc.1
#define DYNALIB_HAL_CORE_INDEX 7
#define HAL_DCT_READ_APP_DATA_INDEX 33
#define HAL_DCT_WRITE_APP_DATA_INDEX 34

static const void*(*HAL_DCT_Read_App_Data)(uint32_t, void*) = NULL;
static int(*HAL_DCT_Write_App_Data)(const void*, uint32_t, uint32_t, void*) = NULL;
static uint8_t dct_funcs_inited = 0;

static const module_info_t* get_module_info(const module_bounds_t* bounds, uint16_t min_version) {
    const module_info_t* module = FLASH_ModuleInfo(FLASH_INTERNAL, bounds->start_address);
    // Check primary module info
    if (!module || module->platform_id != PLATFORM_ID || module->module_function != bounds->module_function ||
            module->module_index != bounds->module_index || module->module_version < min_version) {
        return NULL;
    }
    // Check module boundaries
    const uintptr_t startAddr = (uintptr_t)module->module_start_address;
    const uintptr_t endAddr = (uintptr_t)module->module_end_address;
    if (endAddr < startAddr || startAddr != bounds->start_address || endAddr > bounds->end_address) {
        return NULL;
    }
    // Verify checksum
    if (!FLASH_VerifyCRC32(FLASH_INTERNAL, startAddr, endAddr - startAddr)) {
        return NULL;
    }
    return module;
}

static void init_dct_functions() {
    HAL_DCT_Read_App_Data = NULL;
    HAL_DCT_Write_App_Data = NULL;
    const module_info_t* part2 = get_module_info(&module_system_part2, SYSTEM_PART2_MIN_MODULE_VERSION);
    if (!part2) {
        return;
    }
    // Part2 should contain complete DCT implementation, but it's easy to introduce an additional
    // dependency during development, so we require part1 to be consistent as well
    const module_info_t* part1 = get_module_info(&module_system_part1, part2->dependency.module_version);
    if (!part1 || part1->dependency.module_function != MODULE_FUNCTION_NONE) {
        return;
    }
    // Get addresses of the DCT functions
    void*** dynalib = (void***)((const char*)part2 + sizeof(module_info_t));
    void** dynalib_hal_core = dynalib[DYNALIB_HAL_CORE_INDEX];
    HAL_DCT_Read_App_Data = dynalib_hal_core[HAL_DCT_READ_APP_DATA_INDEX];
    HAL_DCT_Write_App_Data = dynalib_hal_core[HAL_DCT_WRITE_APP_DATA_INDEX];
}

void load_dct_functions() {
    dct_funcs_inited = 0;
}

const void* dct_read_app_data(uint32_t offset) {
    if (!dct_funcs_inited) {
        init_dct_functions();
        dct_funcs_inited = 1;
    }
    if (HAL_DCT_Read_App_Data) {
        return HAL_DCT_Read_App_Data(offset, NULL /* reserved */);
    }
    return NULL;
}

int dct_write_app_data(const void* data, uint32_t offset, uint32_t size) {
    if (!dct_funcs_inited) {
        init_dct_functions();
        dct_funcs_inited = 1;
    }
    if (HAL_DCT_Write_App_Data) {
        return HAL_DCT_Write_App_Data(data, offset, size, NULL /* reserved */);
    }
    return -1;
}
