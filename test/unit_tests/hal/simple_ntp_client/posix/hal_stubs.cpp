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

#include "netdb_hal.h"
#include "socket_hal_posix.h"
#include "inet_hal_posix.h"
#include "rng_hal.h"
#include "rtc_hal.h"
#include "timer_hal.h"

void netdb_freeaddrinfo(struct addrinfo* ai) {
}

int netdb_getaddrinfo(const char* hostname, const char* servname,
                      const struct addrinfo* hints, struct addrinfo** res) {
    return -1;
}

uint32_t HAL_RNG_GetRandomNumber() {
	return 0;
}

int hal_rtc_get_time(struct timeval* tv, void* reserved) {
    return 0;
}

uint64_t hal_timer_micros(void* reserved) {
    return 0;
}

int sock_setsockopt(int s, int level, int optname, const void* optval, socklen_t optlen) {
    return -1;
}

int sock_close(int s) {
    return -1;
}

int sock_connect(int s, const struct sockaddr* name, socklen_t namelen) {
    return -1;
}

ssize_t sock_recv(int s, void* mem, size_t len, int flags) {
    return -1;
}

ssize_t sock_send(int s, const void* dataptr, size_t size, int flags) {
    return -1;
}

int sock_socket(int domain, int type, int protocol) {
    return -1;
}

const char* inet_inet_ntop(int af, const void* src, char* dst, socklen_t size) {
    return nullptr;
}
