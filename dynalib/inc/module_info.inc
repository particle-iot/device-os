/**
 ******************************************************************************
 * @file    module_info.c
 * @authors Matthew McGowan
 * @date    16-February-2015
 * @brief
 ******************************************************************************
 Copyright (c) 2013-15 Spark Labs, Inc.  All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation, either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include "module_info.h"


inline module_function_t module_function_(uint8_t f) {
    return (module_function_t)f;
}

module_function_t  module_function(const module_info_t* mi) {
    return mi ? module_function_(mi->module_function) : MODULE_FUNCTION_NONE;
}

inline uint8_t module_index_(uint8_t value) {
    return value;
}

uint8_t module_index(const module_info_t* mi) {
    return mi ? module_index_(mi->module_index) : 0xFF;
}

uint16_t module_platform_id(const module_info_t* mi) {
    return mi ? mi->platform_id : 0xFFFF;
}

module_scheme_t module_scheme(const module_info_t* mi) {
    switch (module_function(mi)) {
        case MODULE_FUNCTION_BOOTLOADER:
        case MODULE_FUNCTION_MONO_FIRMWARE:
            return MODULE_SCHEME_MONO;
    }
    return MODULE_SCHEME_SPLIT;
}

/**
 * Verifies the module platform ID matches the current system platform ID.
 * @param mi
 * @return 
 */
uint8_t module_info_matches_platform(const module_info_t* mi) {
    return module_platform_id(mi) == PLATFORM_ID;
}



/**
 * Defines the module info block that appears at the start of the module, or after the VTOR table 
 * in modules that have that.
 */

extern char link_module_start;
extern char link_module_end;

#ifndef MODULE_VERSION
    #define MODULE_VERSION 0
#endif

#ifndef MODULE_FUNCTION
#error MODULE_FUNCTION not defined
#endif

#if MODULE_FUNCTION == MOD_FUNC_BOOTLOADER || MODULE_FUNCTION == MOD_FUNC_MONO_FIRMWARE
    #if !defined(MODULE_INDEX)
        #define MODULE_INDEX 0
    #elif MODULE_INDEX != 0
        #error MODULE_INDEX must be 0 for bootloader and monolithic firmware.
    #endif
#endif

#define MODULE_DEPENDENCY1 { 0,0,0}
#define MODULE_DEPENDENCY2 { 0,0,0}

const module_info_t module_info = {
    &link_module_start,         /* start address */
    &link_module_end,           /* end address */
    MODULE_VERSION,             /* module version */
    (uint16_t)(PLATFORM_ID),    /* platform ID */
    (uint8_t)(MODULE_FUNCTION), /* module function */
    (uint8_t)(MODULE_INDEX),    /* module index (part1 part2 etc..) */        
    { MODULE_DEPENDENCY1,
    MODULE_DEPENDENCY2 }
};
