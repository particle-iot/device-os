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

#include "lwip/ip_addr.h"

#define IPADDR_NTOA(_addr) \
        ({ \
            ::particle::detail::IpAddrNtoaHelper<IP6ADDR_STRLEN_MAX> h; \
            ipaddr_ntoa_r(_addr, h.str, sizeof(h.str)); \
            h; \
        }).str

#define IPADDR_DATA(_addr) \
        (IP_IS_V4(_addr) ? &ip_2_ip4(_addr)->addr : ip_2_ip6(_addr)->addr)

#define IPADDR_SIZE(_addr) \
        (IP_IS_V4(_addr) ? sizeof(ip4_addr::addr) : sizeof(ip6_addr::addr))

#define IP6ADDR_NTOA(_addr) \
    ({ \
        ::particle::detail::IpAddrNtoaHelper<IP6ADDR_STRLEN_MAX> tmp; \
        ip6addr_ntoa_r(_addr, tmp.str, sizeof(tmp.str)); \
        tmp; \
    }).str

#define IP4ADDR_NTOA(_addr) \
    ({ \
        ::particle::detail::IpAddrNtoaHelper<IP4ADDR_STRLEN_MAX> tmp; \
        ip4addr_ntoa_r(_addr, tmp.str, sizeof(tmp.str)); \
        tmp; \
    }).str

namespace particle {

namespace detail {

// Helper structure for the IPADDR_NTOA macro
template<size_t N>
struct IpAddrNtoaHelper {
    char str[N];
};

} // particle::detail

void reserve_netif_index();

} // particle
