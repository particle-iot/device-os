/**
 ******************************************************************************
 * @file    wifi_dynalib.h
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

#ifndef WIFI_DYNALIB_H
#define	WIFI_DYNALIB_H

#include "dynalib.h"

DYNALIB_BEGIN(wifi_resource)

DYNALIB_FN(0, wifi_resource, wwd_firmware_image_resource, const resource_hnd_t*(void))
DYNALIB_FN(1, wifi_resource, wwd_nvram_image_resource, const resource_hnd_t*(void))
DYNALIB_FN(2, wifi_resource, wwd_select_nvram_image_resource, int(uint8_t, void*))
DYNALIB_FN(3, wifi_resource, resource_read, resource_result_t(const resource_hnd_t*, uint32_t, uint32_t, uint32_t*, void*))
DYNALIB_END(wifi_resource)


#endif	/* WIFI_DYNALIB_H */

