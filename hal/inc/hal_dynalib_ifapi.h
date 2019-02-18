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

#ifndef HAL_DYNALIB_IFAPI_H
#define HAL_DYNALIB_IFAPI_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "ifapi.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_ifapi)

DYNALIB_FN(0, hal_ifapi, if_get_list, int(struct if_list**))
DYNALIB_FN(1, hal_ifapi, if_free_list, int(struct if_list*))
DYNALIB_FN(2, hal_ifapi, if_get_name_index, int(struct if_nameindex**))
DYNALIB_FN(3, hal_ifapi, if_free_name_index, int(struct if_nameindex*))
DYNALIB_FN(4, hal_ifapi, if_name_to_index, int(const char*, uint8_t*))
DYNALIB_FN(5, hal_ifapi, if_index_to_name, int(uint8_t, char*))
DYNALIB_FN(6, hal_ifapi, if_get_by_index, int(uint8_t, if_t*))
DYNALIB_FN(7, hal_ifapi, if_get_by_name, int(const char*, if_t*))
DYNALIB_FN(8, hal_ifapi, if_get_flags, int(if_t, unsigned int*))
DYNALIB_FN(9, hal_ifapi, if_set_flags, int(if_t, unsigned int))
DYNALIB_FN(10, hal_ifapi, if_clear_flags, int(if_t, unsigned int))
DYNALIB_FN(11, hal_ifapi, if_get_xflags, int(if_t, unsigned int*))
DYNALIB_FN(12, hal_ifapi, if_set_xflags, int(if_t, unsigned int))
DYNALIB_FN(13, hal_ifapi, if_clear_xflags, int(if_t, unsigned int))
DYNALIB_FN(14, hal_ifapi, if_get_index, int(if_t, uint8_t*))
DYNALIB_FN(15, hal_ifapi, if_get_name, int(if_t, char*))
DYNALIB_FN(16, hal_ifapi, if_get_mtu, int(if_t, unsigned int*))
DYNALIB_FN(17, hal_ifapi, if_set_mtu, int(if_t, unsigned int))
DYNALIB_FN(18, hal_ifapi, if_get_metric, int(if_t, unsigned int*))
DYNALIB_FN(19, hal_ifapi, if_set_metric, int(if_t, unsigned int))
DYNALIB_FN(20, hal_ifapi, if_get_if_addrs, int(struct if_addrs**))
DYNALIB_FN(21, hal_ifapi, if_get_addrs, int(if_t, struct if_addrs**))
DYNALIB_FN(22, hal_ifapi, if_free_if_addrs, int(struct if_addrs*))
DYNALIB_FN(23, hal_ifapi, if_add_addr, int(if_t, const struct if_addr*))
DYNALIB_FN(24, hal_ifapi, if_del_addr, int(if_t, const struct if_addr*))
DYNALIB_FN(25, hal_ifapi, if_get_lladdr, int(if_t, struct sockaddr_ll*))
DYNALIB_FN(26, hal_ifapi, if_set_lladdr, int(if_t, const struct sockaddr_ll*))
DYNALIB_FN(27, hal_ifapi, if_event_handler_add, if_event_handler_cookie_t(if_event_handler_t, void*))
DYNALIB_FN(28, hal_ifapi, if_event_handler_add_if, if_event_handler_cookie_t(if_t, if_event_handler_t, void*))
DYNALIB_FN(29, hal_ifapi, if_event_handler_self, if_event_handler_cookie_t(if_t, if_event_handler_t, void*))
DYNALIB_FN(30, hal_ifapi, if_event_handler_del, int(if_event_handler_cookie_t))
DYNALIB_FN(31, hal_ifapi, if_request, int(if_t, int, void*, size_t, void*))

DYNALIB_END(hal_ifapi)

#endif /* HAL_DYNALIB_IFAPI_H */
