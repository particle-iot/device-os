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
#ifndef HAL_DYNALIB_RESOLVAPI_H
#define HAL_DYNALIB_RESOLVAPI_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "resolvapi.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_resolvapi)

DYNALIB_FN(0, hal_resolvapi, resolv_get_dns_servers, int(struct resolv_dns_servers**))
DYNALIB_FN(1, hal_resolvapi, resolv_free_dns_servers, int(struct resolv_dns_servers*))
DYNALIB_FN(2, hal_resolvapi, resolv_add_dns_server, int(const struct sockaddr*, uint8_t))
DYNALIB_FN(3, hal_resolvapi, resolv_del_dns_server, int(const struct sockaddr*))
DYNALIB_FN(4, hal_resolvapi, resolv_event_handler_add, resolv_event_handler_cookie_t(resolv_event_handler_t, void*))
DYNALIB_FN(5, hal_resolvapi, resolv_event_handler_del, int(resolv_event_handler_cookie_t))

DYNALIB_END(hal_resolvapi)

#endif /* HAL_DYNALIB_RESOLVAPI_H */
