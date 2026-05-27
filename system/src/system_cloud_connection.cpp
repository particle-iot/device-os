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
#include "system_connection_manager.h"
#include "system_threading.h"
#include "core_hal.h"
#include "service_debug.h"
#include "system_task.h"
#include "spark_wiring_ticks.h"
#if HAL_PLATFORM_CLOUD_UDP
#include "dtls_session_persist.h"
#endif // HAL_PLATFORM_CLOUD_UDP
#if HAL_PLATFORM_IFAPI && HAL_PLATFORM_BROKEN_MTU
#include "ifapi.h"
// FIXME: this should get included from protocol.h
#include "mbedtls_config.h"
#endif // HAL_PLATFORM_IFAPI && HAL_PLATFORM_BROKEN_MTU
#include "system_env.h"

#define IPNUM(ip)       ((ip)>>24)&0xff,((ip)>>16)&0xff,((ip)>> 8)&0xff,((ip)>> 0)&0xff

namespace {

uint16_t cloud_udp_port = PORT_COAPS; // default Particle Cloud UDP port

struct NetIfKeepAlive {
    unsigned keepAlive;
    particle::protocol::KeepAliveSource::Enum source;
};

// Indexed by network_interface_t. Only USER overrides are stored here; SYSTEM
// values are resolved from env vars / HAL defaults in system_cloud_get_netif_keepalive().
static NetIfKeepAlive netIfKeepAlives[NETWORK_INTERFACE_MAX] = {};

} /* anonymous */

volatile bool cloud_socket_aborted = false;

using namespace particle;
using namespace particle::system::cloud;

#if HAL_PLATFORM_CLOUD_UDP
SessionConnection g_system_cloud_session_data = {};
#endif /* HAL_PLATFORM_CLOUD_UDP */

void spark_cloud_udp_port_set(uint16_t port)
{
    cloud_udp_port = port;
}

uint16_t spark_cloud_udp_port_get()
{
    return cloud_udp_port;
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
        LOG(WARN, "Failed to load session data from persistent storage");
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

#if HAL_PLATFORM_IFAPI && HAL_PLATFORM_BROKEN_MTU
        // FIXME: this should come from somewhere else
        const auto minMtu = 1280;

        if_list* ifs = nullptr;
        if_get_list(&ifs);

        unsigned int minInterfaceMtu = minMtu;
        for (if_list* iface = ifs; iface != nullptr; iface = iface->next) {
            if (iface->iface) {
                unsigned int ifaceMtu = 0;
                if (!if_get_mtu(iface->iface, &ifaceMtu) && ifaceMtu > 0) {
                    minInterfaceMtu = std::min(ifaceMtu, minInterfaceMtu);
                }
            }
        }

        if_free_list(ifs);
        if (minInterfaceMtu < minMtu) {
            const size_t maxTransmitMessageSize = PROTOCOL_BUFFER_SIZE - (minMtu - minInterfaceMtu);
            LOG(INFO, "Updating protocol max tx msg size to %u", maxTransmitMessageSize);
            spark_protocol_set_connection_property(sp, particle::protocol::Connection::MAX_TRANSMIT_MESSAGE_SIZE, maxTransmitMessageSize,
                    nullptr, nullptr);
        } else {
            spark_protocol_set_connection_property(sp, particle::protocol::Connection::MAX_TRANSMIT_MESSAGE_SIZE, 0,
                    nullptr, nullptr);
        }
#endif // HAL_PLATFORM_IFAPI && HAL_PLATFORM_BROKEN_MTU
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
    cloud_socket_aborted = true;
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

    int r = system_cloud_recv(buf, buflen, 0);
    if (r == 0 && SPARK_CLOUD_PROTOCOL_HANDSHAKE_IN_PROGRESS) {
        SystemISRTaskQueue.process();
    }
    return r;
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

int system_cloud_get_netif_keepalive(network_interface_t netif, unsigned* value, uint32_t* source) {
    if (!value || !source || netif >= NETWORK_INTERFACE_MAX) {
        return SYSTEM_ERROR_OUT_OF_RANGE;
    }

    int netIfKeepAlive = 0;
    auto netIfSource = particle::protocol::KeepAliveSource::SYSTEM;

    if (netIfKeepAlives[netif].source == particle::protocol::KeepAliveSource::USER) {
        netIfKeepAlive = netIfKeepAlives[netif].keepAlive;
        netIfSource = particle::protocol::KeepAliveSource::USER;
        LOG(TRACE, "User keep alive override: %u for netif %u", netIfKeepAlive, netif);
    }

#if HAL_PLATFORM_ENV
#if PLATFORM_ID != PLATFORM_GCC
    const char * keepAliveEnvVarName = "";

    switch (netif) {
#if HAL_PLATFORM_ETHERNET
        case NETWORK_INTERFACE_ETHERNET:
            keepAliveEnvVarName = "PARTICLE_ETHERNET_CLOUD_KEEP_ALIVE";
            break;
#endif
#if HAL_PLATFORM_CELLULAR
        case NETWORK_INTERFACE_CELLULAR:
            keepAliveEnvVarName = "PARTICLE_CELLULAR_CLOUD_KEEP_ALIVE";
            break;
#endif
#if HAL_PLATFORM_WIFI
        case NETWORK_INTERFACE_WIFI_STA:
            keepAliveEnvVarName = "PARTICLE_WIFI_CLOUD_KEEP_ALIVE";
            break;
#endif
        default: // NETWORK_INTERFACE_ALL
            LOG(WARN, "Keep alive with no concrete netif specified");
            break;
    }

    if (!netIfKeepAlive) {
        // If netif specific env var, use it
        if (keepAliveEnvVarName && particle::system::hasEnv(keepAliveEnvVarName)) {
            particle::system::getEnv(keepAliveEnvVarName, netIfKeepAlive);
            LOG(TRACE, "Netif env var keep alive: %d for netif %u", netIfKeepAlive, netif);
        } 
        // If no netif specific env var, but global env var use that
        else if (particle::system::hasEnv("PARTICLE_CLOUD_KEEP_ALIVE")) {
            particle::system::getEnv("PARTICLE_CLOUD_KEEP_ALIVE", netIfKeepAlive);
            LOG(TRACE, "Global env var keep alive: %d for netif %u", netIfKeepAlive, netif);
        }
    }
    
#endif // PLATFORM_GCC
#endif // HAL_PLATFORM_ENV

    // Convert env var seconds -> ms
    if (netIfKeepAlive) {
        netIfKeepAlive *= 1000;
    } else {
        // If no env vars, use default keep alives
        netIfKeepAlive = (netif == NETWORK_INTERFACE_CELLULAR ? HAL_PLATFORM_CELLULAR_CLOUD_KEEPALIVE_INTERVAL : HAL_PLATFORM_DEFAULT_CLOUD_KEEPALIVE_INTERVAL);
    }

    *source = netIfSource;
    *value = netIfKeepAlive;
    return 0;
}

int system_cloud_set_netif_keepalive(network_interface_t netif, unsigned value) {
    if (netif >= NETWORK_INTERFACE_MAX) {
        return SYSTEM_ERROR_OUT_OF_RANGE;
    }

    netIfKeepAlives[netif].keepAlive = value;
    netIfKeepAlives[netif].source = particle::protocol::KeepAliveSource::USER;

    // If this is the active connection, change keep alive now
#if HAL_PLATFORM_IFAPI
    if (particle::system::ConnectionManager::instance()->getCloudConnectionNetwork() == netif) {
        return system_cloud_set_keepalive(netif);
    }
#else 
    return system_cloud_set_keepalive(netif);
#endif

    return 0;
}

int system_cloud_set_keepalive(network_interface_t netif) {
    unsigned keepAlive = 0;
    uint32_t keepAliveSource = 0;
    auto r = system_cloud_get_netif_keepalive(netif, &keepAlive, &keepAliveSource);
    if (r) {
        return r;
    }

#if HAL_PLATFORM_CLOUD_UDP
    // Change it now
    LOG(TRACE, "Applying new keepalive interval now: %u", keepAlive);
    particle::protocol::connection_properties_t conn_prop = {};
    conn_prop.size = sizeof(conn_prop);
    conn_prop.keepalive_source = keepAliveSource;
    spark_set_connection_property(particle::protocol::Connection::PING, keepAlive, &conn_prop, nullptr);
#endif // HAL_PLATFORM_CLOUD_UDP
    return 0;
}

