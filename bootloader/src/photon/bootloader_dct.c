#include "bootloader_dct.h"

#include "module.h"

#include <string.h>

#define MIN_MODULE_VERSION_SYSTEM_PART2 SYSTEM_MODULE_VERSION_0_7_0_RC1

#define DYNALIB_INDEX_SYSTEM_MODULE_PART1 2 // system_module_part1
#define FUNC_INDEX_MODULE_SYSTEM_PART1_PRE_INIT 0 // module_system_part1_pre_init()

#define DYNALIB_INDEX_SYSTEM_MODULE_PART2 19 // system_module_part2
#define FUNC_INDEX_MODULE_SYSTEM_PART2_PRE_INIT 0 // module_system_part2_pre_init()

#define DYNALIB_INDEX_HAL_DCT 18 // hal_dct
#define FUNC_INDEX_DCT_READ_APP_DATA 0 // dct_read_app_data()
#define FUNC_INDEX_DCT_WRITE_APP_DATA 4 // dct_write_app_data()

typedef const void*(*dct_read_app_data_func_t)(uint32_t);
typedef int(*dct_write_app_data_func_t)(const void*, uint32_t, uint32_t);
typedef void*(*module_pre_init_func_t)();

static dct_read_app_data_func_t dct_read_app_data_func = NULL;
static dct_write_app_data_func_t dct_write_app_data_func = NULL;
static uint8_t dct_funcs_inited = 0;

static const module_info_t* get_module(uint8_t module_func, uint8_t module_index, uint16_t min_version) {
    const module_bounds_t* bounds = get_module_bounds(module_func, module_index);
    if (!bounds) {
        return NULL;
    }
    const module_info_t* module = get_module_info(bounds);
    if (!module || module->module_version < min_version) {
        return NULL;
    }
    if (verify_module(module, bounds) != 0) {
        return NULL;
    }
    return module;
}

static void init_dct_functions() {
    dct_read_app_data_func = NULL;
    dct_write_app_data_func = NULL;
    const module_info_t* part2 = get_module(MODULE_FUNCTION_SYSTEM_PART, 2, MIN_MODULE_VERSION_SYSTEM_PART2);
    if (!part2) {
        return;
    }
    // At the time of writing, part2 contained a complete DCT implementation. Same time, it's easy to
    // introduce an additional dependency during development, so we require part1 to be consistent as well
    const module_info_t* part1 = get_module(MODULE_FUNCTION_SYSTEM_PART, 1, part2->dependency.module_version);
    if (!part1) {
        return;
    }
    // Get addresses of the DCT functions
    dct_read_app_data_func_t dct_read = get_module_func(part2, DYNALIB_INDEX_HAL_DCT, FUNC_INDEX_DCT_READ_APP_DATA);
    dct_write_app_data_func_t dct_write = get_module_func(part2, DYNALIB_INDEX_HAL_DCT, FUNC_INDEX_DCT_WRITE_APP_DATA);
    module_pre_init_func_t part1_init = get_module_func(part1, DYNALIB_INDEX_SYSTEM_MODULE_PART1,
            FUNC_INDEX_MODULE_SYSTEM_PART1_PRE_INIT);
    module_pre_init_func_t part2_init = get_module_func(part2, DYNALIB_INDEX_SYSTEM_MODULE_PART2,
            FUNC_INDEX_MODULE_SYSTEM_PART2_PRE_INIT);
    if (!dct_read || !dct_write || !part1_init || !part2_init) {
        return;
    }
    // Initialize static data of each module
    part1_init();
    part2_init();
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
