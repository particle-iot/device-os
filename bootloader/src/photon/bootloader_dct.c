#include "bootloader_dct.h"

#include "flash_mal.h"

#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include "../../../hal/src/stm32f2xx/dct_hal_stm32f2xx.h"

#define MODULAR_FIRMWARE 1
#include "../../../hal/src/photon/ota_module_bounds.c"

#define MIN_MODULE_VERSION_SYSTEM_PART2 107 // 0.7.0-rc.1

#define DYNALIB_INDEX_SYSTEM_MODULE_PART1 2 // system_module_part1
#define FUNC_INDEX_MODULE_SYSTEM_PART1_PRE_INIT 0 // module_system_part1_pre_init()

#define DYNALIB_INDEX_SYSTEM_MODULE_PART2 19 // system_module_part2
#define FUNC_INDEX_MODULE_SYSTEM_PART2_PRE_INIT 0 // module_system_part2_pre_init()

#define DYNALIB_INDEX_HAL_DCT 18 // hal_dct
#define FUNC_INDEX_DCT_READ_APP_DATA 0 // dct_read_app_data()
#define FUNC_INDEX_DCT_WRITE_APP_DATA 4 // dct_write_app_data()
#define FUNC_INDEX_DCT_SET_LOCK_ENABLED 5 // dct_set_lock_enabled()

typedef const void*(*dct_read_app_data_func_t)(uint32_t);
typedef int(*dct_write_app_data_func_t)(const void*, uint32_t, uint32_t);
typedef void(*dct_set_lock_enabled_func_t)(int);
typedef void*(*module_pre_init_func_t)();

static dct_read_app_data_func_t dct_read_app_data_func = NULL;
static dct_write_app_data_func_t dct_write_app_data_func = NULL;
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

static const void* get_module_func(const module_info_t* module, size_t dynalib_index, size_t func_index) {
    // Get dynalib table
    void*** module_table = (void***)((const char*)module + sizeof(module_info_t));
    void** dynalib = module_table[dynalib_index];
    // Get function address
    void* func = dynalib[func_index];
    if (func < module->module_start_address || func >= module->module_end_address) {
        return NULL;
    }
    return func;
}

static void init_dct_functions() {
    dct_read_app_data_func = NULL;
    dct_write_app_data_func = NULL;
    const module_info_t* part2 = get_module_info(&module_system_part2, MIN_MODULE_VERSION_SYSTEM_PART2);
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
    dct_read_app_data_func_t dct_read = get_module_func(part2, DYNALIB_INDEX_HAL_DCT, FUNC_INDEX_DCT_READ_APP_DATA);
    dct_write_app_data_func_t dct_write = get_module_func(part2, DYNALIB_INDEX_HAL_DCT, FUNC_INDEX_DCT_WRITE_APP_DATA);
    dct_set_lock_enabled_func_t dct_set_lock_enabled = get_module_func(part2, DYNALIB_INDEX_HAL_DCT, FUNC_INDEX_DCT_SET_LOCK_ENABLED);
    if (!dct_read || !dct_write || !dct_set_lock_enabled) {
        return;
    }
    // Initialize static data of each module
    module_pre_init_func_t part1_init = get_module_func(part1, DYNALIB_INDEX_SYSTEM_MODULE_PART1,
            FUNC_INDEX_MODULE_SYSTEM_PART1_PRE_INIT);
    module_pre_init_func_t part2_init = get_module_func(part2, DYNALIB_INDEX_SYSTEM_MODULE_PART2,
            FUNC_INDEX_MODULE_SYSTEM_PART2_PRE_INIT);
    if (!part1_init || !part2_init) {
        return;
    }
    part1_init();
    part2_init();
    // Disable global DCT lock
    dct_set_lock_enabled(0);
    dct_read_app_data_func = dct_read;
    dct_write_app_data_func = dct_write;
}

void load_dct_functions() {
    dct_funcs_inited = 0;
}

const void* dct_read_app_data(uint32_t offset) {
    if (!dct_funcs_inited) {
        init_dct_functions();
        dct_funcs_inited = 1;
    }
    if (dct_read_app_data_func) {
        return dct_read_app_data_func(offset);
    }
    return NULL;
}

int dct_write_app_data(const void* data, uint32_t offset, uint32_t size) {
    if (!dct_funcs_inited) {
        init_dct_functions();
        dct_funcs_inited = 1;
    }
    if (dct_write_app_data_func) {
        return dct_write_app_data_func(data, offset, size);
    }
    return -1;
}

int dct_read_app_data_copy(uint32_t offset, void* ptr, size_t size) {
    const void* data = dct_read_app_data(offset);
    if (!data) {
        return -1;
    }
    memcpy(ptr, data, size);
    return 0;
}

const void* dct_read_app_data_lock(uint32_t offset) {
    return dct_read_app_data(offset);
}

int dct_read_app_data_unlock(uint32_t offset) {
    return 0;
}
