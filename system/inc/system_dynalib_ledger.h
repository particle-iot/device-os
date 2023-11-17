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

#include "hal_platform.h"

#if HAL_PLATFORM_LEDGER

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "system_ledger.h"
#endif

DYNALIB_BEGIN(system_ledger)

DYNALIB_FN(0, system_ledger, ledger_get_instance, int(ledger_instance**, const char*, void*))
DYNALIB_FN(1, system_ledger, ledger_add_ref, void(ledger_instance*, void*))
DYNALIB_FN(2, system_ledger, ledger_release, void(ledger_instance*, void*))
DYNALIB_FN(3, system_ledger, ledger_lock, void(ledger_instance*, void*))
DYNALIB_FN(4, system_ledger, ledger_unlock, void(ledger_instance*, void*))
DYNALIB_FN(5, system_ledger, ledger_set_callbacks, void(ledger_instance*, const ledger_callbacks*, void*))
DYNALIB_FN(6, system_ledger, ledger_set_app_data, void(ledger_instance*, void*, ledger_destroy_app_data_callback, void*))
DYNALIB_FN(7, system_ledger, ledger_get_app_data, void*(ledger_instance*, void*))
DYNALIB_FN(8, system_ledger, ledger_get_info, int(ledger_instance*, ledger_info*, void*))
DYNALIB_FN(9, system_ledger, ledger_open, int(ledger_stream**, ledger_instance*, int, void*))
DYNALIB_FN(10, system_ledger, ledger_close, int(ledger_stream*, int, void*))
DYNALIB_FN(11, system_ledger, ledger_read, int(ledger_stream*, char*, size_t, void*))
DYNALIB_FN(12, system_ledger, ledger_write, int(ledger_stream*, const char*, size_t, void*))
DYNALIB_FN(13, system_ledger, ledger_purge, int(const char*, void*))
DYNALIB_FN(14, system_ledger, ledger_purge_all, int(void*))
DYNALIB_FN(15, system_ledger, ledger_get_names, int(char***, size_t*, void*))

DYNALIB_END(system_ledger)

#endif // HAL_PLATFORM_LEDGER
