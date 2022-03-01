/**
 ******************************************************************************
 * @file    module_info.h
 * @authors Matthew McGowan
 * @date    17 February 2015
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

#include "static_assert.h"
#include "stddef.h"
#include <stdint.h>

typedef struct module_dependency_t {
    uint8_t module_function;        // module function, lowest 4 bits
    uint8_t module_index;           // moudle index, lowest 4 bits.
    uint16_t module_version;        // version/release number of the module.
} module_dependency_t;

typedef enum module_info_flags_t {
    MODULE_INFO_FLAG_NONE               = 0x00,
    MODULE_INFO_FLAG_DROP_MODULE_INFO   = 0x01, // Indicates that the module_info_t header preceding the actual binary
                                                // and potentially module_info_suffix_t + CRC in the end of the binary (depending on platform/module)
                                                // need to be skipped when copying/writing this module into its target location.
    MODULE_INFO_FLAG_COMPRESSED         = 0x02, // Indicates that the module data is compressed.
    MODULE_INFO_FLAG_COMBINED           = 0x04  // Indicates that this module is combined with another module.
} module_info_flags_t;

/**
 * Describes the module info struct placed at the start of
 */
typedef struct module_info_t {
    const void* module_start_address;   /* the first byte of this module in flash */
    const void* module_end_address;     /* the last byte (exclusive) of this smodule in flash. 4 byte crc starts here. */
    uint8_t reserved;                   /* Platform-specific definition (mcu_target on Gen3) */
    uint8_t flags;                      /* module_info_flags_t */
    uint16_t module_version;            /* 16 bit version */
    uint16_t platform_id;               /* The platform this module was compiled for. */
    uint8_t  module_function;           /* The module function */
    uint8_t  module_index;              /* distinguish modules of the same type */
    module_dependency_t dependency;
    module_dependency_t dependency2;
} __attribute__((__packed__)) module_info_t;

#define STATIC_ASSERT_MODULE_INFO_OFFSET(field, expected) PARTICLE_STATIC_ASSERT( module_info_##field, offsetof(module_info_t, field)==expected || sizeof(void*)!=4)

STATIC_ASSERT_MODULE_INFO_OFFSET(module_start_address, 0);
STATIC_ASSERT_MODULE_INFO_OFFSET(module_end_address, 4);
STATIC_ASSERT_MODULE_INFO_OFFSET(reserved, 8);
STATIC_ASSERT_MODULE_INFO_OFFSET(flags, 9);
STATIC_ASSERT_MODULE_INFO_OFFSET(module_version, 10);
STATIC_ASSERT_MODULE_INFO_OFFSET(platform_id, 12);
STATIC_ASSERT_MODULE_INFO_OFFSET(module_function, 14);
STATIC_ASSERT_MODULE_INFO_OFFSET(module_index, 15);
STATIC_ASSERT_MODULE_INFO_OFFSET(dependency, 16);
STATIC_ASSERT_MODULE_INFO_OFFSET(dependency2, 20);

PARTICLE_STATIC_ASSERT(module_info_size, sizeof(module_info_t) == 24 || sizeof(void*) != 4);

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
#define MOD_FUNC_SETTINGS        6
#define MOD_FUNC_NCP_FIRMWARE    7
#define MOD_FUNC_RADIO_STACK     8

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
    MODULE_FUNCTION_USER_PART = MOD_FUNC_USER_PART,

    /* Rewrite persisted settings. (Not presently used?) */
    MODULE_FUNCTION_SETTINGS = MOD_FUNC_SETTINGS,

    /* Firmware targeted for the NCP. */
    MODULE_FUNCTION_NCP_FIRMWARE = MOD_FUNC_NCP_FIRMWARE,

    /* Radio stack module */
    MODULE_FUNCTION_RADIO_STACK = MOD_FUNC_RADIO_STACK,

    /* Maximum supported value */
    MODULE_FUNCTION_MAX = MODULE_FUNCTION_RADIO_STACK

} module_function_t;

typedef enum {

    MODULE_STORE_MAIN = 0,
    /**
     * Factory restore module.
     */
    MODULE_STORE_FACTORY = 1,

    /**
     * An area that saves a copy of modules.
     */
    MODULE_STORE_BACKUP = 2,

    /**
     * Temporary area used to store the module before transferring to it's
     * target.
     */
    MODULE_STORE_SCRATCHPAD = 3,

} module_store_t;


/**
 * Fetches the module function for the given module.
 */
module_function_t  module_function(const module_info_t* mi);
module_store_t module_store(const module_info_t* mi);
uint32_t module_length(const module_info_t* mi);
uint8_t module_index(const module_info_t* mi);
uint16_t module_version(const module_info_t* mi);

uint16_t module_platform_id(const module_info_t* mi);

uint8_t module_funcion_store(module_function_t function, module_store_t store);

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
 * Compressed module header.
 *
 * In a compressed module, this header immediately follows the module info header (`module_info_t`) and
 * precedes the compressed data.
 */
typedef struct compressed_module_header {
    /**
     * Header size.
     */
    uint16_t size;
    /**
     * Compression method.
     *
     * As of now, the only supported method is raw Deflate (0).
     */
    uint8_t method;
    /**
     * Base two logarithm of the window size used when compressing this module.
     *
     * For raw Deflate the valid range is [8, 15]. The value of 0 corresponds to the default window
     * size of 15 bits.
     */
    uint8_t window_bits;
    /**
     * Size of the uncompressed data.
     */
    uint32_t original_size;
} __attribute__((__packed__)) compressed_module_header;

/*
 * The structure is a suffix to the module, placed before the end symbol
 */
typedef struct module_info_suffix_t {
    // NB: NB: NB: add new members here
    uint16_t reserved;
    uint8_t sha[32];
    uint16_t size;
    // NB: NB: NB: add new members to the start of this module definition, not the end!!
} __attribute__((packed)) module_info_suffix_t;

/**
 * The structure appended to the end of the module.
 */
typedef struct module_info_crc_t {
    uint32_t crc32;
} __attribute__((packed)) module_info_crc_t;

extern const module_info_t module_info;
extern const module_info_suffix_t module_info_suffix;
extern const module_info_crc_t module_info_crc;


#ifdef __cplusplus
}
#endif


#endif /* MODULE_INFO_H */

