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

#if !HAL_USE_SOCKET_HAL_POSIX && HAL_USE_SOCKET_HAL_COMPAT

#include "system_cloud_connection.h"
#include "system_cloud_internal.h"
#include "system_cloud.h"
#include "socket_hal_compat.h"
#include "deviceid_hal.h"
#include "net_hal.h"
#include "timer_hal.h"
#include "system_network.h"
#include "inet_hal_compat.h"
#include "spark_wiring_ipaddress.h"
#include "delay_hal.h"
#include "system_string_interpolate.h"
#include "endian_util.h"

namespace {

struct SystemCloudState {
    SystemCloudState()
            : socket(socket_handle_invalid()),
              protocol(-1) {
    }

    sock_handle_t socket;
    int protocol;
};

SystemCloudState s_state;

inline void ipAddrPortToSockAddr(uint32_t addr, uint16_t port, sockaddr_t& saddr) {
    saddr.sa_family = AF_INET;
    saddr.sa_data[0] = (port & 0xFF00) >> 8;
    saddr.sa_data[1] = (port & 0x00FF);

    uint16_t* sport = reinterpret_cast<uint16_t*>(saddr.sa_data);
    uint32_t* sip = reinterpret_cast<uint32_t*>(saddr.sa_data + 2);
    *sport = particle::nativeToBigEndian(port);
    *sip = particle::nativeToBigEndian(addr);
}

const unsigned CLOUD_DOMAIN_RESOLVE_ATTEMPTS = 3;

} /* anonymous */

int system_cloud_connect(int protocol, const ServerAddress* address, sockaddr* saddrCache)
{
    sockaddr_t saddr = {};

    int ret = 0;

    if (saddrCache && saddrCache->sa_family != AF_UNSPEC) {
        memcpy(&saddr, saddrCache, sizeof(saddr));
    } else if (address) {
        /* Use passed ServerAddress */
        switch (address->addr_type) {
            case IP_ADDRESS: {
                ipAddrPortToSockAddr(address->ip, address->port, saddr);
                break;
            }

            case DOMAIN_NAME: {
                char tmphost[sizeof(address->domain) + 32] = {};
                /* FIXME: this should probably be moved into system_cloud_internal */
                system_string_interpolate(address->domain, tmphost, sizeof(tmphost), system_interpolate_cloud_server_hostname);
                LOG(TRACE, "Resolving %s", tmphost);
                HAL_IPAddress haddr = {};
                for (unsigned i = 0; i < CLOUD_DOMAIN_RESOLVE_ATTEMPTS; i++) {
                    ret = inet_gethostbyname(tmphost, strnlen(tmphost, sizeof(tmphost)), &haddr, NIF_DEFAULT, nullptr);
                    if (!ret) {
                        ipAddrPortToSockAddr(haddr.ipv4, address->port, saddr);
                        break;
                    }
                    HAL_Delay_Milliseconds(1);
                }
                if (saddr.sa_family == AF_UNSPEC) {
                    LOG(ERROR, "Unable to resolve IP for %s (%d)", tmphost, ret);
                } else {
                    LOG(INFO, "Resolved %s to %s", tmphost, String(IPAddress(haddr)).c_str());
                }
                break;
            }
        }
    }

    if (saddr.sa_family == AF_UNSPEC) {
        LOG(ERROR, "Failed to determine server address");
        return SYSTEM_ERROR_NETWORK;
    }

    uint16_t dport = particle::bigEndianToNative(*((uint16_t*)saddr.sa_data));

#if PLATFORM_ID == PLATFORM_GCC
    // Use ephemeral port
    uint16_t bport = 0;
#else
    uint16_t bport = dport;
#endif // PLATFORM_ID == PLATFORM_GCC

    auto sock = socket_create(AF_INET, protocol == IPPROTO_UDP ? SOCK_DGRAM : SOCK_STREAM, protocol, bport, NIF_DEFAULT);
    if (!socket_handle_valid(sock)) {
        return SYSTEM_ERROR_NETWORK;
    }

    IPAddress waddr(saddr.sa_data + 2);

    LOG(TRACE, "Connection attempt to %s:%u", String(waddr).c_str(), dport);

    if (protocol == IPPROTO_TCP) {
        uint32_t ot = HAL_NET_SetNetWatchDog(S2M(MAX_SEC_WAIT_CONNECT));
        ret = socket_connect(sock, &saddr, sizeof(saddr));
        if (ret) {
            LOG(ERROR, "Failed to connect to %s:%u (%d)", String(waddr).c_str(), dport, ret);
        } else {
            LOG(ERROR, "Connected to %s:%u", String(waddr).c_str(), dport);
        }
        HAL_NET_SetNetWatchDog(ot);
    }

    if (ret) {
        socket_close(sock);
    } else {
        s_state.socket = sock;
        s_state.protocol = protocol;

        if (saddrCache) {
            memcpy(saddrCache, &saddr, sizeof(saddr));
        }
    }

    return ret;
}

int system_cloud_disconnect(int flags)
{
    int retVal = 0;
    if (!socket_handle_valid(s_state.socket)) {
        return retVal;
    }

    const bool graceful = flags & SYSTEM_CLOUD_DISCONNECT_GRACEFULLY;

    if (graceful && s_state.protocol == IPPROTO_TCP) {
        // Only TCP sockets can be half-closed
        retVal = socket_shutdown(s_state.socket, SHUT_WR);
        if (!retVal) {
            LOG_DEBUG(TRACE, "Half-closed cloud socket");
            if (!spark_protocol_command(sp, ProtocolCommands::DISCONNECT, 0, nullptr)) {
                // Wait for an error (which means that the server closed our connection).
                system_tick_t start = HAL_Timer_Get_Milli_Seconds();
                while (HAL_Timer_Get_Milli_Seconds() - start < 5000) {
                    if (!Spark_Communication_Loop()) {
                        break;
                    }
                }
            }
        } else {
            spark_protocol_command(sp, ProtocolCommands::DISCONNECT, 0, nullptr);
        }
    }

    LOG_DEBUG(TRACE, "Close Attempt");
    retVal = socket_close(s_state.socket);
    LOG_DEBUG(TRACE, "socket_close()=%s", (retVal ? "fail":"success"));

    if (!graceful) {
        spark_protocol_command(sp, ProtocolCommands::TERMINATE, 0, nullptr);
    }

    s_state.socket = socket_handle_invalid();

    return retVal;
}

int system_cloud_send(const uint8_t* buf, size_t buflen, int flags)
{
#if HAL_PLATFORM_CLOUD_UDP
    if (s_state.protocol == IPPROTO_UDP) {
        auto sessionAddress = g_system_cloud_session_data.address;
        return socket_sendto(s_state.socket, buf, buflen, 0, &sessionAddress, sizeof(sessionAddress));
    }
#endif // HAL_PLATFORM_CLOUD_UDP

    if (s_state.protocol == IPPROTO_TCP) {
        /* send returns negative numbers on error */
        /* Use hard-coded default 20s timeout here for now.
         * Ideally it should come from communication library
         */
        int bytesSent = socket_send_ex(s_state.socket, buf, buflen, 0, 20000, nullptr);
        return bytesSent;
    }

    return SYSTEM_ERROR_UNKNOWN;
}

int system_cloud_recv(uint8_t* buf, size_t buflen, int flags)
{
#if HAL_PLATFORM_CLOUD_UDP
    if (s_state.protocol == IPPROTO_UDP) {
        sockaddr_t addr = {};
        socklen_t size = sizeof(addr);
        auto recvd = socket_receivefrom(s_state.socket, buf, buflen, 0, &addr, &size);
        if (recvd != 0) {
            LOG_DEBUG(TRACE, "received %d", recvd);
        }

        if (recvd > 0) {
            // filter out by destination IP and port
            // todo - IPv6 will need to change this
            const auto sessionAddress = g_system_cloud_session_data.address;
            if (memcmp(&addr.sa_data, &sessionAddress.sa_data, 6)) {
    			// ignore the packet if from a different source
    			recvd = 0;
    		    LOG_DEBUG(ERROR, "received from a different address %d.%d.%d.%d:%d",
                        sessionAddress.sa_data[2],
                        sessionAddress.sa_data[3],
                        sessionAddress.sa_data[4],
                        sessionAddress.sa_data[5],
                        (((unsigned int)sessionAddress.sa_data[0] << 8) + sessionAddress.sa_data[1]));

    		}
        }

        return recvd;
    }
#endif // HAL_PLATFORM_CLOUD_UDP

    if (s_state.protocol == IPPROTO_TCP) {
        int recvd = socket_receive(s_state.socket, buf, buflen, 0);
        return recvd;
    }

    return SYSTEM_ERROR_UNKNOWN;
}

int system_internet_test(void* reserved)
{
    LOG_DEBUG(TRACE, "Internet test socket");
    auto testSocket = socket_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, 53, NIF_DEFAULT);
    LOG_DEBUG(TRACE, "socketed testSocket=%d", testSocket);

    if (!socket_handle_valid(testSocket)) {
        return SYSTEM_ERROR_UNKNOWN;
    }

    sockaddr_t saddr = {};
    // the family is always AF_INET
    saddr.sa_family = AF_INET;

    // the destination port: 53
    saddr.sa_data[0] = 0;
    saddr.sa_data[1] = 53;

    // the destination IP address: 8.8.8.8
    saddr.sa_data[2] = 8;
    saddr.sa_data[3] = 8;
    saddr.sa_data[4] = 8;
    saddr.sa_data[5] = 8;

    uint32_t ot = HAL_NET_SetNetWatchDog(S2M(MAX_SEC_WAIT_CONNECT));
    LOG_DEBUG(TRACE, "Connect Attempt");
    int testResult = socket_connect(testSocket, &saddr, sizeof(saddr));
    LOG_DEBUG(TRACE, "socket_connect()=%s", (testResult ? "fail":"success"));
    HAL_NET_SetNetWatchDog(ot);

    LOG_DEBUG(TRACE, "Close");
    socket_close(testSocket);

    // if connection fails, testResult returns negative error code
    return testResult;
}

int system_multicast_announce_presence(void* reserved)
{
#ifndef SPARK_NO_CLOUD
    auto multicastSocket = socket_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, 0, NIF_DEFAULT);
    if (!socket_handle_valid(multicastSocket)) {
        LOG_DEBUG(TRACE, "socket_handle_valid() = %d", socket_handle_valid(multicastSocket));
        return SYSTEM_ERROR_UNKNOWN;
    }

    const auto idLength = HAL_device_ID(NULL, 0);
    uint8_t id[idLength] = {};
    HAL_device_ID(id, idLength);

    // FIXME: magic number
    uint8_t announcement[idLength + 7] = {};
    auto announcementSize = spark_protocol_presence_announcement(sp, announcement, id);

    // create multicast address 224.0.1.187 port 5683
    sockaddr_t addr = {};
    addr.sa_family = AF_INET;
    addr.sa_data[0] = 0x16; // port MSB
    addr.sa_data[1] = 0x33; // port LSB
    addr.sa_data[2] = 0xe0; // IP MSB
    addr.sa_data[3] = 0x00;
    addr.sa_data[4] = 0x01;
    addr.sa_data[5] = 0xbb; // IP LSB

    //why loop here? Uncommenting this leads to SOS(HardFault Exception) on local cloud
    //for (int i = 3; i > 0; i--)
    {
        LOG_DEBUG(TRACE, "socket_sendto()");
        socket_sendto(multicastSocket, announcement, announcementSize, 0, &addr, sizeof(sockaddr_t));
    }
    LOG_DEBUG(TRACE, "socket_close(multicastSocket)");
    socket_close(multicastSocket);

#endif // SPARK_NO_CLOUD
    return 0;
}

int system_cloud_is_connected(void* reserved)
{
    bool closed = socket_active_status(s_state.socket) == SOCKET_STATUS_INACTIVE;

    if (closed) {
        LOG_DEBUG(TRACE, "socket_active_status(%d)==SOCKET_STATUS_INACTIVE", s_state.socket);
    }
    if (closed && s_state.socket != socket_handle_invalid()) {
        LOG_DEBUG(TRACE, "closed && socket(%d) != SOCKET_INVALID", s_state.socket);
    }
    if (!socket_handle_valid(s_state.socket)) {
        DEBUG("Not valid: socket_handle_valid(%d) = %d", s_state.socket, socket_handle_valid(s_state.socket));
        closed = true;
    }

    return !closed ? 0 : SYSTEM_ERROR_UNKNOWN;
}

#ifndef SPARK_NO_CLOUD
void HAL_NET_notify_socket_closed(sock_handle_t socket)
{
    if (s_state.socket == socket)
    {
        cloud_disconnect(false, false, CLOUD_DISCONNECT_REASON_ERROR);
    }
}
#else
void HAL_NET_notify_socket_closed(sock_handle_t socket)
{
}
#endif /* SPARK_NO_CLOUD */

#endif /* !HAL_USE_SOCKET_HAL_POSIX && HAL_USE_SOCKET_HAL_COMPAT */
