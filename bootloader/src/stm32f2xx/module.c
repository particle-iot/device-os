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
    const module_info_t* module = FLASH_ModuleInfo(FLASH_INTERNAL, bounds->start_address, NULL);
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

#ifdef MONO_MFG_FIRMWARE_AT_USER_PART
const module_info_t* get_mfg_firmware(void) {
    // Dummy module bounds based on module_user with modified function type, index and end address
    module_bounds_t module_mfg = module_user;
    module_mfg.module_function = MODULE_FUNCTION_MONO_FIRMWARE;
    module_mfg.module_index = 0;
    module_mfg.end_address = module_factory.end_address;

    // Fetch module info
    const module_info_t* mod = get_module_info(&module_mfg);
    if (!mod) {
        return NULL;
    }

    // Check whether the module is built for that particular location in flash
    if ((uintptr_t)mod->module_start_address != (uintptr_t)module_mfg.start_address) {
        return NULL;
    }

    // Validate the module
    if (verify_module(mod, &module_mfg)) {
        return NULL;
    }

    return mod;
}
#endif // MONO_MFG_FIRMWARE_AT_USER_PART
