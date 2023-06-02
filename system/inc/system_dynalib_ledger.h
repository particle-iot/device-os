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
#include "system_ledger.h"
#endif

DYNALIB_BEGIN(system_ledger)

DYNALIB_FN(0, system_ledger, ledger_get_instance, int(ledger_instance**, const char*, int, int, void*))
DYNALIB_FN(1, system_ledger, ledger_add_ref, void(ledger_instance*, void*))
DYNALIB_FN(2, system_ledger, ledger_release, void(ledger_instance*, void*))
DYNALIB_FN(3, system_ledger, ledger_lock, void(ledger_instance*, void*))
DYNALIB_FN(4, system_ledger, ledger_unlock, void(ledger_instance*, void*))
DYNALIB_FN(5, system_ledger, ledger_set_callbacks, void(ledger_instance*, const ledger_callbacks*, void*))
DYNALIB_FN(6, system_ledger, ledger_set_app_data, void(ledger_instance*, void*, ledger_destroy_app_data_callback, void*))
DYNALIB_FN(7, system_ledger, ledger_get_app_data, void*(ledger_instance*, void*))
DYNALIB_FN(8, system_ledger, ledger_get_info, int(ledger_instance*, ledger_info*, void*))
DYNALIB_FN(9, system_ledger, ledger_set_default_sync_options, int(ledger_instance*, const ledger_sync_options*, void*))
DYNALIB_FN(10, system_ledger, ledger_get_page, int(ledger_page**, ledger_instance*, const char*, void*))
DYNALIB_FN(11, system_ledger, ledger_add_page_ref, void(ledger_page*, void*))
DYNALIB_FN(12, system_ledger, ledger_release_page, void(ledger_page*, void*))
DYNALIB_FN(13, system_ledger, ledger_lock_page, void(ledger_page*, void*))
DYNALIB_FN(14, system_ledger, ledger_unlock_page, void(ledger_page*, void*))
DYNALIB_FN(15, system_ledger, ledger_get_page_ledger, ledger_instance*(ledger_page*, void*))
DYNALIB_FN(16, system_ledger, ledger_set_page_app_data, void(ledger_page*, void*, ledger_destroy_app_data_callback, void*))
DYNALIB_FN(17, system_ledger, ledger_get_page_app_data, void*(ledger_page*, void*))
DYNALIB_FN(18, system_ledger, ledger_get_page_info, int(ledger_page*, ledger_page_info*, void*))
DYNALIB_FN(19, system_ledger, ledger_sync_page, int(ledger_page*, const ledger_sync_options*, void*))
DYNALIB_FN(20, system_ledger, ledger_unlink_page, int(ledger_page*, void*))
DYNALIB_FN(21, system_ledger, ledger_remove_page, int(ledger_page*, void*))
DYNALIB_FN(22, system_ledger, ledger_open_page, int(ledger_stream**, ledger_page*, int, void*))
DYNALIB_FN(23, system_ledger, ledger_close_stream, void(ledger_stream*, void*))
DYNALIB_FN(24, system_ledger, ledger_read, int(ledger_stream*, char*, size_t, void*))
DYNALIB_FN(25, system_ledger, ledger_write, int(ledger_stream*, const char*, size_t, void*))

DYNALIB_END(system_ledger)
