/**
   Copyright (c) 2013-2016 Particle Industries, Inc.  All rights reserved.

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

#ifndef USER_HAL_H_
#define USER_HAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


const uint8_t* HAL_User_Image(uint32_t* size, void* reserved);


#ifdef __cplusplus
}
#endif

#endif  /* USER_HAL_H_ */

