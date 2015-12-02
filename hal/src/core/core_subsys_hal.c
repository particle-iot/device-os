/**
 ******************************************************************************
 * @file    core_subsys_hal.c
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    25-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

#include "core_subsys_hal.h"
#include "nvmem.h"
#include "stdio.h"


inline void core_read_subsystem_version_impl(char* patchstr, int bufLen, unsigned char patchver[2]) {
    snprintf(patchstr, bufLen, "%d.%d", patchver[0], patchver[1]);
}

int HAL_core_subsystem_version(char* patchstr, int bufLen) {
    unsigned char patchver[2];
    int result = nvmem_read_sp_version(patchver);
    core_read_subsystem_version_impl(patchstr, bufLen, patchver);
    return result;
}


