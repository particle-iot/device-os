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

typedef enum if_flags_t {
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
} if_flags_t;

typedef enum if_xflags_t {
    IFXF_NONE         = 0x00,
    IFXF_WOL          = 0x10,
    IFXF_AUTOCONF6    = 0x20,

    IFXF_DHCP         = 0x10000,
    IFXF_DHCP6        = 0x20000,
    IFXF_AUTOIP       = 0x40000,

    IFXF_READY        = 0x100000,

    IFXF_CANTCHANGE   = (IFXF_READY)
} if_xflags_t;

#ifndef IF_NAMESIZE
#define IF_NAMESIZE (6)
#endif /* IF_NAMESIZE */

typedef enum if_ip6_addr_state_t {
    IF_IP6_ADDR_STATE_NONE                 = 0x00,
    IF_IP6_ADDR_STATE_INVALID              = 0x00,
    IF_IP6_ADDR_STATE_TENTATIVE            = 0x01,

    IF_IP6_ADDR_STATE_VALID                = 0x02,
    IF_IP6_ADDR_STATE_DEPRECATED           = 0x02,

    IF_IP6_ADDR_STATE_PREFERRED            = 0x06,
    IF_IP6_ADDR_STATE_DUPLICATED           = 0x08
 } if_ip6_addr_state_t;

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

typedef enum if_event_type_t {
    IF_EVENT_NONE                 = 0x00,
    IF_EVENT_IF_ADDED             = 0x01,
    IF_EVENT_IF_REMOVED           = 0x02,
    IF_EVENT_STATE                = 0x03,
    IF_EVENT_LINK                 = 0x04,
    IF_EVENT_ADDR                 = 0x05,
    IF_EVENT_LLADDR               = 0x06,
    IF_EVENT_POWER_STATE          = 0x07,
    IF_EVENT_PHY_STATE            = 0x08,
} if_event_type_t;

typedef enum if_state_t {
    IF_STATE_NONE = 0x00,
    IF_STATE_DOWN = 0x01,
    IF_STATE_UP   = 0x02,
} if_state_t;

typedef enum if_link_state_t {
    IF_LINK_STATE_NONE = 0x00,
    IF_LINK_STATE_DOWN = 0x01,
    IF_LINK_STATE_UP   = 0x02,
} if_link_state_t;

typedef enum if_power_state_t {
    IF_POWER_STATE_NONE = 0x00,
    IF_POWER_STATE_DOWN = 0x01,
    IF_POWER_STATE_UP   = 0x02,
    IF_POWER_STATE_POWERING_DOWN = 0x03,
    IF_POWER_STATE_POWERING_UP = 0x04,
} if_power_state_t;

typedef enum if_phy_state_t {
    IF_PHY_STATE_UNKNOWN = 0x00,
    IF_PHY_STATE_OFF = 0x01,
    IF_PHY_STATE_ON = 0x02
} if_phy_state_t;

struct if_event_state {
    uint8_t state;
};

struct if_event_link_state {
    uint8_t state;
    const char* profile;
    uint32_t profile_len;
};

struct if_event_power_state {
    uint8_t state;
};

struct if_event_phy_state {
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
        struct if_event_phy_state* ev_phy_state;
    };
};

typedef enum if_dhcp_state_t {
    IF_DHCP_STATE_OFF             = 0,
    IF_DHCP_STATE_REQUESTING      = 1,
    IF_DHCP_STATE_INIT            = 2,
    IF_DHCP_STATE_REBOOTING       = 3,
    IF_DHCP_STATE_REBINDING       = 4,
    IF_DHCP_STATE_RENEWING        = 5,
    IF_DHCP_STATE_SELECTING       = 6,
    IF_DHCP_STATE_INFORMING       = 7,
    IF_DHCP_STATE_CHECKING        = 8,
    IF_DHCP_STATE_PERMANENT       = 9,
    IF_DHCP_STATE_BOUND           = 10,
    IF_DHCP_STATE_RELEASING       = 11,
    IF_DHCP_STATE_BACKING_OFF     = 12
} if_dhcp_state_t;

struct if_req_dhcp_state {
    if_dhcp_state_t state;
    struct sockaddr_in server_ip;
    struct sockaddr_in offered_ip;
    struct sockaddr_in offered_mask;
    struct sockaddr_in offered_gw;
    uint32_t t0_lease;
    uint32_t t1_renew;
    uint32_t t2_rebind;
};

typedef struct if_req_dhcp_settings {
    struct sockaddr_in override_gw;
    uint8_t ignore_dns;
} if_req_dhcp_settings;

typedef void (*if_event_handler_t)(void* arg, if_t iface, const struct if_event* ev);

typedef struct if_event_power_state if_req_power;

typedef enum if_req_t {
    IF_REQ_NONE        = 0,
    IF_REQ_POWER_STATE = 1,
    IF_REQ_DHCP_STATE  = 2,
    IF_REQ_DRIVER_SPECIFIC = 3,
    IF_REQ_DHCP_SETTINGS = 4,
} if_req_t;

typedef struct if_req_driver_specific {
    uint32_t type;
} if_req_driver_specific;

typedef enum if_wiznet_driver_specific {
    IF_WIZNET_DRIVER_SPECIFIC_PIN_REMAP = 1,
} if_wiznet_driver_specific;

typedef struct if_wiznet_pin_remap {
    if_req_driver_specific base;
    uint16_t cs_pin;
    uint16_t reset_pin;
    uint16_t int_pin;
} if_wiznet_pin_remap;

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
void if_notify_event(if_t iface, const struct if_event* evt, void* reserved);

int if_get_power_state(if_t iface, if_power_state_t* state);

int if_get_profile(if_t iface, char* profile, size_t length);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NETWORK_API_IFAPI_H */
