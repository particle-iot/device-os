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

#include "ifapi.h"
#include "ipsockaddr.h"
#include "lwiplock.h"
#include <lwip/netif.h>
#include <lwip/netifapi.h>
#include <lwip/dhcp.h>
#include <lwip/dhcp6.h>
#include <lwip/autoip.h>
#include "logging.h"

using namespace particle::net;

namespace {

bool netif_validate(if_t iface) {
    LwipTcpIpCoreLock lk;
    netif* netif;
    NETIF_FOREACH(netif) {
        if ((if_t)netif == iface) {
            return true;
        }
    }

    return false;
}

struct EventHandlerList {
    EventHandlerList* next;
    if_event_handler_t handler;
    void* arg;
};

EventHandlerList* s_eventHandlerList = nullptr;

void notify_all_handlers_event(struct netif* netif, const struct if_event* ev) {
    for (EventHandlerList* h = s_eventHandlerList; h != nullptr; h = h->next) {
        if (h->handler) {
            h->handler(h->arg, (if_t)netif, ev);
        }
    }
}

void netif_ext_callback_handler(struct netif* netif, netif_nsc_reason_t reason, const netif_ext_callback_args_t* args) {
    if_event ev = {};
    ev.ev_len = sizeof(if_event);

    char name[IF_NAMESIZE] = {};
    if_get_name((if_t)netif, name);

    switch (reason) {
        case LWIP_NSC_NETIF_ADDED: {
            LOG(INFO, "Netif %s added", name);

            ev.ev_type = IF_EVENT_IF_ADDED;
            notify_all_handlers_event(netif, &ev);
            break;
        }
        case LWIP_NSC_NETIF_REMOVED: {
            LOG(INFO, "Netif %s deleted", name);

            ev.ev_type = IF_EVENT_IF_REMOVED;
            notify_all_handlers_event(netif, &ev);
            break;
        }
        case LWIP_NSC_LINK_CHANGED: {
            struct if_event_link_state ev_if_link = {};
            ev.ev_if_link = &ev_if_link;
            ev.ev_if_link->state = (netif->flags & NETIF_FLAG_LINK_UP) ? 1 : 0;
            LOG(INFO, "Netif %s link %s", name, ev.ev_if_link->state ? "UP" : "DOWN");
            notify_all_handlers_event(netif, &ev);
            break;
        }
        case LWIP_NSC_STATUS_CHANGED: {
            struct if_event_state ev_if_state = {};
            ev.ev_if_state = &ev_if_state;
            ev.ev_if_state->state = (netif->flags & NETIF_FLAG_UP) ? 1 : 0;
            LOG(INFO, "Netif %s state %s", name, ev.ev_if_state->state ? "UP" : "DOWN");
            notify_all_handlers_event(netif, &ev);
            break;
        }
        case LWIP_NSC_IPV4_ADDRESS_CHANGED:
        case LWIP_NSC_IPV4_NETMASK_CHANGED:
        case LWIP_NSC_IPV4_GATEWAY_CHANGED:
        case LWIP_NSC_IPV4_SETTINGS_CHANGED: {
            /* TODO */
            LOG(TRACE, "Netif %s ipv4 configuration changed", name);
            break;
        }

        case LWIP_NSC_IPV6_SET: {
            /* TODO */
            LOG(TRACE, "Netif %s ipv6 configuration changed", name);
            break;
        }

        case LWIP_NSC_IPV6_ADDR_STATE_CHANGED: {
            /* TODO */
            LOG(TRACE, "Netif %s ipv6 addr state changed", name);
            break;
        }

        default: {
            return;
        }
    }
}

} /* anonymous */

int if_init(void) {

    /* TODO */

    NETIF_DECLARE_EXT_CALLBACK(handler);
    netif_add_ext_callback(&handler, &netif_ext_callback_handler);


    return if_init_platform(nullptr);
}

__attribute__((weak)) int if_init_platform(void*) {
    return 0;
}

int if_get_list(struct if_list** ifs, void* buf, size_t* buflen) {
    if (buflen == nullptr) {
        return -1;
    }

    LwipTcpIpCoreLock lk;

    size_t len = 0;
    netif* netif;

    NETIF_FOREACH(netif) {
        len += sizeof(if_list);
    }

    if (*buflen < len || buf == nullptr || ifs == nullptr) {
        *buflen = len;
        return -1;
    }

    if_list* iface = (if_list*)buf;
    if_list* prev = nullptr;

    NETIF_FOREACH(netif) {
        iface->next = nullptr;
        iface->iface = (if_t)netif;
        if (prev) {
            prev->next = iface;
        }
        prev = iface;
        iface++;
    }

    *ifs = (if_list*)buf;

    return 0;
}

int if_name_to_index(const char* name, uint8_t* index) {
    if (!name || !index) {
        return -1;
    }

    LwipTcpIpCoreLock lk;
    uint8_t idx = netif_name_to_index(name);

    if (idx != 0) {
        *index = idx;
        return 0;
    }

    return -1;
}

int if_index_to_name(uint8_t index, char* name) {
    if (!name) {
        return -1;
    }

    LwipTcpIpCoreLock lk;
    return netif_index_to_name(index, name) ? 0 : -1;
}

int if_get_by_index(uint8_t index, if_t* iface) {
    LwipTcpIpCoreLock lk;

    netif* netif = netif_get_by_index(index);

    if (netif) {
        *iface = (if_t)netif;
        return 0;
    }

    return -1;
}

int if_get_by_name(char* name, if_t* iface) {
    LwipTcpIpCoreLock lk;
    auto netif = netif_find(name);

    if (netif != nullptr) {
        *iface = (if_t)netif;
        return 0;
    }

    return -1;
}

int if_get_flags(if_t iface, unsigned int* flags) {
    if (!flags) {
        return -1;
    }

    unsigned int iff = 0;
    {
        LwipTcpIpCoreLock lk;
        if (!netif_validate(iface)) {
            return -1;
        }

        iff = ((netif*)iface)->flags;
    }

    *flags = 0;

    if (iff & NETIF_FLAG_UP) {
        *flags |= IFF_UP;
    }

    if (iff & NETIF_FLAG_BROADCAST) {
        *flags |= IFF_BROADCAST;
    }

    if (iff & NETIF_FLAG_LINK_UP) {
        *flags |= IFF_LOWER_UP;
    }

    if (iff & NETIF_FLAG_ETHERNET) {
        *flags |= IFF_NOARP;
    }

    if (iff & (NETIF_FLAG_IGMP | NETIF_FLAG_MLD6)) {
        *flags |= IFF_MULTICAST;
    }

    if (iff & NETIF_FLAG_NO_ND6) {
        *flags |= IFF_NOND6;
    }

    uint8_t idx = 0;
    if_get_index(iface, &idx);
    if (idx == 1) {
        *flags |= IFF_LOOPBACK;
    }

    struct netif* netif = (struct netif*)iface;
    if (netif->name[0] == 'p' && netif->name[1] == 'p') {
        *flags |= IFF_POINTTOPOINT;
    }

    return 0;
}

int if_set_flags(if_t iface, unsigned int flags) {
    LwipTcpIpCoreLock lk;

    if (!netif_validate(iface)) {
        return -1;
    }

    unsigned int curFlags = 0;
    if (if_get_flags(iface, &curFlags)) {
        return -1;
    }

    curFlags &= ~(IFF_CANTCHANGE);
    flags &= ~(IFF_CANTCHANGE);

    const unsigned int changed = flags ^ curFlags;

    if (changed & IFF_UP) {
        if (curFlags & IFF_UP) {
            netif_set_up((netif*)iface);
        } else {
            netif_set_down((netif*)iface);
        }
    }

    return 0;
}

int if_get_xflags(if_t iface, unsigned int* xflags) {
    if (!xflags) {
        return -1;
    }

    LwipTcpIpCoreLock lk;

    if (!netif_validate(iface)) {
        return -1;
    }

    struct netif* netif = (struct netif*)iface;

    unsigned int ifxf = 0;

#if LWIP_IPV6
    if (netif->ip6_autoconfig_enabled) {
        ifxf |= IFXF_AUTOCONF6;
    }
#endif /* LWIP_IPV6 */

#if LWIP_IPV6_DHCP6
    if (netif_dhcp6_data(netif)) {
        ifxf |= IFXF_DHCP6;
    }
#endif /* LWIP_IPV6_DHCP6 */

#if LWIP_DHCP
    if (netif_dhcp_data(netif)) {
        ifxf |= IFXF_DHCP;
    }
#endif /* LWIP_DHCP */

#if LWIP_IPV4 && LWIP_AUTOIP
    if (netif_autoip_data(netif)) {
        ifxf |= IFXF_AUTOIP;
    }
#endif /* LWIP_IPV4 && LWIP_AUTOIP */

    /* TODO: WOL */

    *xflags = ifxf;

    return 0;
}

int if_set_xflags(if_t iface, unsigned int xflags) {
    LwipTcpIpCoreLock lk;

    if (!netif_validate(iface)) {
        return -1;
    }

    unsigned int curFlags = 0;
    if (if_get_xflags(iface, &curFlags)) {
        return -1;
    }

    curFlags &= ~(IFXF_CANTCHANGE);
    xflags &= ~(IFXF_CANTCHANGE);

    const unsigned int changed = xflags ^ curFlags;

    struct netif* netif = (struct netif*)iface;

#if LWIP_IPV6
    if (changed & IFXF_AUTOCONF6) {
        if (curFlags & IFXF_AUTOCONF6) {
            netif_set_ip6_autoconfig_enabled(netif, 0);
        } else {
            netif_set_ip6_autoconfig_enabled(netif, 1);
            netif_create_ip6_linklocal_address(netif, 1);
        }
    }
#endif /* LWIP_IPV6 */

#if LWIP_DHCP
    if (changed & IFXF_DHCP) {
        if (curFlags & IFXF_DHCP) {
            dhcp_release_and_stop(netif);
        } else {
            dhcp_start(netif);
        }
    }
#endif /* LWIP_DHCP */

#if LWIP_IPV6_DHCP6
    if (changed & IFXF_DHCP6) {
        if (curFlags & IFXF_DHCP6) {
            dhcp6_disable(netif);
        } else {
            /* FIXME */
            dhcp6_enable_stateless(netif);
        }
    }
#endif /* LWIP_IPV6_DHCP6 */

#if LWIP_IPV4 && LWIP_AUTOIP
    if (changed & IFXF_AUTOIP) {
        if (curFlags & IFXF_AUTOIP) {
            autoip_stop(netif);
        } else {
            autoip_start(netif);
        }
    }
#endif /* LWIP_IPV4 && LWIP_AUTOIP */

    /* TODO: WOL */

    return 0;
}

int if_get_index(if_t iface, uint8_t* index) {
    LwipTcpIpCoreLock lk;

    if (!index || !netif_validate(iface)) {
        return -1;
    }

    *index = netif_get_index((netif*)iface);
    return 0;
}

int if_get_name(if_t iface, char* name) {
    LwipTcpIpCoreLock lk;

    if (!name || !netif_validate(iface)) {
        return -1;
    }

    uint8_t idx = 0;
    if_get_index(iface, &idx);

    netif_index_to_name(idx, name);

    return 0;
}

int if_get_mtu(if_t iface, unsigned int* mtu) {
    LwipTcpIpCoreLock lk;

    if (!mtu || !netif_validate(iface)) {
        return -1;
    }

    struct netif* netif = (struct netif*)iface;

    *mtu = netif->mtu;

    return 0;
}

int if_set_mtu(if_t iface, unsigned int mtu) {
    LwipTcpIpCoreLock lk;

    if (!netif_validate(iface)) {
        return -1;
    }

    struct netif* netif = (struct netif*)iface;

    netif->mtu = mtu;

    return 0;
}

int if_get_metric(if_t iface, unsigned int* metric) {
    /* TODO */
    *metric = 0;
    return 0;
}

int if_set_metric(if_t iface, unsigned int metric) {
    /* TODO */
    return -1;
}

int if_get_if_addrs(struct if_addrs** addrs) {
    if (addrs == nullptr) {
        return -1;
    }

    LwipTcpIpCoreLock lk;

    if_addrs* first = nullptr;
    if_addrs* current = nullptr;

    struct netif* netif;
    NETIF_FOREACH(netif) {
        if_addrs* a = nullptr;
        int r = if_get_addrs((if_t)netif, &a);
        if (r) {
            goto cleanup;
        }

        if (!first) {
            first = a;
        }

        if (current) {
            current->next = a;
        }

        for (current = a; current->next != nullptr; current = current->next) {
            /* Empty loop */
        }
    }

cleanup:
    if_free_if_addrs(first);
    return -1;
}

int if_get_addrs(if_t iface, struct if_addrs** addrs) {
    /* TODO: refactor */
    LwipTcpIpCoreLock lk;
    if (addrs == nullptr || !netif_validate(iface)) {
        return -1;
    }

    struct netif* netif = (struct netif*)iface;

    char nametmp[NETIF_NAMESIZE] = {};
    if_get_name(iface, nametmp);

    if_addrs* first = (if_addrs*)calloc(sizeof(if_addrs) + strlen(nametmp) + 1, 1);
    if_addrs* current = first;

    if (!first) {
        return -1;
    }

    first->ifname = (char*)first + sizeof(if_addrs);
    memcpy(first->ifname, nametmp, strlen(nametmp));
    if_get_index(iface, &first->ifindex);
    if_get_flags(iface, &first->ifflags);

    if (netif->hwaddr_len) {
        const size_t alloc = sizeof(if_addr) + sizeof(sockaddr_ll);
        uint8_t* buf = (uint8_t*)calloc(alloc, 1);
        if (!buf) {
            goto cleanup;
        }

        if_addr* a = (if_addr*)buf;
        a->addr = (sockaddr*)(buf + sizeof(if_addr));
        if_get_lladdr(iface, (sockaddr_ll*)a->addr);

        current->if_addr = a;
    }

    if (!ip4_addr_isany_val(*netif_ip4_addr(netif))) {
        if (current->if_addr) {
            if_addrs* a = (if_addrs*)calloc(sizeof(if_addrs*), 1);
            if (!a) {
                goto cleanup;
            }
            memcpy(a, current, sizeof(*a));
            current->if_addr = nullptr;
            current->next = a;
            current = a;
        }

        const size_t alloc = sizeof(if_addr) + sizeof(sockaddr_in) * 3;

        uint8_t* buf = (uint8_t*)calloc(alloc, 1);
        if (!buf) {
            goto cleanup;
        }

        if_addr* a = (if_addr*)buf;
        a->addr = (sockaddr*)(buf + sizeof(if_addr));
        a->netmask = (sockaddr*)(buf + sizeof(if_addr) + sizeof(sockaddr_in));
        a->gw = (sockaddr*)(buf + sizeof(if_addr) + sizeof(sockaddr_in) * 2);

        IP4ADDR_PORT_TO_SOCKADDR((sockaddr_in*)a->addr, netif_ip4_addr(netif), 0);
        IP4ADDR_PORT_TO_SOCKADDR((sockaddr_in*)a->netmask, netif_ip4_addr(netif), 0);
        IP4ADDR_PORT_TO_SOCKADDR((sockaddr_in*)a->gw, netif_ip4_addr(netif), 0);

        current->if_addr = a;
    }

    for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        if ((netif_ip6_addr_state(netif, i) != IP6_ADDR_INVALID) ||
             !ip6_addr_isany(netif_ip6_addr(netif, i))) {

            if (current->if_addr) {
                if_addrs* a = (if_addrs*)calloc(sizeof(if_addrs*), 1);
                if (!a) {
                    goto cleanup;
                }
                memcpy(a, current, sizeof(*a));
                current->if_addr = nullptr;
                current->next = a;
                current = a;
            }

            const size_t alloc = sizeof(if_addr) + sizeof(sockaddr_in6);
            uint8_t* buf = (uint8_t*)calloc(alloc, 1);
            if (!buf) {
                goto cleanup;
            }

            if_addr* a = (if_addr*)buf;
            a->addr = (sockaddr*)(buf + sizeof(if_addr));

            IP6ADDR_PORT_TO_SOCKADDR((sockaddr_in6*)a->addr, netif_ip6_addr(netif, i), 0);
            a->prefixlen = 64;

            current->if_addr = a;
        }
    }

    *addrs = first;
    return 0;

cleanup:
    if_free_if_addrs(first);
    return -1;
}

int if_free_if_addrs(struct if_addrs* addrs) {
    for (auto a = addrs; a != nullptr;) {
        auto next = a->next;
        if (a->if_addr) {
            free(a->if_addr);
        }
        free(a);
        a = next;
    }
    return 0;
}

int if_add_addr(if_t iface, const struct if_addr* addr) {
    LwipTcpIpCoreLock lk;

    if (!addr || !addr->addr || !netif_validate(iface)) {
        return -1;
    }

    switch (addr->addr->sa_family) {
        case AF_INET: {
            if (!addr->netmask) {
                return -1;
            }
            LwipTcpIpCoreLock lk;
            ip_addr_t ip4addr = {};
            ip_addr_t netmask = {};
            ip_addr_t gw = {};
            uint16_t dummy;
            SOCKADDR4_TO_IP4ADDR_PORT((const sockaddr_in*)addr->addr, &ip4addr, dummy);
            SOCKADDR4_TO_IP4ADDR_PORT((const sockaddr_in*)addr->netmask, &netmask, dummy);
            if (addr->gw) {
                SOCKADDR4_TO_IP4ADDR_PORT((const sockaddr_in*)addr->gw, &gw, dummy);
            }
            netif_set_addr((netif*)iface, ip_2_ip4(&ip4addr), ip_2_ip4(&netmask), ip_2_ip4(&gw));
            return 0;
        }

        case AF_INET6: {
            LwipTcpIpCoreLock lk;
            ip_addr_t ip6addr = {};
            uint16_t dummy;
            /* Non-/64 prefixes are not supported. Ignoring prefixlen */
            SOCKADDR6_TO_IP6ADDR_PORT((const sockaddr_in6*)addr->addr, &ip6addr, dummy);
            err_t r = netif_add_ip6_address((netif*)iface, ip_2_ip6(&ip6addr), nullptr);
            return r == 0 ? 0 : -1;
        }

        default: {
            /* Unsupported address family */
            break;
        }
    }

    return -1;
}

int if_del_addr(if_t iface, const struct if_addr* addr) {
    LwipTcpIpCoreLock lk;

    if (!addr || !addr->addr || !netif_validate(iface)) {
        return -1;
    }

    switch (addr->addr->sa_family) {
        case AF_INET: {
            netif_set_addr((netif*)iface, nullptr, nullptr, nullptr);
            return 0;
        }

        case AF_INET6: {
            LwipTcpIpCoreLock lk;
            ip_addr_t ip6addr = {};
            uint16_t dummy;
            /* Non-/64 prefixes are not supported. Ignoring netmask */
            SOCKADDR6_TO_IP6ADDR_PORT((const sockaddr_in6*)addr->addr, &ip6addr, dummy);

            for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
                if (ip6_addr_cmp_zoneless(ip_2_ip6(&ip6addr), netif_ip6_addr((netif*)iface, i))) {
                    netif_ip6_addr_set_state((netif*)iface, i, IP6_ADDR_INVALID);
                    netif_ip6_addr_set_parts((netif*)iface, i, 0, 0, 0, 0);
                    return 0;
                }
            }
            break;
        }

        default: {
            /* Unsupported address family */
            break;
        }
    }

    return -1;
}

int if_get_lladdr(if_t iface, struct sockaddr_ll* addr) {
    LwipTcpIpCoreLock lk;

    if (!addr || !netif_validate(iface)) {
        return -1;
    }

    addr->sll_family = AF_LINK;
    addr->sll_protocol = 0x0001;
    if_get_index(iface, &addr->sll_ifindex);
    addr->sll_halen = ((netif*)iface)->hwaddr_len;
    memcpy(addr->sll_addr, ((netif*)iface)->hwaddr, ((netif*)iface)->hwaddr_len);

    return 0;
}

int if_set_lladdr(if_t iface, const struct sockaddr_ll* addr) {
    LwipTcpIpCoreLock lk;

    if (!addr || !netif_validate(iface)) {
        return -1;
    }

    if (addr->sll_family == AF_LINK && addr->sll_halen && addr->sll_halen <= NETIF_MAX_HWADDR_LEN) {
        memcpy(((netif*)iface)->hwaddr, addr->sll_addr, addr->sll_halen);
        ((netif*)iface)->hwaddr_len = addr->sll_halen;
        return 0;
    }

    return -1;
}

if_event_handler_cookie_t if_event_handler_add(if_event_handler_t handler, void* arg) {
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

int if_event_handler_del(if_event_handler_cookie_t cookie) {
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
