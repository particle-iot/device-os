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

#ifndef NETWORK_API_IFAPI_H
#define NETWORK_API_IFAPI_H

#include <stdint.h>
#include "ifapi_impl.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef IF_T_DEFINED
typedef void* if_t;
#endif /* IF_T_DEFINED */

enum if_flags_t {
    IFF_NONE          = 0x00,
    IFF_UP            = 0x01,
    IFF_BROADCAST     = 0x02,
    IFF_DEBUG         = 0x04,
    IFF_LOOPBACK      = 0x08,
    IFF_POINTTOPOINT  = 0x10,

    IFF_RUNNING       = 0x40,
    IFF_LOWER_UP      = 0x40,

    IFF_NOARP         = 0x80,
    IFF_PROMISC       = 0x100,
    IFF_ALLMULTI      = 0x200,

    IFF_MULTICAST     = 0x8000,
    IFF_NOND6         = 0x10000,

    IFF_CANTCHANGE    = (IFF_BROADCAST | IFF_LOOPBACK | IFF_POINTTOPOINT | \
                         IFF_RUNNING   | IFF_MULTICAST)
};

enum if_xflags_t {
    IFXF_NONE         = 0x00,
    IFXF_WOL          = 0x10,
    IFXF_AUTOCONF6    = 0x20,

    IFXF_DHCP         = 0x10000,
    IFXF_DHCP6        = 0x20000,
    IFXF_AUTOIP       = 0x40000,

    IFXF_CANTCHANGE   = 0
};

#ifndef IF_NAMESIZE
#define IF_NAMESIZE (6)
#endif /* IF_NAMESIZE */

enum if_ip6_addr_state_t {
    IF_IP6_ADDR_STATE_NONE                 = 0x00,
    IF_IP6_ADDR_STATE_INVALID              = 0x00,
    IF_IP6_ADDR_STATE_TENTATIVE            = 0x01,

    IF_IP6_ADDR_STATE_VALID                = 0x02,
    IF_IP6_ADDR_STATE_DEPRECATED           = 0x02,

    IF_IP6_ADDR_STATE_PREFERRED            = 0x06,
    IF_IP6_ADDR_STATE_DUPLICATED           = 0x08
};

struct if_ip6_addr_data {
    uint8_t state;
    unsigned int valid_lifetime;
    unsigned int preferred_lifetime;
};

struct if_addr {
    /* Do not reorder. Aims to be compatible with 'struct ifaddrs' */
    struct sockaddr* addr;
    /* Unset for AF_INET6 addresses */
    struct sockaddr* netmask;
    union {
        struct sockaddr* peeraddr;
        /* XXX: Routing table should be used instead */
        struct sockaddr* gw;
    };

    union {
        void* data;
        struct if_ip6_addr_data* ip6_addr_data;
    };

    /* Will be set for both IPv4 and IPv6 */
    uint8_t prefixlen;

    unsigned int flags;
};

struct if_addrs {
    /* Do not reorder. Aims to be compatible with 'struct ifaddrs' */
    struct if_addrs* next;
    char* ifname;
    unsigned int ifflags;

    struct if_addr* if_addr;

    uint8_t ifindex;
    void* data;
};

struct if_list {
    if_list* next;
    if_t iface;
    void* data;
};

struct if_nameindex {
    /* Aims to be compatible with RFC 3493 if_nameindex */
    unsigned int if_index;
    char* if_name;
};

typedef void* if_event_handler_cookie_t;

enum if_event_type_t {
    IF_EVENT_NONE                 = 0x00,
    IF_EVENT_IF_ADDED             = 0x01,
    IF_EVENT_IF_REMOVED           = 0x02,
    IF_EVENT_STATE                = 0x03,
    IF_EVENT_LINK                 = 0x04,
    IF_EVENT_ADDR                 = 0x05,
    IF_EVENT_LLADDR               = 0x06,
    IF_EVENT_POWER_STATE          = 0x07
};

enum if_state_t {
    IF_STATE_NONE = 0x00,
    IF_STATE_DOWN = 0x01,
    IF_STATE_UP   = 0x02,
};

enum if_link_state_t {
    IF_LINK_STATE_NONE = 0x00,
    IF_LINK_STATE_DOWN = 0x01,
    IF_LINK_STATE_UP   = 0x02,
};

enum if_power_state_t {
    IF_POWER_STATE_NONE = 0x00,
    IF_POWER_STATE_DOWN = 0x01,
    IF_POWER_STATE_UP   = 0x02
};

struct if_event_state {
    uint8_t state;
};

struct if_event_link_state {
    uint8_t state;
};

struct if_event_power_state {
    uint8_t state;
};

struct if_event_addr {
    struct if_addr* oldaddr;
    struct if_addr* addr;
};

struct if_event_lladdr {
    struct sockaddr_ll* oldaddr;
    struct sockaddr_ll* addr;
};

struct if_event {
    unsigned int ev_type;
    size_t ev_len;
    union {
        void* ev_data;
        struct if_event_state* ev_if_state;
        struct if_event_link_state* ev_if_link;
        struct if_event_addr* ev_if_addr;
        struct if_event_lladdr* ev_if_lladdr;
        struct if_event_power_state* ev_power_state;
    };
};

typedef void (*if_event_handler_t)(void* arg, if_t iface, const struct if_event* ev);

typedef struct if_event_power_state if_req_power;

enum if_req_t {
    IF_REQ_NONE        = 0,
    IF_REQ_POWER_STATE = 1
};

int if_init(void);
int if_init_platform(void*);

int if_get_list(struct if_list** ifs);
int if_free_list(struct if_list* ifs);

int if_get_name_index(struct if_nameindex** ifs);
int if_free_name_index(struct if_nameindex* ifs);

int if_name_to_index(const char* name, uint8_t* index);
int if_index_to_name(uint8_t index, char* name);

int if_get_by_index(uint8_t index, if_t* iface);
int if_get_by_name(const char* name, if_t* iface);

int if_get_flags(if_t iface, unsigned int* flags);
int if_set_flags(if_t iface, unsigned int flags);
int if_clear_flags(if_t iface, unsigned int flags);

int if_get_xflags(if_t iface, unsigned int* xflags);
int if_set_xflags(if_t iface, unsigned int xflags);
int if_clear_xflags(if_t iface, unsigned int xflags);

int if_get_index(if_t iface, uint8_t* index);
int if_get_name(if_t iface, char* name);

int if_get_mtu(if_t iface, unsigned int* mtu);
int if_set_mtu(if_t iface, unsigned int mtu);

int if_get_metric(if_t iface, unsigned int* metric);
int if_set_metric(if_t iface, unsigned int metric);

int if_get_if_addrs(struct if_addrs** addrs);
int if_get_addrs(if_t iface, struct if_addrs** addrs);
int if_free_if_addrs(struct if_addrs* addrs);

int if_add_addr(if_t iface, const struct if_addr* addr);
int if_del_addr(if_t iface, const struct if_addr* addr);

int if_get_lladdr(if_t iface, struct sockaddr_ll* addr); // TODO: Add size_t addr_size?
int if_set_lladdr(if_t iface, const struct sockaddr_ll* addr); // TODO: ditto

if_event_handler_cookie_t if_event_handler_add(if_event_handler_t handler, void* arg);
if_event_handler_cookie_t if_event_handler_add_if(if_t iface, if_event_handler_t handler, void* arg);
if_event_handler_cookie_t if_event_handler_self(if_t iface, if_event_handler_t handler, void* arg);
int if_event_handler_del(if_event_handler_cookie_t cookie);

int if_request(if_t iface, int type, void* req, size_t reqsize, void* reserved);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NETWORK_API_IFAPI_H */
