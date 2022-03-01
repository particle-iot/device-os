/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "mesh_hal_deprecated.h"
#endif

DYNALIB_BEGIN(hal_mesh_deprecated)

DYNALIB_FN(0, hal_mesh, mesh_select_antenna_deprecated, int(int, void*))

DYNALIB_END(hal_mesh_deprecated)
