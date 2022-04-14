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

#ifndef HAL_NETWORK_LWIP_IPSOCKADDR_H
#define HAL_NETWORK_LWIP_IPSOCKADDR_H

#include <lwip/sockets.h>

/* Copied these from lwip sockets.c. Why aren't these in the header? */
#define IP4ADDR_PORT_TO_SOCKADDR(sin, ipaddr, port) do { \
      (sin)->sin_len = sizeof(struct sockaddr_in); \
      (sin)->sin_family = AF_INET; \
      (sin)->sin_port = lwip_htons((port)); \
      inet_addr_from_ip4addr(&(sin)->sin_addr, ipaddr); \
      memset((sin)->sin_zero, 0, SIN_ZERO_LEN); }while(0)

#define SOCKADDR4_TO_IP4ADDR_PORT(sin, ipaddr, port) do { \
    inet_addr_to_ip4addr(ip_2_ip4(ipaddr), &((sin)->sin_addr)); \
    (port) = lwip_ntohs((sin)->sin_port); }while(0)

#define IP6ADDR_PORT_TO_SOCKADDR(sin6, ipaddr, port) do { \
      (sin6)->sin6_len = sizeof(struct sockaddr_in6); \
      (sin6)->sin6_family = AF_INET6; \
      (sin6)->sin6_port = lwip_htons((port)); \
      (sin6)->sin6_flowinfo = 0; \
      inet6_addr_from_ip6addr(&(sin6)->sin6_addr, ipaddr); \
      (sin6)->sin6_scope_id = ip6_addr_zone(ipaddr); }while(0)

#define SOCKADDR6_TO_IP6ADDR_PORT(sin6, ipaddr, port) do { \
    inet6_addr_to_ip6addr(ip_2_ip6(ipaddr), &((sin6)->sin6_addr)); \
    if (ip6_addr_has_scope(ip_2_ip6(ipaddr), IP6_UNKNOWN)) { \
      ip6_addr_set_zone(ip_2_ip6(ipaddr), (u8_t)((sin6)->sin6_scope_id)); \
    } \
    (port) = lwip_ntohs((sin6)->sin6_port); }while(0)

namespace particle { namespace net {

/* Also taken from LwIP sockets.c */
static inline void sockaddr_to_ipaddr_port(const struct sockaddr *sockaddr, ip_addr_t *ipaddr, u16_t *port)
{
  if ((sockaddr->sa_family) == AF_INET6) {
    SOCKADDR6_TO_IP6ADDR_PORT((const struct sockaddr_in6 *)(const void *)(sockaddr), ipaddr, *port);
    ipaddr->type = IPADDR_TYPE_V6;
  } else if ((sockaddr->sa_family) == AF_INET) {
    SOCKADDR4_TO_IP4ADDR_PORT((const struct sockaddr_in *)(const void *)(sockaddr), ipaddr, *port);
    ipaddr->type = IPADDR_TYPE_V4;
  }
}

static inline void ipaddr_port_to_sockaddr(const ip_addr_t* ipaddr, u16_t port, struct sockaddr* sockaddr)
{
  if (IP_IS_V4(ipaddr)) {
    IP4ADDR_PORT_TO_SOCKADDR((sockaddr_in*)sockaddr, ip_2_ip4(ipaddr), port);
  } else if (IP_IS_V6(ipaddr)) {
    IP6ADDR_PORT_TO_SOCKADDR((sockaddr_in6*)sockaddr, ip_2_ip6(ipaddr), port);
  }
}

} } /* particle::net */

#endif /* HAL_NETWORK_LWIP_IPSOCKADDR_H */
