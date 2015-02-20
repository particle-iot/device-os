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

#include <stdint.h>

/**
 * Describes the module info struct placed at the start of 
 */
typedef struct module_info_t {
    const void* module_start_address;   /* the first byte of this module in flash */
    const void* module_end_address;     /* the last byte (exclusive) of this smodule in flash. 4 byte crc starts here. */
    uint32_t module_version;            /* 32 bit version */
    uint32_t reserved1;                 /* reserved - all 0 by default. */
    uint32_t reserved2;
    uint32_t reserved3;        
} module_info_t;


/**
 * The structure appended to the end of the module.
 */
typedef struct module_info_end_t {
    uint32_t crc32;     
} module_info_end_t;

extern const module_info_t module_info;


#endif	/* MODULE_INFO_H */

