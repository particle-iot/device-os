/**
 ******************************************************************************
 * @file    user_dynalib.h
 * @authors Matthew McGowan
 * @date    12 February 2015
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

#ifndef USER_PREINIT_DYNALIB_H
#define	USER_PREINIT_DYNALIB_H


#include "dynalib.h"

DYNALIB_BEGIN(preinit)
DYNALIB_FN(0, preinit, module_user_pre_init, void*(void))
DYNALIB_END(preinit)


#endif	/* USER_PREINIT_DYNALIB_H */

