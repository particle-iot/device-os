#include "module.h"

#include "flash_mal.h"

#define MODULAR_FIRMWARE 1
#include "ota_module_bounds.c"

const module_bounds_t* get_module_bounds(uint8_t module_func, uint8_t module_index) {
    for (size_t i = 0; i < module_bounds_length; ++i) {
        const module_bounds_t* bounds = module_bounds[i];
        if (bounds->module_function == module_func && bounds->module_index == module_index) {
            return bounds;
        }
    }
    return NULL;
}

const module_info_t* get_module_info(const module_bounds_t* bounds) {
    const module_info_t* module = FLASH_ModuleInfo(FLASH_INTERNAL, bounds->start_address);
    // Check primary module info
    if (!module || module->platform_id != PLATFORM_ID || module->module_function != bounds->module_function ||
            module->module_index != bounds->module_index) {
        return NULL;
    }
    return module;
}

int verify_module(const module_info_t* module, const module_bounds_t* bounds) {
    // Check module boundaries
    const uintptr_t startAddr = (uintptr_t)module->module_start_address;
    const uintptr_t endAddr = (uintptr_t)module->module_end_address;
    if (endAddr < startAddr || startAddr != bounds->start_address || endAddr > bounds->end_address) {
        return -1;
    }
    // Verify checksum
    if (!FLASH_VerifyCRC32(FLASH_INTERNAL, startAddr, endAddr - startAddr)) {
        return -1;
    }
    return 0;
}

const void* get_module_func(const module_info_t* module, size_t dynalib_index, size_t func_index) {
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

int get_main_module_version() {
    const module_info_t* module = get_module_info(&module_system_part2);
    if (!module) {
        // Monolithic firmware?
        module = get_module_info(&module_user_mono);
        if (!module) {
            return -1;
        }
    }
    return module->module_version;
}
