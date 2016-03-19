/**
 ******************************************************************************
 * @file    module_system_wifi_init.h
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

#ifndef MODULE_SYSTEM_PART1_INIT_DYNALIB_H
#define	MODULE_SYSTEM_PART1_INIT_DYNALIB_H

#include "dynalib.h"

/**
    Module-management functions
 */

DYNALIB_BEGIN(system_module_part1)

DYNALIB_FN(0, system_module_part1, module_system_part1_pre_init, void*(void))
DYNALIB_FN(1, system_module_part1, module_system_part1_init, void(void))

DYNALIB_END(system_module_part1)



#endif	/* MODULE_SYSTEM_WIFI_INIT_H */

