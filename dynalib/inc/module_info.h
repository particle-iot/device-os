/**
 ******************************************************************************
 * @file    module_info.h
 * @authors Matthew McGowan
 * @date    17 February 2015
 ******************************************************************************
  Copyright (c) 2015 Spark Labs, Inc.  All rights reserved.

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

#ifndef MODULE_INFO_H
#define	MODULE_INFO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * Describes the module info struct placed at the start of 
 */
typedef struct module_info_t {
    const void* module_start_address;   /* the first byte of this module in flash */
    const void* module_end_address;     /* the last byte (exclusive) of this smodule in flash. 4 byte crc starts here. */
    uint32_t module_version;            /* 32 bit version */
    uint16_t platform_id;               /* The platform this module was compiled for. */
    uint8_t  module_function;           /* The module function */
    uint8_t  module_index;
    uint32_t reserved1;                 /* reserved - all 0 by default. */
    uint32_t reserved2;
} module_info_t;

/**
 * Define the module function enum also as preprocessor symbols so we can 
 * set some defaults in the preprocessor.
 */
#define MOD_FUNC_NONE            0
#define MOD_FUNC_RESOURCE        1
#define MOD_FUNC_BOOTLOADER      2
#define MOD_FUNC_MONO_FIRMWARE   3
#define MOD_FUNC_SYSTEM_PART     4
#define MOD_FUNC_USER_PART       5


typedef enum module_function_t {
    MODULE_FUNCTION_NONE = MOD_FUNC_NONE,
            
    /* The module_info and CRC is not part of the resource. */
    MODULE_FUNCTION_RESOURCE = MOD_FUNC_RESOURCE,      
    
    /* The module is the bootloader */        
    MODULE_FUNCTION_BOOTLOADER = MOD_FUNC_BOOTLOADER,  
    
    /* The module is complete system and user firmware */        
    MODULE_FUNCTION_MONO_FIRMWARE = MOD_FUNC_MONO_FIRMWARE,                  
    
    /* The module is a system part */        
    MODULE_FUNCTION_SYSTEM_PART = MOD_FUNC_SYSTEM_PART,
            
    /* The module is a user part */        
    MODULE_FUNCTION_USER_PART = MOD_FUNC_USER_PART                     
} module_function_t;


module_function_t  module_function(const module_info_t* mi);


uint8_t module_index(const module_info_t* mi);

uint16_t module_platform_id(const module_info_t* mi);


typedef enum module_scheme_t {
    MODULE_SCHEME_MONO,                           /* monolithic firmware */
    MODULE_SCHEME_SPLIT,                          /* modular - called it split to make the names more distinct and less confusable. */        
} module_scheme_t;

module_scheme_t module_scheme(const module_info_t* mi);

/**
 * Verifies the module platform ID matches the current system platform ID.
 * @param mi
 * @return 
 */
uint8_t module_info_matches_platform(const module_info_t* mi);


/**
 * The structure appended to the end of the module.
 */
typedef struct module_info_end_t {
    uint32_t crc32;     
} module_info_end_t;

extern const module_info_t module_info;


#ifdef __cplusplus
}
#endif


#endif	/* MODULE_INFO_H */

