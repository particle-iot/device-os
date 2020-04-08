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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <lwip/memp.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef void (*lwip_memp_event_handler_t)(memp_t type, unsigned available, unsigned size, void* ctx);

int lwip_memp_event_handler_add(lwip_memp_event_handler_t handler, memp_t type, void* ctx);

#ifdef __cplusplus
}
#endif // __cplusplus
