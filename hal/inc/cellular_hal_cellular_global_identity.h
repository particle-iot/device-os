/**
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

#ifndef CELLULAR_HAL_CELLULAR_GLOBAL_IDENTITY_H
#define CELLULAR_HAL_CELLULAR_GLOBAL_IDENTITY_H

#include <stdint.h>

typedef struct __attribute__((__packed__))
{
    uint16_t size;
    uint16_t mobile_country_code;
    uint16_t mobile_network_code;
    uint16_t location_area_code;
    uint32_t cell_id;
    uint8_t schema;
} CellularGlobalIdentity;

#endif // CELLULAR_HAL_CELLULAR_GLOBAL_IDENTITY_H
