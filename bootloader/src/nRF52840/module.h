#pragma once

#include "module_info.h"
#include "ota_flash_hal.h"

#define SYSTEM_MODULE_VERSION_0_7_0_RC1 200 // 0.7.0-rc.1

#ifdef __cplusplus
extern "C" {
#endif

const module_bounds_t* get_module_bounds(uint8_t module_func, uint8_t module_index);
const module_info_t* get_module_info(const module_bounds_t* bounds);
int verify_module(const module_info_t* module, const module_bounds_t* bounds);
const void* get_module_func(const module_info_t* module, size_t dynalib_index, size_t func_index);

// Returns version number of the main system module
int get_main_module_version();

#ifdef __cplusplus
} // extern "C"
#endif
