/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MBR_DYNALIB_H
#define	MBR_DYNALIB_H


#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "km0_km4_ipc.h"
#endif

DYNALIB_BEGIN(mbr)

DYNALIB_FN(0, mbr, km0_km4_ipc_on_request_received, int(uint8_t, km0_km4_ipc_msg_type_t, km0_km4_ipc_msg_callback_t, void*))
DYNALIB_FN(1, mbr, km0_km4_ipc_send_request, int(uint8_t, km0_km4_ipc_msg_type_t, void*, uint32_t, km0_km4_ipc_msg_callback_t, void*))
DYNALIB_FN(2, mbr, km0_km4_ipc_send_response, int(uint8_t, uint16_t, void*, uint32_t))

DYNALIB_END(mbr)



#endif	/* MBR_DYNALIB_H */

