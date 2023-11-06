/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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
#include "logging.h"
LOG_SOURCE_CATEGORY("system.cm")

#undef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ALL

#include "hal_platform.h"

#if HAL_PLATFORM_IFAPI

#include "system_cloud_connection.h"
#include "system_connection_manager.h"
#include "system_cloud.h"
#include "system_cloud_internal.h"
#include "system_string_interpolate.h"
#include "spark_wiring_network.h"
#include "spark_wiring_vector.h"
#include "spark_wiring_random.h"
#include "endian_util.h"
#include "netdb_hal.h"
#include "ifapi.h"

namespace particle { namespace system {

typedef struct {
    uint8_t type;
    uint8_t version[2];
    uint16_t epoch;
    uint8_t sequence_number[6];
    uint16_t length;
} __attribute__((__packed__)) DTLSPlaintext_t;

static const char* netifToName(uint8_t interfaceNumber) {
    switch(interfaceNumber) {
        case NETWORK_INTERFACE_ETHERNET:
            return "Ethernet";
#if HAL_PLATFORM_CELLULAR
        case NETWORK_INTERFACE_CELLULAR:
            return "Cellular";
#endif
#if HAL_PLATFORM_WIFI
        case NETWORK_INTERFACE_WIFI_STA:
            return "WiFi    ";
#endif
        default:
            return "";
    }
}

ConnectionManager::ConnectionManager() {
    preferredNetwork_ = NETWORK_INTERFACE_ALL;
}

ConnectionManager* ConnectionManager::instance() {
    static ConnectionManager man;
    return &man;
}

void ConnectionManager::setPreferredNetwork(network_handle_t network, bool preferred) {
    // network_interface_t defaultInterface = NETWORK_INTERFACE_ALL;
    // network_interface_t preferredNetwork = preferred ? (network_interface_t)network : defaultInterface;
    // auto r = spark_set_connection_property(SPARK_CLOUD_BIND_NETWORK_INTERFACE, preferredNetwork, nullptr, nullptr);
    //LOG(INFO, "%d preferredNetwork %lu setPreferredNetwork %lu", r, preferredNetwork, network);
    
    //LOG(INFO, "setPreferredNetwork network: %lu preferredNetwork_: %lu", network, preferredNetwork_);
    if (preferred) {
        preferredNetwork_ = network;
    } else if (network == preferredNetwork_) {
        preferredNetwork_ = NETWORK_INTERFACE_ALL;
    }
}

network_handle_t ConnectionManager::getPreferredNetwork() {
    // network_handle_t network;
    // size_t n = sizeof(network);
    // auto r = spark_get_connection_property(SPARK_CLOUD_BIND_NETWORK_INTERFACE, &network, &n, nullptr);
    // LOG(INFO, "%d getPreferredNetwork %lu", r, network);
    // return network;

    //LOG(INFO, "getPreferredNetwork %lu", preferredNetwork_);
    return preferredNetwork_;
}

network_handle_t ConnectionManager::getCloudConnectionNetwork() {
    uint8_t socketNetIfIndex = 0;

    if (system_cloud_is_connected(nullptr) == 0) {
        socklen_t len = sizeof(socketNetIfIndex);
        sock_handle_t cloudSocket = system_cloud_get_socket_handle();
        //int r = 
        sock_getsockopt(cloudSocket, SOL_SOCKET, SO_BINDTODEVICE, &socketNetIfIndex, &len);
        //LOG(TRACE, "getCloudConnectionNetwork %d : %lu", r, socketNetIfIndex);
    }

	return socketNetIfIndex;
}

network_handle_t ConnectionManager::selectCloudConnectionNetwork() {
    network_handle_t bestNetwork = NETWORK_INTERFACE_ALL;

    // 1: If there is a bound network connection. Do not use anything else, regardless of network state
    network_handle_t boundNetwork;
    size_t n = sizeof(boundNetwork);
    // TODO: error handling
    auto r = spark_get_connection_property(SPARK_CLOUD_BIND_NETWORK_INTERFACE, &boundNetwork, &n, nullptr);
    LOG(TRACE, "%d selectCloudConnectionNetwork %lu", r, boundNetwork);

    if (boundNetwork != NETWORK_INTERFACE_ALL) {
        LOG(TRACE, "%d using bound network: %lu", r, boundNetwork);
        return boundNetwork;
    }

    // 2: If no bound network, use preferred network
    if (preferredNetwork_ != NETWORK_INTERFACE_ALL && spark::Network.from(preferredNetwork_).ready()) {
        LOG(TRACE, "using preferred network: %lu", preferredNetwork_);
        return preferredNetwork_;
    }

    // 3: If no preferred network, use the 'best' network based on criteria
    // 3.1: Network is ready: ie configured + connected (see ipv4 routable hook)
    // 3.2: Network has best criteria based on network tester stats (vector should be sorted in "best" order)
    for (auto& i: *ConnectionTester::instance()->getConnectionMetrics()) {
        if (spark::Network.from(i.interface).ready()) {
            LOG(TRACE, "using best tested network: %lu", i.interface);
            return i.interface;
        }
    }

    // TODO: Determine a specific interface to bind to, even in the default case.
    // ie: How should we handle selecting a cloud connection when no interfaces are up/ready?
    // We should have some historical stats to rely on and then bring that network up? 
    return bestNetwork;
}

ConnectionTester::ConnectionTester() {
    network_interface_t interfaceList[] = { 
        NETWORK_INTERFACE_ETHERNET,
#if HAL_PLATFORM_WIFI 
        NETWORK_INTERFACE_WIFI_STA, 
#endif
#if HAL_PLATFORM_CELLULAR
        NETWORK_INTERFACE_CELLULAR
#endif
    };
    
    for (const auto& i: interfaceList) {
        struct ConnectionMetrics interfaceDiagnostics = {};
        interfaceDiagnostics.interface = i;
        metrics_.append(interfaceDiagnostics);
    }
}

ConnectionTester* ConnectionTester::instance() {
    static ConnectionTester* tester = new ConnectionTester();
    return tester;
}

static int getCloudHostnameAndPort(uint16_t * port, char * hostname, int hostnameLength) {
    // Use this test server until real echo server is ready
    strcpy(hostname, "publish-receiver-udp.particle.io");
    *port = 40000;
    return 0;

    // TODO: Something better than this?
    // ServerAddress server_addr = {};
    // char tmphost[sizeof(server_addr.domain) + 32] = {};
    // if (hostnameLength < (int)(sizeof(tmphost)+1)) {
    //     return SYSTEM_ERROR_TOO_LARGE;
    // }

    // HAL_FLASH_Read_ServerAddress(&server_addr);
    // if (server_addr.port == 0 || server_addr.port == 0xFFFF) {
    //     server_addr.port = spark_cloud_udp_port_get();
    // }

    // system_string_interpolate(server_addr.domain, tmphost, sizeof(tmphost), system_interpolate_cloud_server_hostname);
    // strcpy(hostname, tmphost);
    // *port = server_addr.port;

    // //LOG(TRACE, "Cloud hostname#port %s#%d", hostname, *port);
    // return 0;
};

ConnectionMetrics* ConnectionTester::metricsFromSocketDescriptor(int socketDescriptor) {
    for (auto& i : metrics_) {
        if (i.socketDescriptor == socketDescriptor) {
            return &i;
        }
    }
    return nullptr;
};

int ConnectionTester::allocateTestPacketBuffers(ConnectionMetrics* metrics) {
    int maxMessageLength = REACHABILITY_MAX_PAYLOAD_SIZE + sizeof(DTLSPlaintext_t);
    uint8_t* txBuffer = (uint8_t*)malloc(maxMessageLength);
    uint8_t* rxBuffer = (uint8_t*)malloc(maxMessageLength);

    if (!(txBuffer && rxBuffer)) {
        if (txBuffer) {
            free(txBuffer);
        }
        if (rxBuffer) {
            free(rxBuffer);
        }
        LOG(ERROR, "%s failed to allocate connection test buffers of size %d", netifToName(metrics->interface), maxMessageLength);
        return SYSTEM_ERROR_NO_MEMORY;
    }

    metrics->txBuffer = txBuffer;
    metrics->rxBuffer = rxBuffer;
    return 0;
};

int ConnectionTester::sendTestPacket(ConnectionMetrics* metrics, int length) {
    int r = 0;
    // Only send a new packet if we have received the previous one
    if (metrics->txPacketCount == metrics->rxPacketCount) {
        CHECK(generateTestPacket(metrics, length));

        int r = sock_send(metrics->socketDescriptor, metrics->txBuffer, metrics->testPacketSize, 0);
        if (r > 0) {
            metrics->txPacketStartMillis = millis();
            metrics->txPacketCount++;
            metrics->testPacketSequenceNumber++;
            metrics->txBytes += metrics->testPacketSize;
        } else {
            LOG(ERROR, "test sock_send failed %d errno %d interface %d", r, errno, metrics->interface);
            return SYSTEM_ERROR_NETWORK;
        }
        
        LOG(TRACE, "sock %d packet # %d tx > %d", metrics->socketDescriptor, metrics->txPacketCount, r);
    }
    return r;
};

int ConnectionTester::receiveTestPacket(ConnectionMetrics* metrics) {
    int r = sock_recv(metrics->socketDescriptor, metrics->rxBuffer, metrics->testPacketSize, MSG_DONTWAIT);
    if (r > 0) {
        CHECK_TRUE(r == metrics->testPacketSize, SYSTEM_ERROR_BAD_DATA);

        if (memcmp(metrics->rxBuffer, metrics->txBuffer, r)) {
            // Did not receive the exact same message sent
            LOG(ERROR, "test socket on interface %d did not receive the same echo data");
            return SYSTEM_ERROR_BAD_DATA;
        }

        metrics->totalPacketWaitMillis += (millis() - metrics->txPacketStartMillis);
        metrics->rxPacketCount++;
        metrics->rxBytes += metrics->testPacketSize;
    } else {
        LOG(ERROR, "test sock_recv failed %d errno %d interface %d", r, errno, metrics->interface);
        return SYSTEM_ERROR_NETWORK;
    }
    
    LOG(TRACE, "sock %d packet # %d rx < %d", metrics->socketDescriptor, metrics->rxPacketCount, r);
    return r;
};

int ConnectionTester::generateTestPacket(ConnectionMetrics* metrics, int packetDataLength) {
    auto network = spark::Network.from(metrics->interface);
    if (!network) {
        LOG(ERROR, "No Network associated with interface %d", metrics->interface);
        return SYSTEM_ERROR_BAD_DATA;
    }

    DTLSPlaintext_t msg = {
        REACHABILITY_TEST_MSG, // DTLS Message Type
        {0xfe, 0xfd},          // DTLS type 1.2
        0x8000,                // Differentiate interfaces by epoch field
        {},                    // Sequence number
        0                      // Payload length
    };

    int headerLength = sizeof(msg);
    int totalMessageLength = packetDataLength + headerLength;
    //LOG(TRACE, "packetDataLength %u headerLength %d totalMessageLength %d", packetDataLength, headerLength, totalMessageLength);

    msg.epoch |= metrics->interface;
    msg.length = packetDataLength;
    uint32_t sequenceNumber = nativeToBigEndian(metrics->testPacketSequenceNumber);

    msg.epoch = nativeToBigEndian(msg.epoch);
    msg.length = nativeToBigEndian(msg.length);
    memcpy(&msg.sequence_number, &sequenceNumber, sizeof(sequenceNumber));

    // TODO: Make data random instead
    memset(metrics->txBuffer, 0xFF, totalMessageLength);    
    memcpy(metrics->txBuffer, &msg, headerLength);
    metrics->testPacketSize = totalMessageLength;
    // LOG_DUMP(TRACE, metrics->txBuffer, totalMessageLength);
    return 0;
};

int ConnectionTester::pollSockets(struct pollfd* pfds, int socketCount, int packetDataLength) {
    int pollCount = sock_poll(pfds, socketCount, 5000);

    if (pollCount <= 0) {
        LOG(WARN, "Connection test poll timeout/error %d", pollCount);
        return 0;
    }

    for (int i = 0; i < socketCount; i++) {
        ConnectionMetrics* connection = metricsFromSocketDescriptor(pfds[i].fd);
        if (!connection) {
            LOG(ERROR, "No connection associated with socket descriptor %d", pfds[i].fd);
            return SYSTEM_ERROR_BAD_DATA;
        }

        if (pfds[i].revents & POLLIN) {
            CHECK(receiveTestPacket(connection));
        }
        if (pfds[i].revents & POLLOUT) {
            CHECK(sendTestPacket(connection, packetDataLength));
        }
    }
    return 0;
};

void ConnectionTester::cleanupSockets(bool recalculateMetrics) {
    for (auto& i: metrics_) {

        if (recalculateMetrics && i.rxPacketCount > 0) {
            i.avgPacketRoundTripTime = (i.totalPacketWaitMillis / i.rxPacketCount);

            LOG(INFO,"%s sent %d packets, got %d packets, avg rtt: %d", 
                netifToName(i.interface), 
                i.txPacketCount, 
                i.rxPacketCount,
                i.avgPacketRoundTripTime);
        }

        sock_close(i.socketDescriptor);
        i.socketDescriptor = -1;
        i.txPacketCount = 0;
        i.rxPacketCount = 0;
        i.totalPacketWaitMillis = 0;

        if (i.txBuffer) {
            free(i.txBuffer);
        }
        if (i.rxBuffer) {
            free(i.rxBuffer);
        }
    }

    // Sort list by packet latency in ascending order, ie fastest to slowest
    std::sort(metrics_.begin(), metrics_.end(), [](const ConnectionMetrics& dg1, const ConnectionMetrics& dg2) {
        return (dg1.avgPacketRoundTripTime < dg2.avgPacketRoundTripTime); 
    });
};


// GOAL: To maintain a list of which network interface is "best" at any given time
// General workflow
// 1) Retrieve the server hostname and port. Resolve the hostname to an addrinfo list (ie IP addresses of server)
// 2) Create a socket for each network interface to test. Bind this socket to the specific interface. Connect the socket
// 3) Add these created+connected sockets to a pollfd structure. Allocate buffers for the reachability test messages
// 4) Poll all the sockets. Polling sends a reachability test message and waits for the response. The test continues for the test duration
// 5) After polling completes, free the allocated buffers, reset diagnostics and calculate updated metrics. 
int ConnectionTester::testConnections() {
    // Step 1: resolve server hostname to IP address
    struct addrinfo* info = nullptr;
    struct addrinfo hints = {};
    hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_socktype = SOCK_DGRAM;

    char tmphost[128] = {}; // TODO: better size ie sizeof(address->domain)
    char tmpserv[8] = {};
    uint16_t tmpport = 0;

    int r = SYSTEM_ERROR_NETWORK;
    bool testSuccessful = false;

    getCloudHostnameAndPort(&tmpport, tmphost, sizeof(tmphost));
    snprintf(tmpserv, sizeof(tmpserv), "%u", tmpport);
    LOG(TRACE, "Resolving %s#%s", tmphost, tmpserv);
    // TODO: get addrinfo/server IP from DNS lookup using the specific interface? 
    r = netdb_getaddrinfo(tmphost, tmpserv, &hints, &info); 
    if (r) {
        LOG(ERROR, "No addrinfo for %s#%s", tmphost, tmpserv);
        return SYSTEM_ERROR_NETWORK;
    }

    SCOPE_GUARD({
        cleanupSockets(testSuccessful);
        netdb_freeaddrinfo(info);
    });

    // Step 2: Create, bind, and connect sockets for each network interface to test
    for (struct addrinfo* a = info; a != nullptr; a = a->ai_next) {

        // For each network interface to test, create + open a socket with the retrieved server address
        // If any of the sockets fail to be created + opened with this server address, return an error
        for (auto& connectionMetrics: metrics_) {
            int s = sock_socket(a->ai_family, a->ai_socktype, a->ai_protocol);
            if (s < 0) {
                LOG(ERROR, "test socket failed, family=%d, type=%d, protocol=%d, errno=%d", a->ai_family, a->ai_socktype, a->ai_protocol, errno);
                return SYSTEM_ERROR_NETWORK;
            }
       
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
            //LOG(TRACE, "test socket=%d, connecting to %s#%u", s, serverHost, serverPort);

            struct ifreq ifr = {};
            if_index_to_name(connectionMetrics.interface, ifr.ifr_name);
            r = sock_setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr));
            if (r) {
                LOG(ERROR, "test socket=%d, failed to sock_setsockopt to IF %s, errno=%d", s, ifr.ifr_name, errno);
                sock_close(s);
                return SYSTEM_ERROR_NETWORK;
            }

            r = sock_connect(s, a->ai_addr, a->ai_addrlen);
            if (r) {
                LOG(ERROR, "test socket=%d, failed to connect to %s#%u, errno=%d", s, serverHost, serverPort, errno);
                return SYSTEM_ERROR_NETWORK;
            }
            LOG(INFO, "test socket # %d, name %s bound to %s connected to %s#%u", s, ifr.ifr_name, netifToName(connectionMetrics.interface), serverHost, serverPort); 

            // s is now valid socket descriptor, use it for POLL
            connectionMetrics.socketDescriptor = s;

            CHECK(allocateTestPacketBuffers(&connectionMetrics));
        }
    }
    
    // Step 3: Setup the needed buffers to poll all of the sockets
    int socketCount = 3; // TODO: Make dynamic based on number of available interfaces
    struct pollfd pfds[socketCount];
    
    for (int i = 0; i < socketCount; i++) {
        pfds[i].fd = metrics_[i].socketDescriptor;
        pfds[i].events = (POLLIN | POLLOUT);
    }

    // Step 4: Send/Receive data on the sockets for the duration of the test time
    unsigned packetDataLength = random(1, REACHABILITY_MAX_PAYLOAD_SIZE);
    auto endTime = millis() + REACHABILITY_TEST_DURATION_MS;
    while (millis() < endTime) {
        CHECK(pollSockets(pfds, socketCount, packetDataLength));
    }

    // Read from sockets to get last packet
    for (int i = 0; i < socketCount; i++) {
        pfds[i].events = (POLLIN);
    }
    pollSockets(pfds, socketCount, packetDataLength);

    // STEP 5: Cleanup test metrics, close sockets, calculate updated diagnostics
    testSuccessful = true;

    return 0;
}

const Vector<ConnectionMetrics>* ConnectionTester::getConnectionMetrics(){
    return &metrics_;
}

}} /* namespace particle::system */

#endif /* HAL_PLATFORM_IFAPI */

