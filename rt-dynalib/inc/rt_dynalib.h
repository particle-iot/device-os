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

DYNALIB_FN(0, rt, malloc, void*(size_t))
DYNALIB_FN(1, rt, free, void(void*))
DYNALIB_FN(2, rt, realloc, void*(void*, size_t))
DYNALIB_FN(3, rt, sprintf, int(char*, const char*, ...))
DYNALIB_FN(4, rt, siprintf, int(char*, const char*, ...))
DYNALIB_FN(5, rt, sscanf, int(const char*, const char*, ...))
DYNALIB_FN(6, rt, siscanf, int(const char*, const char*, ...))
DYNALIB_FN(7, rt, snprintf, int(char*, size_t, const char*, ...))
DYNALIB_FN(8, rt, sniprintf, int(char*, size_t, const char*, ...))
DYNALIB_FN(9, rt, vsnprintf, int(char*, size_t, const char*, va_list))
DYNALIB_FN(10, rt, vsniprintf, int(char*, size_t, const char*, va_list))
DYNALIB_FN(11, rt, abort, void(void))
DYNALIB_FN(12, rt, _malloc_r, void*(struct _reent*, size_t))
DYNALIB_FN(13, rt, _free_r, void(struct _reent*, void*))
DYNALIB_FN(14, rt, _realloc_r, void*(struct _reent*, void*, size_t))

DYNALIB_END(rt)
