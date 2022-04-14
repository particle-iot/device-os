/**
 ******************************************************************************
 * @file    system-wifi.h
 * @authors Matthew McGowan
 * @date    09 February 2015
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

#ifndef SYSTEM_PART1_H
#define	SYSTEM_PART1_H

#include "dynalib.h"

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Indices into the module-level export table.
 */
#define SYSTEM_PART1MODULE_JUMP_TABLE_INDEX_COMMUNICATION 0
#define SYSTEM_PART1MODULE_JUMP_TABLE_INDEX_SERVICES 1
#define SYSTEM_PART1MODULE_JUMP_TABLE_INDEX_MODULE 2


/**
 * The static module-level export table of library jump table addresses.
 */
extern const void* const system_part1_module[];


#ifdef	__cplusplus
}
#endif

#endif	/* SYSTEM_WIFI_H */

