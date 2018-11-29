/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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
#include "nfc_hal.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_nfc)

DYNALIB_FN(0, hal_nfc, hal_nfc_type2_init, int(void))
DYNALIB_FN(1, hal_nfc, hal_nfc_type2_set_payload, int(const void *msg_buf, size_t msg_len))
DYNALIB_FN(2, hal_nfc, hal_nfc_type2_start_emulation, int(void))
DYNALIB_FN(3, hal_nfc, hal_nfc_type2_stop_emulation, int(void))
DYNALIB_FN(4, hal_nfc, hal_nfc_type2_set_callback, int(nfc_event_callback_t callback))

DYNALIB_END(hal_nfc)
