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
 *  This file implements POSIX-compatible netdb_hal for mesh-virtual platform.
 */

/* netdb_hal_impl.h should get included from netdb_hal.h automagically */
#include "netdb_hal.h"
#include <lwip/sockets.h>
#include <errno.h>
#include <algorithm>

struct hostent* netdb_gethostbyname(const char *name) {
    return lwip_gethostbyname(name);
}

int netdb_gethostbyname_r(const char* name, struct hostent* ret, char* buf,
                          size_t buflen, struct hostent** result, int* h_errnop) {
    return lwip_gethostbyname_r(name, ret, buf, buflen, result, h_errnop);
}

void netdb_freeaddrinfo(struct addrinfo* ai) {
    return lwip_freeaddrinfo(ai);
}

int netdb_getaddrinfo(const char* hostname, const char* servname,
                      const struct addrinfo* hints, struct addrinfo** res) {
    /* Change the behavior when AF_UNSPEC is used */
    if (hints && hints->ai_family == AF_UNSPEC) {
        struct addrinfo h = *hints;

        /* First perform a lookup with AF_INET6 */
        h.ai_family = AF_INET6;
        int rinet6 = lwip_getaddrinfo(hostname, servname, &h, res);

        /* Next perform a lookup with AF_INET */
        h.ai_family = AF_INET;
        /* FIXME: expects that there is either 1 or 0 results from the previous call */
        int rinet = lwip_getaddrinfo(hostname, servname, &h, rinet6 == 0 && *res ? &((*res)->ai_next) : res);

        if (rinet6 == 0 || rinet == 0) {
            return 0;
        }

        return std::max(rinet, rinet6);
    }
    return lwip_getaddrinfo(hostname, servname, hints, res);
}

int netdb_getnameinfo(const struct sockaddr* sa, socklen_t salen, char* host,
                      socklen_t hostlen, char* serv, socklen_t servlen, int flags) {

    if (sa == nullptr || salen <= 0 ||
        ((host == nullptr || hostlen <= 0) && (serv == nullptr || servlen <= 0))) {
        return EAI_NONAME;
    }

    /* Only numeric forms are supported */
    if ((flags & (NI_NUMERICHOST | NI_NUMERICSERV)) != (NI_NUMERICHOST | NI_NUMERICSERV)) {
        return EAI_BADFLAGS;
    }

    switch (sa->sa_family) {
        case AF_INET: {
            struct sockaddr_in* in = (struct sockaddr_in*)sa;

            if (host && hostlen > 0) {
                if (lwip_inet_ntop(AF_INET, &in->sin_addr, host, hostlen) == nullptr) {
                    return EAI_NONAME;
                }
            }

            if (serv && servlen > 0) {
                snprintf(serv, servlen, "%u", lwip_ntohs(in->sin_port));
            }
            break;
        }

        case AF_INET6: {
            struct sockaddr_in6* in6 = (struct sockaddr_in6*)sa;

            if (host && hostlen > 0) {
                if (lwip_inet_ntop(AF_INET6, &in6->sin6_addr, host, hostlen) == nullptr) {
                    return EAI_NONAME;
                }
            }

            if (serv && servlen > 0) {
                snprintf(serv, servlen, "%u", lwip_ntohs(in6->sin6_port));
            }
            break;
        }

        default: {
            return EAI_FAMILY;
        }
    }

    return 0;
}
