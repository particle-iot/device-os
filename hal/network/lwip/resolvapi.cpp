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

#include "resolvapi.h"
#include "lwiplock.h"
#include "ipsockaddr.h"
#include <lwip/dns.h>
#include "logging.h"

using namespace particle::net;

struct EventHandlerList {
    EventHandlerList* next;
    resolv_event_handler_t handler;
    void* arg;
};

static void dns_list_change_callback_handler(u8_t numdns, const ip_addr_t *dnsserver);

EventHandlerList* s_eventHandlerList = nullptr;

int resolv_init(void) {
    LwipTcpIpCoreLock lk;

    DNS_DECLARE_LIST_CHANGE_CALLBACK(handler);
    dns_add_list_change_callback(&handler, &dns_list_change_callback_handler);

    return 0;
}

int resolv_get_dns_servers(struct resolv_dns_servers** servers) {
    if (servers == nullptr) {
        return -1;
    }

    resolv_dns_servers* first = nullptr;
    resolv_dns_servers* current = nullptr;

    LwipTcpIpCoreLock lk;

    for (int i = 0; i < DNS_MAX_SERVERS; i++) {
        const ip_addr_t* s = dns_getserver(i);
        if (!ip_addr_isany(s)) {
            uint8_t* buf = (uint8_t*)calloc(1, sizeof(resolv_dns_servers) + sizeof(sockaddr_storage));
            if (!buf) {
                goto cleanup;
            }

            resolv_dns_servers* entry = (resolv_dns_servers*)buf;
            entry->server = (struct sockaddr*)(buf + sizeof(resolv_dns_servers));
            if (IP_IS_V4(s)) {
                IP4ADDR_PORT_TO_SOCKADDR((struct sockaddr_in*)entry->server, ip_2_ip4(s), 0);
            } else {
                IP6ADDR_PORT_TO_SOCKADDR((struct sockaddr_in6*)entry->server, ip_2_ip6(s), 0);
            }

            if (!first) {
                first = entry;
            }

            if (current) {
                current->next = entry;
            }

            current = entry;
        }
    }

    *servers = first;
    return 0;

cleanup:
    resolv_free_dns_servers(first);
    return -1;
}

int resolv_free_dns_servers(struct resolv_dns_servers* servers) {
    for (auto s = servers; s != nullptr;) {
        auto next = s->next;
        free(s);
        s = next;
    }
    return 0;
}

int resolv_add_dns_server(const struct sockaddr* server, uint8_t priority) {
    /* TODO: priority */
    (void)priority;

    if (!server) {
        return -1;
    }

    ip_addr_t addr = {};
    uint16_t dummy;
    sockaddr_to_ipaddr_port(server, &addr, &dummy);

    LwipTcpIpCoreLock lk;

    for (int i = 0; i < DNS_MAX_SERVERS; i++) {
        const ip_addr_t* s = dns_getserver(i);
        if (ip_addr_isany(s)) {
            dns_setserver(i, &addr);
            return 0;
        }
    }

    return -1;
}

int resolv_del_dns_server(const struct sockaddr* server) {
    if (!server) {
        return -1;
    }

    ip_addr_t addr = {};
    uint16_t dummy;
    sockaddr_to_ipaddr_port(server, &addr, &dummy);

    LwipTcpIpCoreLock lk;

    for (int i = 0; i < DNS_MAX_SERVERS; i++) {
        const ip_addr_t* s = dns_getserver(i);
        if (!ip_addr_isany(s)) {
            if (ip_addr_cmp_zoneless(&addr, s)) {
                dns_setserver(i, nullptr);
                return 0;
            }
        }
    }

    return -1;
}

resolv_event_handler_cookie_t resolv_event_handler_add(resolv_event_handler_t handler, void* arg) {
    /* We should really implement a list-like common class */
    EventHandlerList* e = new EventHandlerList;
    if (e) {
        LwipTcpIpCoreLock lk;
        e->handler = handler;
        e->arg = arg;
        e->next = s_eventHandlerList;
        s_eventHandlerList = e;
    }

    return e;
}

int resolv_event_handler_del(resolv_event_handler_cookie_t cookie) {
    if (!cookie) {
        return -1;
    }

    EventHandlerList* e = (EventHandlerList*)cookie;

    LwipTcpIpCoreLock lk;

    for (EventHandlerList* h = s_eventHandlerList, *prev = nullptr; h != nullptr; h = h->next) {
        if (h == e) {
            if (prev) {
                prev->next = h->next;
            } else {
                s_eventHandlerList = h->next;
            }

            delete e;

            return 0;
        }

        prev = h;
    }

    return -1;
}

void dns_list_change_callback_handler(u8_t numdns, const ip_addr_t *dnsserver) {
    LOG(INFO, "DNS server list changed");
    for (EventHandlerList* h = s_eventHandlerList; h != nullptr; h = h->next) {
        if (h->handler) {
            /* FIXME */
            h->handler(h->arg, nullptr);
        }
    }
}