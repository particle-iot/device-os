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

#include "lwip_util.h"
#include <lwip/netif.h>
#include <lwip/netifapi.h>

namespace particle {

void reserve_netif_index() {
    struct netif iface = {};
    netifapi_netif_add(&iface, nullptr, nullptr, nullptr, nullptr, [](struct netif* iface) -> err_t {
        // Dummy
        iface->name[0] = 'd';
        iface->name[1] = 'm';
        iface->flags = 0;
        return ERR_OK;
    }, nullptr);

    netifapi_netif_remove(&iface);
}

} // particle
