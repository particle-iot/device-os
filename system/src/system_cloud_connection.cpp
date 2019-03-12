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

#include "system_cloud_connection.h"
#include "system_cloud_internal.h"
#include "system_cloud.h"
#include "core_hal.h"
#include "service_debug.h"
#include "system_task.h"
#include "spark_wiring_ticks.h"
#include "dtls_session_persist.h"

#define IPNUM(ip)       ((ip)>>24)&0xff,((ip)>>16)&0xff,((ip)>> 8)&0xff,((ip)>> 0)&0xff

namespace {

uint16_t cloud_udp_port = PORT_COAPS; // default Particle Cloud UDP port

} /* anonymous */

/* FIXME: */
extern uint8_t feature_cloud_udp;
extern uint32_t particle_key_errors;
volatile bool cloud_socket_aborted = false;

static volatile int s_ipv4_cloud_keepalive = HAL_PLATFORM_DEFAULT_CLOUD_KEEPALIVE_INTERVAL;
static volatile int s_ipv6_cloud_keepalive = HAL_PLATFORM_DEFAULT_CLOUD_KEEPALIVE_INTERVAL;

using namespace particle::system::cloud;

#if HAL_PLATFORM_CLOUD_UDP
SessionConnection g_system_cloud_session_data = {};
#endif /* HAL_PLATFORM_CLOUD_UDP */

void spark_cloud_udp_port_set(uint16_t port)
{
    cloud_udp_port = port;
}

#if HAL_PLATFORM_CLOUD_UDP

namespace {

uint32_t compute_session_checksum(const ServerAddress& addr)
{
    uint32_t checksum = HAL_Core_Compute_CRC32((uint8_t*)&addr, sizeof(addr));
    return checksum;
}

} /* anonymous */

int SessionConnection::load(const ServerAddress& addr)
{
    using particle::protocol::SessionPersistOpaque;

    SessionPersistOpaque persist;

    int r = Spark_Restore(&persist, sizeof(persist), SparkCallbacks::PERSIST_SESSION, nullptr);

    if (r == sizeof(persist) && persist.is_valid()) {
        SessionConnection* connection = (SessionConnection*)persist.connection_data();
        if (connection->server_address_checksum == compute_session_checksum(addr) &&
            connection->address.ss_family != AF_UNSPEC) {
            /* Assume valid */
            this->address = connection->address;
            LOG(INFO, "Loaded cloud server address and port from session data");
        } else {
            /* Invalidate */
            LOG(ERROR, "Address checksum %08x, expected %08x", connection->server_address_checksum, compute_session_checksum(addr));
            LOG(ERROR, "Address family %lu", connection->address.ss_family);
            discard();
            return -1;
        }
    } else {
        LOG(ERROR, "Failed to load session data from persistent storage");
        discard();
        return -1;
    }
    return -1;
}

int SessionConnection::discard()
{
    LOG(INFO, "Discarding session data");
    using particle::protocol::SessionPersistOpaque;
    SessionPersistOpaque persist;
    memset(this, 0, sizeof(*this));
    return Spark_Save(&persist, sizeof(persist), SparkCallbacks::PERSIST_SESSION, nullptr);
}

int SessionConnection::save(const ServerAddress& addr)
{
    this->server_address_checksum = compute_session_checksum(addr);
    return 0;
}

#endif /* HAL_PLATFORM_CLOUD_UDP */

// Same return value as connect(), -1 on error
int spark_cloud_socket_connect()
{
    system_cloud_disconnect(false);

#if HAL_PLATFORM_CLOUD_UDP
    const bool udp = HAL_Feature_Get(FEATURE_CLOUD_UDP);
#else
    const bool udp = false;
#endif

    uint16_t port = SPARK_SERVER_PORT;
    if (udp) {
        port = cloud_udp_port;
    }

    ServerAddress server_addr = {};
    HAL_FLASH_Read_ServerAddress(&server_addr);

    // if server address is erased, restore with a backup from system firmware
    if (server_addr.addr_type != IP_ADDRESS && server_addr.addr_type != DOMAIN_NAME) {
        LOG(WARN, "Public Server Address was blank, restoring.");
        if (udp) {
#if HAL_PLATFORM_CLOUD_UDP
            memcpy(&server_addr, backup_udp_public_server_address, backup_udp_public_server_address_size);
#endif // HAL_PLATFORM_CLOUD_UDP
        }
        else {
#if HAL_PLATFORM_CLOUD_TCP
            memcpy(&server_addr, backup_tcp_public_server_address, sizeof(backup_tcp_public_server_address));
#endif // HAL_PLATFORM_CLOUD_TCP
        }
        particle_key_errors |= SERVER_ADDRESS_BLANK;
    }
    switch (server_addr.addr_type)
    {
        case IP_ADDRESS:
            LOG(INFO,"Read Server Address = type:%d,domain:%s,ip: %d.%d.%d.%d, port: %d", server_addr.addr_type, server_addr.domain, IPNUM(server_addr.ip), server_addr.port);
            break;

        case DOMAIN_NAME:
            LOG(INFO,"Read Server Address = type:%d,domain:%s", server_addr.addr_type, server_addr.domain);
            break;

        default:
            LOG(WARN,"Read Server Address = type:%d,defaulting to device.spark.io", server_addr.addr_type);
    }

    if (server_addr.port == 0 || server_addr.port == 0xffff || (udp && port != server_addr.port)) {
        server_addr.port = port;
    }

#if HAL_PLATFORM_CLOUD_UDP
    g_system_cloud_session_data.load(server_addr);
#endif /* HAL_PLATFORM_CLOUD_UDP */

    int r = system_cloud_connect(udp ? IPPROTO_UDP : IPPROTO_TCP, &server_addr,
#if HAL_PLATFORM_CLOUD_UDP
                                 (sockaddr*)&g_system_cloud_session_data.address);
#else
                                 nullptr);
#endif /* HAL_PLATFORM_CLOUD_UDP */

#if HAL_PLATFORM_CLOUD_UDP
    if (!r) {
        /* This does not actually save anyhing to persistent storage, just computes the checksum */
        g_system_cloud_session_data.save(server_addr);
    }
#endif /* HAL_PLATFORM_CLOUD_UDP */

    return r;
}

int spark_cloud_socket_disconnect(bool graceful)
{
    return system_cloud_disconnect(graceful ? SYSTEM_CLOUD_DISCONNECT_GRACEFULLY : 0);
}

uint8_t spark_cloud_socket_closed()
{
    return system_cloud_is_connected(nullptr);
}

void Spark_Abort() {
#ifndef SPARK_NO_CLOUD
    cloud_socket_aborted = true;
#endif
}

#if HAL_PLATFORM_CLOUD_UDP

int Spark_Send_UDP(const unsigned char* buf, uint32_t buflen, void* reserved)
{
    if (SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || spark_cloud_socket_closed() || cloud_socket_aborted)
    {
        LOG(TRACE, "SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || spark_cloud_socket_closed() || cloud_socket_aborted");
        //break from any blocking loop
        return -1;
    }

    return system_cloud_send(buf, buflen, 0);
}

int Spark_Receive_UDP(unsigned char *buf, uint32_t buflen, void* reserved)
{
    if (SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || spark_cloud_socket_closed() || cloud_socket_aborted)
    {
        //break from any blocking loop
        LOG(TRACE, "SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || spark_cloud_socket_closed() || cloud_socket_aborted");
        return -1;
    }

    return system_cloud_recv(buf, buflen, 0);
}

#endif /* HAL_PLATFORM_CLOUD_UDP */

// Returns number of bytes sent or -1 if an error occurred
int Spark_Send(const unsigned char *buf, uint32_t buflen, void* reserved)
{
    if (SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || spark_cloud_socket_closed() || cloud_socket_aborted)
    {
        LOG(TRACE, "SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || spark_cloud_socket_closed() || cloud_socket_aborted");
        //break from any blocking loop
        return -1;
    }

    return system_cloud_send(buf, buflen, 0);
}

// Returns number of bytes received or -1 if an error occurred
int Spark_Receive(unsigned char *buf, uint32_t buflen, void* reserved)
{
    if (SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || spark_cloud_socket_closed() || cloud_socket_aborted)
    {
        //break from any blocking loop
        LOG(TRACE, "SPARK_WLAN_RESET || SPARK_WLAN_SLEEP || spark_cloud_socket_closed() || cloud_socket_aborted");
        return -1;
    }

    return system_cloud_recv(buf, buflen, 0);
}

int Internet_Test(void)
{
    int r = system_internet_test(nullptr);
    if (r == SYSTEM_ERROR_NOT_SUPPORTED) {
        r = 0;
    }

    return r;
}

void Multicast_Presence_Announcement(void)
{
#if HAL_PLATFORM_NETWORK_MULTICAST
    system_multicast_announce_presence(nullptr);
#endif // HAL_PLATFORM_NETWORK_MULTICAST
}

int system_cloud_set_inet_family_keepalive(int af, unsigned int value, int flags) {
    switch (af) {
        case AF_INET: {
            LOG(TRACE, "Updating cloud keepalive for AF_INET: %lu -> %lu", s_ipv4_cloud_keepalive,
                    value);
            s_ipv4_cloud_keepalive = value;
            break;
        }
        case AF_INET6: {
            LOG(TRACE, "Updating cloud keepalive for AF_INET6: %lu -> %lu", s_ipv6_cloud_keepalive,
                    value);
            s_ipv6_cloud_keepalive = value;
            break;
        }
        default: {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
    }

#if !defined(SPARK_NO_CLOUD) && HAL_PLATFORM_CLOUD_UDP
    // Check if connected
    if (spark_cloud_flag_connected() || (flags & 1)) {
        if (((sockaddr*)&g_system_cloud_session_data.address)->sa_family == af) {
            // Change it now
            LOG(TRACE, "Applying new keepalive interval now");
            particle::protocol::connection_properties_t conn_prop = {};
            conn_prop.size = sizeof(conn_prop);
            conn_prop.keepalive_source = particle::protocol::KeepAliveSource::SYSTEM;
            spark_set_connection_property(particle::protocol::Connection::PING,
                    value, &conn_prop, nullptr);
        }
    }
#endif // !defined(SPARK_NO_CLOUD) && HAL_PLATFORM_CLOUD_UDP
    return 0;
}

int system_cloud_get_inet_family_keepalive(int af, unsigned int* value) {
    if (!value) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    switch (af) {
        case AF_INET: {
            *value = s_ipv4_cloud_keepalive;
            break;
        }
        case AF_INET6: {
            *value = s_ipv6_cloud_keepalive;
            break;
        }
        default: {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
    }

    return 0;
}
