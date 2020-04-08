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

#include "memp_hook.h"
#include "check.h"
#include "system_error.h"
#include "lwiplock.h"
#include "lwip_util.h"
#include "lwiphooks.h"

namespace {

struct EventHandlerList {
    EventHandlerList* next;
    lwip_memp_event_handler_t handler;
    memp_t type;
    void* ctx;
};

EventHandlerList* sEventHandlerList = nullptr;

} // anonymous

typedef void (*lwip_memp_event_handler_t)(memp_t type, unsigned available, unsigned size, void* ctx);

void lwip_hook_memp_free(memp_t type, unsigned available, unsigned size) {
    for (EventHandlerList* h = sEventHandlerList; h != nullptr; h = h->next) {
        if (h->type == type && h->handler) {
            h->handler(type, available, size, h->ctx);
        }
    }
}

int lwip_memp_event_handler_add(lwip_memp_event_handler_t handler, memp_t type, void* ctx) {
    /* We should really implement a list-like common class */
    EventHandlerList* e = new EventHandlerList();
    CHECK_TRUE(e, SYSTEM_ERROR_NO_MEMORY);

    particle::net::LwipTcpIpCoreLock lk;
    e->handler = handler;
    e->type = type;
    e->ctx = ctx;
    e->next = sEventHandlerList;
    sEventHandlerList = e;

    return 0;
}
