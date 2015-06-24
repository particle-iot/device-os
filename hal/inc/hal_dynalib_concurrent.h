/**
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

#ifndef HAL_DYNALIB_CONCURRENT_H
#define	HAL_DYNALIB_CONCURRENT_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "concurrent_hal.h"
#endif

DYNALIB_BEGIN(hal_concurrent)
DYNALIB_FN(hal_concurrent,__gthread_equal)
DYNALIB_END(hal_concurrent)



#endif	/* HAL_DYNALIB_CONCURRENT_H */

