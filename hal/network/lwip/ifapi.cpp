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

#include "logging.h"
LOG_SOURCE_CATEGORY("net.ifapi")

#include "ifapi.h"
#include "ipsockaddr.h"
#include "lwiplock.h"
#include <lwip/tcpip.h>
#include <lwip/netif.h>
#include <lwip/netifapi.h>
#include <lwip/dhcp.h>
extern "C" {
#include <lwip/dhcp6.h>
}
#include <lwip/autoip.h>

#include "resolvapi.h"
#include "basenetif.h"
#include "check.h"

using namespace particle::net;

namespace {

bool netif_validate(if_t iface) {
    LwipTcpIpCoreLock lk;
    netif* netif;
    NETIF_FOREACH(netif) {
        if (netif == iface) {
            return true;
        }
    }

    return false;
}

BaseNetif* getBaseNetif(if_t iface) {
    auto idx = BaseNetif::getClientDataId();
    CHECK_TRUE(idx >= 0, nullptr);
    return (BaseNetif*)netif_get_client_data(iface, idx);
}

struct EventHandlerList {
    EventHandlerList* next;
    if_t iface;
    if_event_handler_t handler;
    void* arg;
    bool self;
};

EventHandlerList* s_eventHandlerList = nullptr;
EventHandlerList* s_selfEventHandlerList = nullptr;

if_ip6_addr_state_t netif_ip6_state_to_if_ip6_addr_state(uint8_t state) {
    switch (state) {
        case IP6_ADDR_TENTATIVE:
        case IP6_ADDR_TENTATIVE_1:
        case IP6_ADDR_TENTATIVE_2:
        case IP6_ADDR_TENTATIVE_3:
        case IP6_ADDR_TENTATIVE_4:
        case IP6_ADDR_TENTATIVE_5:
        case IP6_ADDR_TENTATIVE_6:
        case IP6_ADDR_TENTATIVE_7: {
            return IF_IP6_ADDR_STATE_TENTATIVE;
        }
        case IP6_ADDR_DEPRECATED: {
            return IF_IP6_ADDR_STATE_DEPRECATED;
        }
        case IP6_ADDR_PREFERRED: {
            return IF_IP6_ADDR_STATE_PREFERRED;
        }
        case IP6_ADDR_DUPLICATED: {
            return IF_IP6_ADDR_STATE_DUPLICATED;
        }
        case IP6_ADDR_INVALID:
        default: {
            break;
        }
    }

    return IF_IP6_ADDR_STATE_INVALID;
}

void netif_ip6_address_to_if_addr(struct netif* netif, uint8_t i, struct if_addr* a) {
    ipaddr_port_to_sockaddr(netif_ip_addr6(netif, i), 0, a->addr);
    /* XXX: LwIP supports only /64 prefixes at the moment */
    if (!ip6_addr_isany(netif_ip6_addr(netif, i))) {
        a->prefixlen = 64;
    } else {
        a->prefixlen = 0;
    }

    const uint8_t state = netif_ip6_addr_state(netif, i);
    a->ip6_addr_data->state = netif_ip6_state_to_if_ip6_addr_state(state);

    a->ip6_addr_data->valid_lifetime = netif_ip6_addr_valid_life(netif, i);
    a->ip6_addr_data->preferred_lifetime = netif_ip6_addr_pref_life(netif, i);
}

unsigned int ip4_netmask_to_prefix_length(const ip4_addr_t* netmask) {
    const unsigned int mask = lwip_ntohl(ip4_addr_get_u32(netmask));
    return mask == 0 ? 0 : (32 - __builtin_ctz(mask));
}

void netif_ip4_address_to_if_addr(struct netif* netif, struct if_addr* a) {
    ipaddr_port_to_sockaddr(netif_ip_addr4(netif), 0, a->addr);
    ipaddr_port_to_sockaddr(netif_ip_netmask4(netif), 0, a->netmask);
    ipaddr_port_to_sockaddr(netif_ip_gw4(netif), 0, a->gw);

    a->prefixlen = ip4_netmask_to_prefix_length(netif_ip4_netmask(netif));
}

if_event_handler_cookie_t common_event_handler_add(if_t iface, bool self, if_event_handler_t handler, void* arg) {
    EventHandlerList** list = self ? &s_selfEventHandlerList : &s_eventHandlerList;

    /* We should really implement a list-like common class */
    EventHandlerList* e = new EventHandlerList;
    if (e) {
        LwipTcpIpCoreLock lk;
        e->handler = handler;
        e->arg = arg;
        e->next = *list;
        e->iface = iface;
        e->self = self;
        *list = e;
    }

    return e;
}

void notify_all_handlers_event_list(EventHandlerList* list, struct netif* netif, const struct if_event* ev) {
    for (EventHandlerList* h = list; h != nullptr; h = h->next) {
        if (h->handler && (h->iface == nullptr || h->iface == netif)) {
            h->handler(h->arg, netif, ev);
        }
    }
}

void notify_all_handlers_event(struct netif* netif, const struct if_event* ev) {
    if (ev->ev_type != IF_EVENT_STATE || ev->ev_if_state->state) {
        /* Execute handlers from this list first */
        notify_all_handlers_event_list(s_selfEventHandlerList, netif, ev);
    }
    notify_all_handlers_event_list(s_eventHandlerList, netif, ev);
    if (ev->ev_type == IF_EVENT_STATE && !ev->ev_if_state->state) {
        /* Execute handlers from this list last */
        notify_all_handlers_event_list(s_selfEventHandlerList, netif, ev);
    }
}

void netif_ext_callback_handler(struct netif* netif, netif_nsc_reason_t reason, const netif_ext_callback_args_t* args) {
    if_event ev = {};
    ev.ev_len = sizeof(if_event);

    char name[IF_NAMESIZE] = {};
    if_get_name(netif, name);

    if (reason & (LWIP_NSC_IPV4_ADDRESS_CHANGED |
                  LWIP_NSC_IPV4_NETMASK_CHANGED |
                  LWIP_NSC_IPV4_GATEWAY_CHANGED |
                  LWIP_NSC_IPV4_SETTINGS_CHANGED)) {
        /* Patch the reason */
        reason = LWIP_NSC_IPV4_SETTINGS_CHANGED;
    }

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
            ev.ev_type = IF_EVENT_LINK;
            ev.ev_if_link = &ev_if_link;
            ev.ev_if_link->state = args->link_changed.state;
            LOG(INFO, "Netif %s link %s", name, ev.ev_if_link->state ? "UP" : "DOWN");
            notify_all_handlers_event(netif, &ev);
            break;
        }
        case LWIP_NSC_STATUS_CHANGED: {
            struct if_event_state ev_if_state = {};
            ev.ev_type = IF_EVENT_STATE;
            ev.ev_if_state = &ev_if_state;
            ev.ev_if_state->state = args->status_changed.state;
            LOG(INFO, "Netif %s state %s", name, ev.ev_if_state->state ? "UP" : "DOWN");
            notify_all_handlers_event(netif, &ev);
            break;
        }
        case LWIP_NSC_IPV4_ADDRESS_CHANGED:
        case LWIP_NSC_IPV4_NETMASK_CHANGED:
        case LWIP_NSC_IPV4_GATEWAY_CHANGED:
        case LWIP_NSC_IPV4_SETTINGS_CHANGED: {
            LOG(TRACE, "Netif %s ipv4 configuration changed", name);
            ev.ev_type = IF_EVENT_ADDR;
            struct if_event_addr ev_if_addr = {};
            ev.ev_if_addr = &ev_if_addr;
            struct if_addr oldaddr = {};
            struct if_addr newaddr = {};
            ev_if_addr.oldaddr = &oldaddr;
            ev_if_addr.addr = &newaddr;

            struct sockaddr_in addrs[6] = {};
            oldaddr.addr = (sockaddr*)&addrs[0];
            oldaddr.netmask = (sockaddr*)&addrs[1];
            oldaddr.gw = (sockaddr*)&addrs[2];
            newaddr.addr = (sockaddr*)&addrs[3];
            newaddr.netmask = (sockaddr*)&addrs[4];
            newaddr.gw = (sockaddr*)&addrs[5];

            if (args->ipv4_changed.old_address) {
                ipaddr_port_to_sockaddr(args->ipv4_changed.old_address, 0, oldaddr.addr);
            } else {
                ipaddr_port_to_sockaddr(netif_ip_addr4(netif), 0, oldaddr.addr);
            }

            if (args->ipv4_changed.old_netmask) {
                ipaddr_port_to_sockaddr(args->ipv4_changed.old_netmask, 0, oldaddr.netmask);
                oldaddr.prefixlen = ip4_netmask_to_prefix_length(ip_2_ip4(args->ipv4_changed.old_netmask));
            } else {
                ipaddr_port_to_sockaddr(netif_ip_netmask4(netif), 0, oldaddr.netmask);
                oldaddr.prefixlen = ip4_netmask_to_prefix_length(netif_ip4_netmask(netif));
            }

            if (args->ipv4_changed.old_gw) {
                ipaddr_port_to_sockaddr(args->ipv4_changed.old_gw, 0, oldaddr.gw);
            } else {
                ipaddr_port_to_sockaddr(netif_ip_gw4(netif), 0, oldaddr.gw);
            }

            netif_ip4_address_to_if_addr(netif, &newaddr);

            notify_all_handlers_event(netif, &ev);

            break;
        }

        /* TODO: refactor these */
        case LWIP_NSC_IPV6_SET: {
            LOG(TRACE, "Netif %s ipv6 configuration changed", name);
            ev.ev_type = IF_EVENT_ADDR;
            struct if_event_addr ev_if_addr = {};
            ev.ev_if_addr = &ev_if_addr;
            struct if_addr oldaddr = {};
            struct if_addr newaddr = {};
            struct if_ip6_addr_data ip6_newaddr_data = {};
            newaddr.ip6_addr_data = &ip6_newaddr_data;
            ev_if_addr.oldaddr = &oldaddr;
            ev_if_addr.addr = &newaddr;

            struct sockaddr_in6 addrs[2] = {};
            oldaddr.addr = (sockaddr*)&addrs[0];
            newaddr.addr = (sockaddr*)&addrs[1];

            ipaddr_port_to_sockaddr(args->ipv6_set.old_address, 0, oldaddr.addr);
            if (!ip_addr_isany(args->ipv6_set.old_address)) {
                oldaddr.prefixlen = 64;
            }
            netif_ip6_address_to_if_addr(netif, args->ipv6_set.addr_index, &newaddr);
            notify_all_handlers_event(netif, &ev);
            break;
        }

        case LWIP_NSC_IPV6_ADDR_STATE_CHANGED: {
            LOG(TRACE, "Netif %s ipv6 addr state changed", name);
            ev.ev_type = IF_EVENT_ADDR;
            struct if_event_addr ev_if_addr = {};
            ev.ev_if_addr = &ev_if_addr;
            struct if_addr oldaddr = {};
            struct if_addr newaddr = {};
            struct if_ip6_addr_data ip6_oldaddr_data = {};
            struct if_ip6_addr_data ip6_newaddr_data = {};
            oldaddr.ip6_addr_data = &ip6_oldaddr_data;
            newaddr.ip6_addr_data = &ip6_newaddr_data;
            ev_if_addr.oldaddr = &oldaddr;
            ev_if_addr.addr = &newaddr;

            netif_ip6_address_to_if_addr(netif, args->ipv6_addr_state_changed.addr_index, &oldaddr);
            netif_ip6_address_to_if_addr(netif, args->ipv6_addr_state_changed.addr_index, &newaddr);
            /* Only the state differs */
            ip6_oldaddr_data.state = netif_ip6_state_to_if_ip6_addr_state(args->ipv6_addr_state_changed.old_state);
            /* Reset lifetimes because we have no idea what they were before the change */
            ip6_oldaddr_data.valid_lifetime = ip6_oldaddr_data.preferred_lifetime = 0;
            notify_all_handlers_event(netif, &ev);
            break;
        }

        default: {
            return;
        }
    }
}

} /* anonymous */

int if_init(void) {
    tcpip_init([](void* arg) {
        LOG(TRACE, "LwIP started");
    }, /* &sem */ nullptr);

    LwipTcpIpCoreLock lk;

    NETIF_DECLARE_EXT_CALLBACK(handler);
    netif_add_ext_callback(&handler, &netif_ext_callback_handler);

    resolv_init();

    return if_init_platform(nullptr);
}

__attribute__((weak)) int if_init_platform(void*) {
    return 0;
}

int if_get_list(struct if_list** ifs) {
    if (ifs == nullptr) {
        return -1;
    }

    LwipTcpIpCoreLock lk;

    size_t len = 0;
    netif* netif;

    NETIF_FOREACH(netif) {
        len += sizeof(if_list);
    }

    if (len == 0) {
        return -1;
    }

    void* buf = calloc(1, len);
    if (!buf) {
        return -1;
    }

    if_list* iface = (if_list*)buf;
    if_list* prev = nullptr;

    NETIF_FOREACH(netif) {
        iface->next = nullptr;
        iface->iface = netif;
        if (prev) {
            prev->next = iface;
        }
        prev = iface;
        iface++;
    }

    *ifs = (if_list*)buf;

    return 0;
}

int if_free_list(struct if_list* ifs) {
    if (ifs) {
        free(ifs);
        return 0;
    }

    return -1;
}

int if_get_name_index(struct if_nameindex** ifs) {
    if (ifs == nullptr) {
        return -1;
    }

    LwipTcpIpCoreLock lk;

    size_t len = 0;
    unsigned count = 0;
    netif* netif;

    NETIF_FOREACH(netif) {
        len += sizeof(if_nameindex);
        char tmp[IF_NAMESIZE] = {};
        if_get_name(netif, tmp);
        len += strlen(tmp) + 1;
        count++;
    }

    len += sizeof(if_nameindex);
    count++;

    void* buf = calloc(1, len);
    if (!buf) {
        return -1;
    }

    char* namePtr = (char*)(((if_nameindex*)buf) + count);

    if_nameindex* iface = (if_nameindex*)buf;
    NETIF_FOREACH(netif) {
        iface->if_index = netif_get_index(netif);
        iface->if_name = namePtr;
        if_get_name(netif, iface->if_name);
        namePtr += strlen(iface->if_name) + 1;
        iface++;
    }

    iface->if_index = 0;
    iface->if_name = nullptr;

    *ifs = (if_nameindex*)buf;

    return 0;
}

int if_free_name_index(struct if_nameindex* ifs) {
    if (ifs) {
        free(ifs);
        return 0;
    }

    return -1;
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
        *iface = netif;
        return 0;
    }

    return -1;
}

int if_get_by_name(const char* name, if_t* iface) {
    LwipTcpIpCoreLock lk;
    auto netif = netif_find(name);

    if (netif != nullptr) {
        *iface = netif;
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

        iff = iface->flags;
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

    /* We only care about the flags which are mentioned in `flags` */
    curFlags &= flags;

    const unsigned int changed = flags ^ curFlags;

    if (changed & IFF_UP) {
        netif_set_up(iface);
    }

    return 0;
}

int if_clear_flags(if_t iface, unsigned int flags) {
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

    /* We only care about the flags which are mentioned in `flags` */
    const unsigned int changed = curFlags & flags;

    if (changed & IFF_UP) {
        netif_set_down(iface);
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

    unsigned int flags = 0;
    if (if_get_flags(iface, &flags)) {
        return -1;
    }

    if ((flags & IFF_POINTTOPOINT) || !(flags & IFF_BROADCAST)) {
        /* Drop DHCPv4 flag */
        xflags &= ~(IFXF_DHCP);
    }

    /* We only care about the flags which are mentioned in `xflags` */
    curFlags &= xflags;

    const unsigned int changed = xflags ^ curFlags;

    struct netif* netif = (struct netif*)iface;

#if LWIP_IPV6
    if (changed & IFXF_AUTOCONF6) {
        netif_set_ip6_autoconfig_enabled(netif, 1);
        netif_create_ip6_linklocal_address(netif, 1);
    }
#endif /* LWIP_IPV6 */

#if LWIP_DHCP
    if (changed & IFXF_DHCP) {
        dhcp_start(netif);
    }
#endif /* LWIP_DHCP */

#if LWIP_IPV6_DHCP6
    if (changed & IFXF_DHCP6) {
        /* FIXME */
        dhcp6_enable_stateless(netif);
    }
#endif /* LWIP_IPV6_DHCP6 */

#if LWIP_IPV4 && LWIP_AUTOIP
    if (changed & IFXF_AUTOIP) {
        autoip_start(netif);
    }
#endif /* LWIP_IPV4 && LWIP_AUTOIP */

    /* TODO: WOL */

    return 0;
}

int if_clear_xflags(if_t iface, unsigned int xflags) {
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

    /* We only care about the flags which are mentioned in `xflags` */
    const unsigned int changed = curFlags & xflags;

    struct netif* netif = (struct netif*)iface;

#if LWIP_IPV6
    if (changed & IFXF_AUTOCONF6) {
        netif_set_ip6_autoconfig_enabled(netif, 0);
    }
#endif /* LWIP_IPV6 */

#if LWIP_DHCP
    if (changed & IFXF_DHCP) {
        dhcp_release_and_stop(netif);
    }
#endif /* LWIP_DHCP */

#if LWIP_IPV6_DHCP6
    if (changed & IFXF_DHCP6) {
        dhcp6_disable(netif);
    }
#endif /* LWIP_IPV6_DHCP6 */

#if LWIP_IPV4 && LWIP_AUTOIP
    if (changed & IFXF_AUTOIP) {
        autoip_stop(netif);
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

    *index = netif_get_index(iface);
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
        int r = if_get_addrs(netif, &a);
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

    *addrs = first;
    return 0;

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
        a->addr = (sockaddr*)((char*)buf + sizeof(if_addr));
        if_get_lladdr(iface, (sockaddr_ll*)a->addr);

        current->if_addr = a;
    }

    if (!ip4_addr_isany_val(*netif_ip4_addr(netif))) {
        if (current->if_addr) {
            if_addrs* a = (if_addrs*)calloc(sizeof(if_addrs), 1);
            if (!a) {
                goto cleanup;
            }
            memcpy(a, current, sizeof(*a));
            a->if_addr = nullptr;
            current->next = a;
            current = a;
        }

        const size_t alloc = sizeof(if_addr) + sizeof(sockaddr_in) * 3;

        uint8_t* buf = (uint8_t*)calloc(alloc, 1);
        if (!buf) {
            goto cleanup;
        }

        if_addr* a = (if_addr*)buf;
        a->addr = (sockaddr*)((char*)buf + sizeof(if_addr));
        a->netmask = (sockaddr*)((char*)buf + sizeof(if_addr) + sizeof(sockaddr_in));
        a->gw = (sockaddr*)((char*)buf + sizeof(if_addr) + sizeof(sockaddr_in) * 2);

        netif_ip4_address_to_if_addr(netif, a);

        current->if_addr = a;
    }

    for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        if ((netif_ip6_addr_state(netif, i) != IP6_ADDR_INVALID) &&
             !ip6_addr_isany(netif_ip6_addr(netif, i))) {

            if (current->if_addr) {
                if_addrs* a = (if_addrs*)calloc(sizeof(if_addrs), 1);
                if (!a) {
                    goto cleanup;
                }
                memcpy(a, current, sizeof(*a));
                a->if_addr = nullptr;
                current->next = a;
                current = a;
            }

            const size_t alloc = sizeof(if_addr) + sizeof(sockaddr_in6) + sizeof(if_ip6_addr_data);
            uint8_t* buf = (uint8_t*)calloc(alloc, 1);
            if (!buf) {
                goto cleanup;
            }

            if_addr* a = (if_addr*)buf;
            a->addr = (sockaddr*)((char*)buf + sizeof(if_addr));
            a->ip6_addr_data = (if_ip6_addr_data*)((char*)buf + sizeof(if_addr) + sizeof(sockaddr_in6));

            netif_ip6_address_to_if_addr(netif, i, a);

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
            sockaddr_to_ipaddr_port(addr->addr, &ip4addr, &dummy);
            sockaddr_to_ipaddr_port(addr->netmask, &netmask, &dummy);
            if (addr->gw) {
                sockaddr_to_ipaddr_port(addr->gw, &gw, &dummy);
            }
            netif_set_addr(iface, ip_2_ip4(&ip4addr), ip_2_ip4(&netmask), ip_2_ip4(&gw));
            return 0;
        }

        case AF_INET6: {
            LwipTcpIpCoreLock lk;
            ip_addr_t ip6addr = {};
            uint16_t dummy;
            /* Non-/64 prefixes are not supported. Ignoring prefixlen */
            sockaddr_to_ipaddr_port(addr->addr, &ip6addr, &dummy);
            err_t r = netif_add_ip6_address(iface, ip_2_ip6(&ip6addr), nullptr);
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
            netif_set_addr(iface, nullptr, nullptr, nullptr);
            return 0;
        }

        case AF_INET6: {
            LwipTcpIpCoreLock lk;
            ip_addr_t ip6addr = {};
            uint16_t dummy;
            /* Non-/64 prefixes are not supported. Ignoring netmask */
            sockaddr_to_ipaddr_port(addr->addr, &ip6addr, &dummy);

            for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
                if (ip6_addr_cmp_zoneless(ip_2_ip6(&ip6addr), netif_ip6_addr(iface, i))) {
                    netif_ip6_addr_set_state(iface, i, IP6_ADDR_INVALID);
                    netif_ip6_addr_set_parts(iface, i, 0, 0, 0, 0);
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
    addr->sll_halen = iface->hwaddr_len;
    memcpy(addr->sll_addr, iface->hwaddr, iface->hwaddr_len);

    return 0;
}

int if_set_lladdr(if_t iface, const struct sockaddr_ll* addr) {
    LwipTcpIpCoreLock lk;

    if (!addr || !netif_validate(iface)) {
        return -1;
    }

    if (addr->sll_family == AF_LINK && addr->sll_halen && addr->sll_halen <= NETIF_MAX_HWADDR_LEN) {
        memcpy(iface->hwaddr, addr->sll_addr, addr->sll_halen);
        iface->hwaddr_len = addr->sll_halen;
        return 0;
    }

    return -1;
}

if_event_handler_cookie_t if_event_handler_add(if_event_handler_t handler, void* arg) {
    return common_event_handler_add(nullptr, false, handler, arg);
}

if_event_handler_cookie_t if_event_handler_add_if(if_t iface, if_event_handler_t handler, void* arg) {
    return common_event_handler_add(iface, false, handler, arg);
}

if_event_handler_cookie_t if_event_handler_self(if_t iface, if_event_handler_t handler, void* arg) {
    return common_event_handler_add(iface, true, handler, arg);
}

int if_event_handler_del(if_event_handler_cookie_t cookie) {
    if (!cookie) {
        return -1;
    }

    EventHandlerList* e = (EventHandlerList*)cookie;
    EventHandlerList** list = e->self ? &s_selfEventHandlerList : &s_eventHandlerList;

    LwipTcpIpCoreLock lk;

    for (EventHandlerList* h = *list, *prev = nullptr; h != nullptr; h = h->next) {
        if (h == e) {
            if (prev) {
                prev->next = h->next;
            } else {
                *list = h->next;
            }

            delete e;

            return 0;
        }

        prev = h;
    }

    return -1;
}

int if_request(if_t iface, int type, void* req, size_t reqsize, void* reserved) {
    LwipTcpIpCoreLock lk;

    if (!netif_validate(iface)) {
        return -1;
    }

    switch (type) {
        case IF_REQ_POWER_STATE: {
            if (reqsize != sizeof(if_req_power)) {
                return -1;
            }

            if_req_power* preq = (if_req_power*)req;
            auto bnetif = getBaseNetif(iface);
            CHECK_TRUE(bnetif, -1);
            if (preq->state == IF_POWER_STATE_UP) {
                return bnetif->powerUp();
            } else if (preq->state == IF_POWER_STATE_DOWN) {
                return bnetif->powerDown();
            }
            break;
        }

        default: {
            return -1;
        }
    }

    return 0;
}