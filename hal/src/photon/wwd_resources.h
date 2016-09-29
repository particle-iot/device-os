/**
 ******************************************************************************
 * @file    wiced_stubs.h
 * @authors Matthew McGowan
 * @date    10 February 2015
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

#ifndef WICED_STUBS_H
#define	WICED_STUBS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef INCLUDED_RESOURCE_H_
// resource_hnd_t not defined so let's typedef it (it's only used as a pointer here.)
typedef struct
{
} resource_hnd_t;
#endif

const resource_hnd_t* wwd_firmware_image_resource(void);

const resource_hnd_t* wwd_nvram_image_resource(void);

int wwd_select_nvram_image_resource(uint8_t res, void* reserved);

#ifdef __cplusplus
}
#endif

#endif	/* WICED_STUBS_H */
