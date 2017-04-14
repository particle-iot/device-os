#include "bootloader_dct.h"

#include <stdint.h>
#include <stddef.h>

#ifdef LOAD_DCT_FUNCTIONS

#include "flash_mal.h"

#define MODULAR_FIRMWARE 1
#include "../../../hal/src/photon/ota_module_bounds.c"

typedef const void* dct_read_app_data_func_t(uint32_t);
typedef int dct_write_app_data_func_t(const void*, uint32_t, uint32_t);

static dct_read_app_data_func_t* dct_read_app_data_func = NULL;
static dct_write_app_data_func_t* dct_write_app_data_func = NULL;

int load_dct_functions() {
    // Get module info
    const module_info_t* const module = FLASH_ModuleInfo(FLASH_INTERNAL, module_system_part2.start_address);
    if (!module || module->module_function != MODULE_FUNCTION_SYSTEM_PART || module->module_index != 2 ||
        module->platform_id != PLATFORM_ID || module->module_version < 106 /* FIXME: 0.7.0-rc.1 */) {
        return -1;
    }
    // Check module boundaries
    const uintptr_t startAddr = (uintptr_t)module->module_start_address;
    const uintptr_t endAddr = (uintptr_t)module->module_end_address;
    if (endAddr < startAddr || startAddr != module_system_part2.start_address ||
            endAddr > module_system_part2.end_address) {
        return -1;
    }
    // Verify checksum
    if (!FLASH_VerifyCRC32(FLASH_INTERNAL, startAddr, endAddr - startAddr)) {
        return -1;
    }
    // Get addresses of the DCT functions
    void* const* const dynalib = (void* const*)((const char*)module + sizeof(module_info_t));
    void* const* const dynalib_hal_core = dynalib[7];
    dct_read_app_data_func = (dct_read_app_data_func_t*)dynalib_hal_core[33];
    dct_write_app_data_func = (dct_write_app_data_func_t*)dynalib_hal_core[34];
    return 0;
}

const void* dct_read_app_data(uint32_t offset) {
    if (dct_read_app_data_func) {
        return dct_read_app_data_func(offset);
    }
    return NULL;
}

int dct_write_app_data(const void* data, uint32_t offset, uint32_t size) {
    if (dct_write_app_data_func) {
        return dct_write_app_data_func(data, offset, size);
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
