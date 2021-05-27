/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

DYNALIB_BEGIN(user_compat)

DYNALIB_FN(0, user_compat, module_user_pre_init_compat, void*(void))
DYNALIB_FN(1, user_compat, module_user_init_compat, void(void))
DYNALIB_FN(2, user_compat, module_user_setup_compat, void(void))
DYNALIB_FN(3, user_compat, module_user_loop_compat, void(void))

DYNALIB_END(user_compat)
