/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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
#include "asset_manager_api.h"
#endif // DYNALIB_EXPORT

DYNALIB_BEGIN(system_asset_manager)

DYNALIB_FN(0, system_asset_manager, asset_manager_set_notify_hook, int(asset_manager_notify_hook, void*, void*))
DYNALIB_FN(1, system_asset_manager, asset_manager_get_info, int(asset_manager_info*, void*))
DYNALIB_FN(2, system_asset_manager, asset_manager_free_info, void(asset_manager_info*, void*))
DYNALIB_FN(3, system_asset_manager, asset_manager_set_consumer_state, int(asset_manager_consumer_state, void*))
DYNALIB_FN(4, system_asset_manager, asset_manager_open, int(asset_manager_stream**, const asset_manager_asset*, void*))
DYNALIB_FN(5, system_asset_manager, asset_manager_available, int(asset_manager_stream*, void*))
DYNALIB_FN(6, system_asset_manager, asset_manager_read, int(asset_manager_stream*, char*, size_t, void*))
DYNALIB_FN(7, system_asset_manager, asset_manager_peek, int(asset_manager_stream*, char*, size_t, void*))
DYNALIB_FN(8, system_asset_manager, asset_manager_skip, int(asset_manager_stream*, size_t, void*))
DYNALIB_FN(9, system_asset_manager, asset_manager_seek, int(asset_manager_stream*, size_t, void*))
DYNALIB_FN(10, system_asset_manager, asset_manager_close, void(asset_manager_stream*, void*))
// UNSTABLE
DYNALIB_FN(11, system_asset_manager, asset_manager_format_storage, int(void*))
// /UNSTABLE

DYNALIB_END(system_asset_manager)
