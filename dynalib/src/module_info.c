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

/**
 * Defines the module info block that appears at the start of the module, or after the VTOR table 
 * in modules that have that.
 */

extern char link_module_start;
extern char link_module_end;

#ifndef MODULE_VERSION
#define MODULE_VERSION 0
#endif


const module_info_t module_info = {
    &link_module_start,         /* start address */
    &link_module_end,           /* end address */
    MODULE_VERSION,             /* module version */
    0,                          /* reserved */
    0,
    0
};


