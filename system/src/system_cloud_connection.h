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

#ifndef SYSTEM_CLOUD_CONNECTION_H
#define SYSTEM_CLOUD_CONNECTION_H

#include <stddef.h>
#include <stdint.h>
#include "hal_platform.h"
#include "ota_flash_hal.h"
#include "socket_hal.h"
#include <type_traits>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
    SYSTEM_CLOUD_DISCONNECT_GRACEFULLY = 1
} system_cloud_connection_flags_t;

int system_cloud_connect(int protocol, const ServerAddress* address, sockaddr* saddrCache);
int system_cloud_disconnect(int flags);
int system_cloud_send(const uint8_t* buf, size_t buflen, int flags);
int system_cloud_recv(uint8_t* buf, size_t buflen, int flags);
int system_cloud_is_connected(void* reserved);
int system_internet_test(void* reserved);
int system_multicast_announce_presence(void* reserved);
int system_cloud_set_inet_family_keepalive(int af, unsigned int value, int flags);
int system_cloud_get_inet_family_keepalive(int af, unsigned int* value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**
 * Functions for managing the cloud connection, performing cloud operations
 * and system upgrades.
 */
void spark_cloud_udp_port_set(uint16_t port);
int spark_cloud_socket_connect(void);
int spark_cloud_socket_disconnect(bool graceful=true);
uint8_t spark_cloud_socket_closed();

int Spark_Send(const unsigned char *buf, uint32_t buflen, void* reserved);
int Spark_Receive(unsigned char *buf, uint32_t buflen, void* reserved);
#if HAL_PLATFORM_CLOUD_UDP
int Spark_Send_UDP(const unsigned char* buf, uint32_t buflen, void* reserved);
int Spark_Receive_UDP(unsigned char *buf, uint32_t buflen, void* reserved);
#endif /* HAL_PLATFORM_CLOUD_UDP */

/**
 * regular async update to check that the cloud has been serviced recently.
 * After 15 seconds of inactivity, the LED status is changed to
 * @return
 */
bool system_cloud_active();

void Spark_Abort();

int Internet_Test(void);
void Multicast_Presence_Announcement(void);

namespace particle { namespace system { namespace cloud {

#if HAL_PLATFORM_CLOUD_UDP

struct SessionConnection
{
    /**
     * The previously used address.
     */
#if !HAL_USE_SOCKET_HAL_POSIX
    sockaddr_t address;
#else
    sockaddr_storage address;
#endif /* !HAL_USE_SOCKET_HAL_POSIX */

    /**
     * The checksum of the server address data that was used
     * to derive the connection address.
     */
    uint32_t server_address_checksum;

    int load(const ServerAddress& addr);
    int discard();
    int save(const ServerAddress& addr);
};
static_assert(std::is_pod<SessionConnection>::value, "SessionConnection is not a POD struct");

#endif /* HAL_PLATFORM_CLOUD_UDP */

} } } /* particle::system::cloud */

#if HAL_PLATFORM_CLOUD_UDP
extern particle::system::cloud::SessionConnection g_system_cloud_session_data;
#endif // HAL_PLATFORM_CLOUD_UDP

#endif /* SYSTEM_CLOUD_CONNECTION_H */
