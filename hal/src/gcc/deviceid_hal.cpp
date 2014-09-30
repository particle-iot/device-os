/**
 ******************************************************************************
 * @file    deviceid_hal.cpp
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    27-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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


#include "deviceid_hal.h"
#include <stddef.h>
#include <algorithm>

const char* device_id = "\x48\xff\x6b\x06\x50\x67\x55\x50\x37\x34\x13\x87";

unsigned HAL_device_ID(uint8_t* dest, unsigned destLen)
{    
    memcpy(dest, device_id, std::min(destLen, 12u));
    return 12;
}

