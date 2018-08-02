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

#ifndef NETWORK_API_RESOLVAPI_H
#define NETWORK_API_RESOLVAPI_H

#include <stdint.h>
#include "resolvapi_impl.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct resolv_dns_servers {
    struct resolv_dns_servers* next;
    struct sockaddr* server;
};

int resolv_init(void);

int resolv_get_dns_servers(struct resolv_dns_servers** servers);
int resolv_free_dns_servers(struct resolv_dns_servers* servers);

int resolv_add_dns_server(const struct sockaddr* server, uint8_t priority);
int resolv_del_dns_server(const struct sockaddr* server);

typedef void* resolv_event_handler_cookie_t;
typedef void (*resolv_event_handler_t)(void* arg, const void* ptr);

resolv_event_handler_cookie_t resolv_event_handler_add(resolv_event_handler_t handler, void* arg);
int resolv_event_handler_del(resolv_event_handler_cookie_t cookie);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NETWORK_API_RESOLVAPI_H */
