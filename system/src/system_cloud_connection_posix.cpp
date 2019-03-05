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

#include "hal_platform.h"

#if HAL_USE_SOCKET_HAL_POSIX
#include "system_cloud_connection.h"
#include "system_cloud_internal.h"
#include "system_error.h"
#include "inet_hal.h"
#include "netdb_hal.h"
#include "system_string_interpolate.h"
#include "spark_wiring_ticks.h"
#include <arpa/inet.h>
#include "spark_wiring_cloud.h"

namespace {

enum CloudServerAddressType {
    CLOUD_SERVER_ADDRESS_TYPE_NONE            = 0,
    CLOUD_SERVER_ADDRESS_TYPE_CACHED          = 1,
    CLOUD_SERVER_ADDRESS_TYPE_CACHED_ADDRINFO = 2,
    CLOUD_SERVER_ADDRESS_TYPE_NEW_ADDRINFO    = 3
};

struct SystemCloudState {
    int socket = -1;
    struct addrinfo* addr = nullptr;
    struct addrinfo* next = nullptr;
};

SystemCloudState s_state;

const unsigned CLOUD_SOCKET_HALF_CLOSED_WAIT_TIMEOUT = 5000;

} /* anonymous */

int system_cloud_connect(int protocol, const ServerAddress* address, sockaddr* saddrCache)
{
    struct addrinfo* info = nullptr;
    CloudServerAddressType type = CLOUD_SERVER_ADDRESS_TYPE_NONE;
    bool clean = true;

    if (saddrCache && /* protocol == IPPROTO_UDP && */ saddrCache->sa_family != AF_UNSPEC) {
        char tmphost[INET6_ADDRSTRLEN] = {};
        char tmpserv[8] = {};
        if (!netdb_getnameinfo(saddrCache, saddrCache->sa_len, tmphost, sizeof(tmphost),
                               tmpserv, sizeof(tmpserv), AI_NUMERICHOST | AI_NUMERICSERV)) {
            /* There is a cached address, use it, but still pass it to getaddrinfo */
            struct addrinfo hints = {};
            hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | AI_ADDRCONFIG;
            hints.ai_family = saddrCache->sa_family;
            hints.ai_protocol = protocol;
            /* FIXME: */
            hints.ai_socktype = hints.ai_protocol == IPPROTO_UDP ? SOCK_DGRAM : SOCK_STREAM;

            if (!netdb_getaddrinfo(tmphost, tmpserv, &hints, &info)) {
                type = CLOUD_SERVER_ADDRESS_TYPE_CACHED;
            }
        }
    }

    if (type == CLOUD_SERVER_ADDRESS_TYPE_NONE) {
        /* Check if we have another address to try from the cached addrinfo list */
        if (s_state.addr && s_state.next) {
            info = s_state.next;
            type = CLOUD_SERVER_ADDRESS_TYPE_CACHED_ADDRINFO;
        }
    }

    if ((type == CLOUD_SERVER_ADDRESS_TYPE_NONE) && address) {
        /* Use passed ServerAddress */
        switch (address->addr_type) {
            case IP_ADDRESS: {
                struct addrinfo hints = {};
                hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | AI_ADDRCONFIG;
                /* XXX: IPv4-only */
                hints.ai_family = AF_INET;
                hints.ai_protocol = protocol;
                /* FIXME: */
                hints.ai_socktype = hints.ai_protocol == IPPROTO_UDP ? SOCK_DGRAM : SOCK_STREAM;

                char tmphost[INET_ADDRSTRLEN] = {};
                char tmpserv[8] = {};

                struct in_addr in = {};
                in.s_addr = htonl(address->ip);
                if (inet_inet_ntop(AF_INET, &in, tmphost, sizeof(tmphost))) {
                    snprintf(tmpserv, sizeof(tmpserv), "%u", address->port);

                    netdb_getaddrinfo(tmphost, tmpserv, &hints, &info);
                    type = CLOUD_SERVER_ADDRESS_TYPE_NEW_ADDRINFO;
                }
                break;
            }

            case DOMAIN_NAME: {
                struct addrinfo hints = {};
                hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG;
                hints.ai_protocol = protocol;
                /* FIXME: */
                hints.ai_socktype = hints.ai_protocol == IPPROTO_UDP ? SOCK_DGRAM : SOCK_STREAM;

                char tmphost[sizeof(address->domain) + 32] = {};
                char tmpserv[8] = {};
                /* FIXME: this should probably be moved into system_cloud_internal */
                system_string_interpolate(address->domain, tmphost, sizeof(tmphost), system_interpolate_cloud_server_hostname);
                snprintf(tmpserv, sizeof(tmpserv), "%u", address->port);
                LOG(TRACE, "Resolving %s#%s", tmphost, tmpserv);
                netdb_getaddrinfo(tmphost, tmpserv, &hints, &info);
                type = CLOUD_SERVER_ADDRESS_TYPE_NEW_ADDRINFO;
                break;
            }
        }
    }

    int r = SYSTEM_ERROR_NETWORK;

    if (info == nullptr) {
        LOG(ERROR, "Failed to determine server address");
    }

    LOG(TRACE, "Address type: %d", type);

    for (struct addrinfo* a = info; a != nullptr; a = a->ai_next) {
        /* Iterate over all the addresses and attempt to connect */

        int s = sock_socket(a->ai_family, a->ai_socktype, a->ai_protocol);
        if (s < 0) {
            LOG(ERROR, "Cloud socket failed, family=%d, type=%d, protocol=%d, errno=%d", a->ai_family, a->ai_socktype, a->ai_protocol, errno);
            continue;
        }

        LOG(TRACE, "Cloud socket=%d, family=%d, type=%d, protocol=%d", s, a->ai_family, a->ai_socktype, a->ai_protocol);

        char serverHost[INET6_ADDRSTRLEN] = {};
        uint16_t serverPort = 0;
        switch (a->ai_family) {
            case AF_INET: {
                inet_inet_ntop(a->ai_family, &((sockaddr_in*)a->ai_addr)->sin_addr, serverHost, sizeof(serverHost));
                serverPort = ntohs(((sockaddr_in*)a->ai_addr)->sin_port);
                break;
            }
            case AF_INET6: {
                inet_inet_ntop(a->ai_family, &((sockaddr_in6*)a->ai_addr)->sin6_addr, serverHost, sizeof(serverHost));
                serverPort = ntohs(((sockaddr_in6*)a->ai_addr)->sin6_port);
                break;
            }
        }
        LOG(INFO, "Cloud socket=%d, connecting to %s#%u", s, serverHost, serverPort);

        /* We are using fixed source port only for IPv6 connections */
        if (protocol == IPPROTO_UDP && a->ai_family == AF_INET6) {
            struct sockaddr_storage saddr = {};
            saddr.s2_len = sizeof(saddr);
            saddr.ss_family = a->ai_family;

            /* NOTE: Always binding to 5684 by default */
            switch (a->ai_family) {
                case AF_INET: {
                    ((sockaddr_in*)&saddr)->sin_port = htons(PORT_COAPS);
                    break;
                }
                case AF_INET6: {
                    ((sockaddr_in6*)&saddr)->sin6_port = htons(PORT_COAPS);
                    break;
                }
            }

            const int one = 1;
            if (sock_setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one))) {
                LOG(ERROR, "Cloud socket=%d, failed to set SO_REUSEADDR, errno=%d", s, errno);
                sock_close(s);
                continue;
            }

            /* Bind socket */
            if (sock_bind(s, (const struct sockaddr*)&saddr, sizeof(saddr))) {
                LOG(ERROR, "Cloud socket=%d, failed to bind, errno=%d");
                sock_close(s);
                continue;
            }
        }

        /* FIXME: timeout for TCP */
        /* NOTE: we do this for UDP sockets as well in order to automagically filter
         * on source address and port */
        r = sock_connect(s, a->ai_addr, a->ai_addrlen);
        if (r) {
            LOG(ERROR, "Cloud socket=%d, failed to connect to %s#%u, errno=%d", s, serverHost, serverPort, errno);
            sock_close(s);
            continue;
        }
        LOG(TRACE, "Cloud socket=%d, connected to %s#%u", s, serverHost, serverPort);

        /* If we got here, we are most likely connected, however keep track of current addrinfo list
         * in order to try the next address if application layer fails to establish the connection
         */
        if (protocol == IPPROTO_UDP &&
            (type == CLOUD_SERVER_ADDRESS_TYPE_NEW_ADDRINFO ||
             type == CLOUD_SERVER_ADDRESS_TYPE_CACHED_ADDRINFO)) {
            if (s_state.addr) {
                /* We are already iterating over a cached addrinfo list */
                s_state.next = a->ai_next;
                if (a->ai_next) {
                    s_state.next = a->ai_next;
                    clean = false;
                } else {
                    info = s_state.addr;
                    s_state.addr = s_state.next = nullptr;
                }
            } else {
                if (a->ai_next) {
                    s_state.addr = info;
                    s_state.next = a->ai_next;
                    clean = false;
                }
            }
        }

        s_state.socket = s;
        if (saddrCache) {
            memcpy(saddrCache, a->ai_addr, a->ai_addrlen);
        }

        unsigned int keepalive = 0;
        system_cloud_get_inet_family_keepalive(a->ai_family, &keepalive);
        system_cloud_set_inet_family_keepalive(a->ai_family, keepalive, 1);

        break;
    }

    if (clean) {
        netdb_freeaddrinfo(info);
    }

    return r;
}

int system_cloud_disconnect(int flags)
{
    int ret = SYSTEM_ERROR_NONE;

    if (s_state.socket < 0) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    if (flags & SYSTEM_CLOUD_DISCONNECT_GRACEFULLY) {
        /* FIXME: Only TCP sockets can be half-closed */
        ret = sock_shutdown(s_state.socket, SHUT_WR);
        if (!ret) {
            LOG_DEBUG(TRACE, "Half-closed cloud socket");
            if (!spark_protocol_command(sp, ProtocolCommands::DISCONNECT, 0, nullptr)) {
                /* Wait for an error (which means that the server closed our connection). */
                system_tick_t start = millis();
                while (millis() - start < CLOUD_SOCKET_HALF_CLOSED_WAIT_TIMEOUT) {
                    if (!spark_protocol_event_loop(system_cloud_protocol_instance())) {
                        break;
                    }
                }
            }
        } else {
            spark_protocol_command(sp, ProtocolCommands::DISCONNECT, 0, nullptr);
        }
    }

    LOG_DEBUG(TRACE, "Close Attempt");
    ret = sock_close(s_state.socket);
    LOG_DEBUG(TRACE, "sock_close()=%s", (ret ? "fail":"success"));

    if (!(flags & SYSTEM_CLOUD_DISCONNECT_GRACEFULLY)) {
        spark_protocol_command(sp, ProtocolCommands::TERMINATE, 0, nullptr);
    }

    s_state.socket = -1;

    return ret;
}

int system_cloud_send(const uint8_t* buf, size_t buflen, int flags)
{
    (void)flags;
    return sock_send(s_state.socket, buf, buflen, 0);
}

int system_cloud_recv(uint8_t* buf, size_t buflen, int flags)
{
    (void)flags;
    int recvd = sock_recv(s_state.socket, buf, buflen, MSG_DONTWAIT);
    if (recvd < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            /* Not an error */
            recvd = 0;
        }
    }

    return recvd;
}

int system_internet_test(void* reserved)
{
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int system_multicast_announce_presence(void* reserved)
{
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int system_cloud_is_connected(void* reserved)
{
    /* FIXME */
    return s_state.socket >= 0 ? 0 : -1;
}

#endif /* HAL_USE_SOCKET_HAL_POSIX */
