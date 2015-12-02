/**
 ******************************************************************************
 * @file    rt-dynalib.h
 * @author  Matthew McGowan
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

#pragma once

#include "dynalib.h"

DYNALIB_BEGIN(rt)
DYNALIB_FN(rt, malloc)
DYNALIB_FN(rt, free)
DYNALIB_FN(rt, realloc)
DYNALIB_FN(rt, sprintf)
DYNALIB_FN(rt, siprintf)
DYNALIB_FN_NO_MINIMAL(rt, sscanf)
DYNALIB_FN_NO_MINIMAL(rt, siscanf)
DYNALIB_FN_NO_MINIMAL(rt, snprintf)
DYNALIB_FN_NO_MINIMAL(rt, sniprintf)
DYNALIB_FN_NO_MINIMAL(rt, vsnprintf)
DYNALIB_FN_NO_MINIMAL(rt, vsniprintf)
DYNALIB_FN(rt, abort)
DYNALIB_FN(rt, _malloc_r)
DYNALIB_FN(rt, _free_r)
DYNALIB_FN(rt, _realloc_r)
DYNALIB_END(rt)
