/**
 ******************************************************************************
 * @authors Matthew McGowan
 * @date    11 February 2015
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

#ifndef MODULE_SYSTEM_PART3_INIT_H
#define	MODULE_SYSTEM_PART3_INIT_H

#include <sys/reent.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialize this module. This should erase the BSS area, copy initialized
 * variables from flash to RAM.
 * Returns a pointer to the address following the statically allocated memory.
 */
void* module_system_part3_pre_init();

/**
 * Called after the dynamic memory heap has been established. This function should
 * perform any final initialization of the module, such as calling constructors on static instances.
 */
void module_system_part3_init();

void module_system_part3_newlib_impure_set(struct _reent* r, size_t size, uint32_t version, void* ctx);

#ifdef __cplusplus
}
#endif

#endif	/* MODULE_SYSTEM_PART3_INIT_H */

