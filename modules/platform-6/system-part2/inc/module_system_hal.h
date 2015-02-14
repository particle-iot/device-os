/**
 ******************************************************************************
 * @file    system_hal_dynalib.h
 * @authors Matthew McGowan
 * @date    10 February 2015
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

#ifndef SYSTEM_HAL_DYNALIB_H
#define	SYSTEM_HAL_DYNALIB_H

#define SYSTEM_HAL_MODULE_JUMP_TABLE_INDEX_SERVICES 0    
#define SYSTEM_HAL_MODULE_JUMP_TABLE_INDEX_HAL 1
#define SYSTEM_HAL_MODULE_JUMP_TABLE_INDEX_RT 2

extern const void* const system_part1_module[];
    

#endif	/* SYSTEM_HAL_DYNALIB_H */

