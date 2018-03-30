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

#include <lwip/ip4.h>
#include <lwip/ip6.h>
#include "lwiphooks.h"

/* IPv4 hooks */
__attribute__((weak)) int lwip_hook_ip4_input(struct pbuf *p, struct netif *inp) {
  return 0;
}
__attribute__((weak)) int lwip_hook_ip4_input_post_validation(struct pbuf* p, struct netif* inp) {
  return 0;
}

/* IPv6 hooks */
__attribute__((weak)) int lwip_hook_ip6_input(struct pbuf* p, struct netif* inp) {
  return 0;
}
__attribute__((weak)) int lwip_hook_ip6_input_post_validation(struct pbuf* p, struct netif* inp) {
  return 0;
}
__attribute__((weak)) int lwip_hook_ip6_input_accept_multicast(struct pbuf* p, struct netif* inp, ip6_addr_t* dest) {
  return 0;
}
__attribute__((weak)) int lwip_hook_ip6_route_multicast(struct pbuf* p, struct ip6_hdr* ip6hdr, struct netif* inp) {
  return 0;
}
__attribute__((weak)) int lwip_hook_ip6_input_post_local_handling(struct pbuf* p, struct ip6_hdr* ip6hdr, struct netif* inp, u8_t proto) {
  return 0;
}
__attribute__((weak)) int lwip_hook_ip6_forward_pre_routing(struct pbuf* p, struct ip6_hdr* ip6hdr, struct netif* inp, u32_t* flags) {
  return 0;
}
__attribute__((weak)) int lwip_hook_ip6_forward_post_routing(struct pbuf* p, struct ip6_hdr* ip6hdr, struct netif* inp, struct netif* out, u32_t* flags) {
  return 0;
}

__attribute__((weak)) struct netif* lwip_hook_ip6_route(ip6_addr_t* src, ip6_addr_t* dst) {
  return NULL;
}