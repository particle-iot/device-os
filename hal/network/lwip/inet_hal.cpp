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

/**
 * @file
 * @brief
 *  This file implements POSIX-compatible inet_hal for mesh-virtual platform.
 */

/* inet_hal_posix_impl.h should get included from inet_hal.h automagically */
#include "inet_hal.h"
#include <lwip/sockets.h>

in_addr_t inet_inet_addr(const char* cp) {
  return ipaddr_addr(cp);
}

int inet_inet_aton(const char* cp, struct in_addr* pin) {
  return ip4addr_aton(cp, (ip4_addr_t*)pin);
}

in_addr_t inet_inet_network(const char* cp) {
  in_addr_t v = inet_addr(cp);
  if (v != INADDR_NONE) {
    v = lwip_ntohl(v);
  }

  return v;
}

char* inet_inet_ntoa(struct in_addr in) {
  return ip4addr_ntoa((const ip4_addr_t*)&(in));
}

char* inet_inet_ntoa_r(struct in_addr in, char *buf, socklen_t size) {
  return ip4addr_ntoa_r((const ip4_addr_t*)&(in), buf, size);
}

const char* inet_inet_ntop(int af, const void* src, char* dst, socklen_t size) {
  return lwip_inet_ntop(af, src, dst, size);
}

int inet_inet_pton(int af, const char* src, void* dst) {
  return lwip_inet_pton(af, src, dst);
}

uint32_t inet_htonl(uint32_t v) {
  return lwip_htonl(v);
}

uint32_t inet_ntohl(uint32_t v) {
  return lwip_ntohl(v);
}

uint16_t inet_htons(uint16_t v) {
  return lwip_htons(v);
}

uint16_t inet_ntohs(uint16_t v) {
  return lwip_ntohs(v);
}
