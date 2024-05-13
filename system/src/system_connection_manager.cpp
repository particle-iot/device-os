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

#include "hal_platform.h"

#if HAL_PLATFORM_IFAPI

#include "system_cloud_connection.h"
#include "system_connection_manager.h"
#include "system_cloud.h"
#include "system_cloud_internal.h"
#include "system_string_interpolate.h"
#include "system_network_diagnostics.h"
#include "spark_wiring_network.h"
#include "spark_wiring_vector.h"
#include "spark_wiring_random.h"
#include "endian_util.h"
#include "netdb_hal.h"
#include "ifapi.h"
#include "random.h"
#include "system_threading.h"

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

static int getCloudHostnameAndPort(uint16_t * port, char * hostname, int hostnameLength) {
    ServerAddress server_addr = {};
    char tmphost[sizeof(server_addr.domain) + 32] = {};
    if (hostnameLength < (int)(sizeof(tmphost)+1)) {
        return SYSTEM_ERROR_TOO_LARGE;
    }

    HAL_FLASH_Read_ServerAddress(&server_addr);
    if (server_addr.port == 0 || server_addr.port == 0xFFFF) {
        server_addr.port = spark_cloud_udp_port_get();
    }

    system_string_interpolate(server_addr.domain, tmphost, sizeof(tmphost), system_interpolate_cloud_server_hostname);
    strcpy(hostname, tmphost);
    *port = server_addr.port;

    LOG_DEBUG(TRACE, "Cloud hostname#port %s#%d", hostname, *port);
    return 0;
};

ConnectionManager::ConnectionManager()
    : preferredNetwork_(NETWORK_INTERFACE_ALL) {
    bestNetworks_ = ConnectionTester::getSupportedInterfaces();
}

ConnectionManager* ConnectionManager::instance() {
    static ConnectionManager man;
    return &man;
}

void ConnectionManager::setPreferredNetwork(network_handle_t network, bool preferred) {
    if (preferred) {
        if (network != NETWORK_INTERFACE_ALL) {
            preferredNetwork_ = network;

            // If cloud is already connected, and a preferred network is set, and it is up, move cloud connection to it immediately
            if (spark_cloud_flag_connected() && network_ready(spark::Network.from(preferredNetwork_), 0, nullptr)) {
                auto options = CloudDisconnectOptions().graceful(true).reconnect(true).toSystemOptions();
                spark_cloud_disconnect(&options, nullptr);
            }
        }
    } else {
        if (network == preferredNetwork_ || network == NETWORK_INTERFACE_ALL) {
            preferredNetwork_ = NETWORK_INTERFACE_ALL;
        }
    }
}

network_handle_t ConnectionManager::getPreferredNetwork() {
    return preferredNetwork_;
}

network_handle_t ConnectionManager::getCloudConnectionNetwork() {
    uint8_t socketNetIfIndex = 0;

    if (system_cloud_is_connected(nullptr) == 0) {
        socklen_t len = sizeof(socketNetIfIndex);
        sock_handle_t cloudSocket = system_cloud_get_socket_handle();
        sock_getsockopt(cloudSocket, SOL_SOCKET, SO_BINDTODEVICE, &socketNetIfIndex, &len);
    }

    return socketNetIfIndex;
}

network_handle_t ConnectionManager::selectCloudConnectionNetwork() {
    network_handle_t bestNetwork = NETWORK_INTERFACE_ALL;

    // 2: If no bound network, use preferred network
    if (preferredNetwork_ != NETWORK_INTERFACE_ALL && network_ready(spark::Network.from(preferredNetwork_), 0, nullptr)) {
        LOG_DEBUG(TRACE, "Using preferred network: %lu", preferredNetwork_);
        return preferredNetwork_;
    }

    // 3: If no preferred network, use the 'best' network based on criteria
    // 3.1: Network is ready: ie configured + connected (see ipv4 routable hook)
    // 3.2: Network has best criteria based on network tester results
    for (auto& i: bestNetworks_) {
        if (network_ready(spark::Network.from(i), 0, nullptr)) {
            LOG_DEBUG(TRACE, "Using best network: %lu", i);
            return i;
        }
    }

    // TODO: Determine a specific interface to bind to, even in the default case.
    // ie: How should we handle selecting a cloud connection when no interfaces are up/ready?
    // We should have some historical stats to rely on and then bring that network up? 
    return bestNetwork;
}

int ConnectionManager::testConnections() {
    ConnectionTester tester;
    int r = tester.testConnections();
    if (r == 0) {
        auto metrics = tester.getConnectionMetrics();
        bestNetworks_.clear();
        for (auto& i: metrics) {
            bestNetworks_.append(i.interface);
        }
    }
    return r;
}

ConnectionTester::ConnectionTester() {
    for (const auto& i: getSupportedInterfaces()) {
        struct ConnectionMetrics interfaceDiagnostics = {};
        interfaceDiagnostics.interface = i;
        interfaceDiagnostics.socketDescriptor = -1; // 0 is a valid socket fd number
        metrics_.append(interfaceDiagnostics);
    }
}

ConnectionTester::~ConnectionTester() {
    for (auto& i: metrics_) {
        sock_close(i.socketDescriptor);

        if (i.txBuffer) {
            free(i.txBuffer);
        }
        if (i.rxBuffer) {
            free(i.rxBuffer);
        }
    }
};

ConnectionMetrics* ConnectionTester::metricsFromSocketDescriptor(int socketDescriptor) {
    for (auto& i : metrics_) {
        if (i.socketDescriptor == socketDescriptor && i.txBuffer && i.rxBuffer /* sanity check */) {
            return &i;
        }
    }
    return nullptr;
};

bool ConnectionTester::testPacketsOutstanding() {
    for (auto& i : metrics_) {
        if (i.txPacketCount > i.rxPacketCount) {
            return true;
        }
    }
    return false;
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

int ConnectionTester::sendTestPacket(ConnectionMetrics* metrics) {
    int r = 0;
    // Only send a new packet if we have received the previous one, or we timeout waiting for a response
    if (metrics->txPacketCount == metrics->rxPacketCount || 
        (millis() > metrics->txPacketStartMillis + REACHABILITY_TEST_PACKET_TIMEOUT_MS)) {

        generateTestPacket(metrics);

        int r = sock_send(metrics->socketDescriptor, metrics->txBuffer, metrics->testPacketSize, 0);
        if (r > 0) {
            metrics->txPacketStartMillis = millis();
            metrics->txPacketCount++;
            metrics->testPacketSequenceNumber++;
            metrics->txBytes += metrics->testPacketSize;
        } else {
            LOG_DEBUG(WARN, "Test sock_send failed %d errno %d interface %d", r, errno, metrics->interface);
            return SYSTEM_ERROR_NETWORK;
        }
        
        LOG_DEBUG(TRACE, "Sock %d packet # %d tx > %d", metrics->socketDescriptor, metrics->txPacketCount, r);
    }
    return r;
};

int ConnectionTester::receiveTestPacket(ConnectionMetrics* metrics) {
    int r = sock_recv(metrics->socketDescriptor, metrics->rxBuffer, metrics->testPacketSize, MSG_DONTWAIT);
    if (r > 0) {
        metrics->totalPacketWaitMillis += (millis() - metrics->txPacketStartMillis);
        metrics->rxPacketCount++;
        metrics->rxBytes += metrics->testPacketSize;

        CHECK_TRUE((uint32_t)r == metrics->testPacketSize, SYSTEM_ERROR_BAD_DATA);

        if (memcmp(metrics->rxBuffer, metrics->txBuffer, r)) {
            LOG(WARN, "Socket %d Interface %d did not receive the same echo data: %d", metrics->socketDescriptor, metrics->interface, r);
            return SYSTEM_ERROR_BAD_DATA;
        }

    } else {
        LOG_DEBUG(WARN, "Test sock_recv failed %d errno %d interface %d", r, errno, metrics->interface);
        return SYSTEM_ERROR_NETWORK;
    }
    
    LOG_DEBUG(TRACE, "Sock %d packet # %d rx < %d", metrics->socketDescriptor, metrics->rxPacketCount, r);
    return r;
};

int ConnectionTester::generateTestPacket(ConnectionMetrics* metrics) {
    unsigned packetDataLength = random(1, REACHABILITY_MAX_PAYLOAD_SIZE);

    DTLSPlaintext_t msg = {
        REACHABILITY_TEST_MSG, // DTLS Message Type
        {0xfe, 0xfd},          // DTLS type 1.2
        0x8000,                // Differentiate interfaces by epoch field
        {},                    // Sequence number
        0                      // Payload length
    };

    int headerLength = sizeof(msg);
    int totalMessageLength = packetDataLength + headerLength;

    msg.epoch |= metrics->interface;
    msg.length = packetDataLength;
    uint32_t sequenceNumber = nativeToBigEndian(metrics->testPacketSequenceNumber);

    msg.epoch = nativeToBigEndian(msg.epoch);
    msg.length = nativeToBigEndian(msg.length);
    memcpy(&msg.sequence_number, &sequenceNumber, sizeof(sequenceNumber));

    Random rand;
    rand.gen((char*)metrics->txBuffer + sizeof(msg), packetDataLength);    
    memcpy(metrics->txBuffer, &msg, headerLength);
    metrics->testPacketSize = totalMessageLength;
    return 0;
};

int ConnectionTester::pollSockets(struct pollfd* pfds, int socketCount) {
    int pollCount = sock_poll(pfds, socketCount, 0);

    if (pollCount < 0) {
        LOG(ERROR, "Connection test poll error %d", pollCount);
        return 0;
    }

    for (int i = 0; i < socketCount; i++) {
        ConnectionMetrics* connection = metricsFromSocketDescriptor(pfds[i].fd);
        if (!connection) {
            LOG(ERROR, "No connection associated with socket descriptor %d", pfds[i].fd);
            return SYSTEM_ERROR_BAD_DATA;
        }

        if (pfds[i].revents & POLLIN) {
            receiveTestPacket(connection);
        }
        if (pfds[i].revents & POLLOUT) {
            CHECK(sendTestPacket(connection));
        }
    }
    return 0;
};

// GOAL: To maintain a list of which network interface is "best" at any given time
// 1) Retrieve the server hostname and port. Resolve the hostname to an addrinfo list (ie IP addresses of server)
// 2) Create a socket for each network interface to test. Bind this socket to the specific interface. Connect the socket
// 3) Add these created+connected sockets to a pollfd structure. Allocate buffers for the reachability test messages.
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

    getCloudHostnameAndPort(&tmpport, tmphost, sizeof(tmphost));
    snprintf(tmpserv, sizeof(tmpserv), "%u", tmpport);
    LOG(TRACE, "Resolving %s#%s", tmphost, tmpserv);
    // FIXME: get addrinfo/server IP from DNS lookup using the specific interfaces DNS server 
    r = netdb_getaddrinfo(tmphost, tmpserv, &hints, &info); 
    if (r) {
        LOG(ERROR, "No addrinfo for %s#%s", tmphost, tmpserv);
        return SYSTEM_ERROR_NETWORK;
    }

    SCOPE_GUARD({
        netdb_freeaddrinfo(info);
    });

    int socketCount = 0;
    auto pfds = std::make_unique<pollfd[]>(metrics_.size());;
    CHECK_TRUE(pfds, SYSTEM_ERROR_NO_MEMORY);
    
    // Step 2: Create, bind, and connect sockets for each network interface to test
    for (struct addrinfo* a = info; a != nullptr; a = a->ai_next) {
        // For each network interface to test, create + open a socket with the retrieved server address
        // If any of the sockets fail to be created + opened with this server address, return an error
        for (auto& connectionMetrics: metrics_) {
            auto network = spark::Network.from(connectionMetrics.interface);
            if (network == spark::Network) {
                LOG(ERROR, "No Network associated with interface %d", connectionMetrics.interface);
                return SYSTEM_ERROR_NETWORK;
            }
            if (!network_ready(network, 0, nullptr)) {
                LOG_DEBUG(TRACE,"%s not ready, skipping test", netifToName(connectionMetrics.interface));
                continue;
            }

            int s = sock_socket(a->ai_family, a->ai_socktype, a->ai_protocol);
            NAMED_SCOPE_GUARD(guard, {
                sock_close(s);
            });

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
            LOG_DEBUG(TRACE, "test socket=%d, connecting to %s#%u", s, serverHost, serverPort);

            struct ifreq ifr = {};
            if_index_to_name(connectionMetrics.interface, ifr.ifr_name);
            r = sock_setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr));
            if (r) {
                LOG(ERROR, "test socket=%d, failed to sock_setsockopt to IF %s, errno=%d", s, ifr.ifr_name, errno);
                return SYSTEM_ERROR_NETWORK;
            }

            connectionMetrics.socketConnAttempts++;
            r = sock_connect(s, a->ai_addr, a->ai_addrlen);
            if (r) {
                LOG(ERROR, "test socket=%d, failed to connect to %s#%u, errno=%d", s, serverHost, serverPort, errno);
                connectionMetrics.socketConnFailures++;
                return SYSTEM_ERROR_NETWORK;
            }
            LOG_DEBUG(INFO, "test socket # %d, %s bound to %s connected to %s#%u", s, ifr.ifr_name, netifToName(connectionMetrics.interface), serverHost, serverPort); 

            // Step 3: Use the socket descriptor for the polling structure, allocate our test buffers
            connectionMetrics.socketDescriptor = s;
            pfds[socketCount].fd = connectionMetrics.socketDescriptor;
            pfds[socketCount].events = (POLLIN | POLLOUT);
            socketCount++;

            guard.dismiss();
            CHECK(allocateTestPacketBuffers(&connectionMetrics));
        }
    }
    
    // Step 4: Send/Receive data on the sockets for the duration of the test time
    auto endTime = HAL_Timer_Get_Milli_Seconds() + REACHABILITY_TEST_DURATION_MS;
    while (HAL_Timer_Get_Milli_Seconds() < endTime) {
        CHECK(pollSockets(pfds.get(), socketCount));
        SystemISRTaskQueue.process();
    }

    // Only read from sockets to receive any final outstanding packets
    for (int i = 0; i < socketCount; i++) {
        pfds[i].events = (POLLIN);
    }

    endTime = HAL_Timer_Get_Milli_Seconds() + REACHABILITY_TEST_DURATION_MS;
    while(testPacketsOutstanding() && HAL_Timer_Get_Milli_Seconds() < endTime) {
        pollSockets(pfds.get(), socketCount);
        SystemISRTaskQueue.process();
    }

    // Step 5: calculate updated metrics
    for (auto& i: metrics_) {
        if (i.rxPacketCount > 0) {
            i.avgPacketRoundTripTime = (i.totalPacketWaitMillis / i.rxPacketCount);

            LOG(INFO,"%s: %d/%d packets %d/%d bytes received, avg rtt: %d", 
                netifToName(i.interface), 
                i.rxPacketCount,
                i.txPacketCount, 
                i.rxBytes,
                i.txBytes,
                i.avgPacketRoundTripTime);
        }
    }

    // Sort list by packet latency in ascending order, ie fastest to slowest
    std::sort(metrics_.begin(), metrics_.end(), [](const ConnectionMetrics& dg1, const ConnectionMetrics& dg2) {
        return (dg1.avgPacketRoundTripTime < dg2.avgPacketRoundTripTime); 
    });

    return 0;
}

const Vector<ConnectionMetrics> ConnectionTester::getConnectionMetrics(){
    return metrics_;
}

const Vector<network_interface_t> ConnectionTester::getSupportedInterfaces() {
    const Vector<network_interface_t> interfaceList = { 
#if HAL_PLATFORM_ETHERNET
    static_cast<network_interface_t>(NetworkDiagnosticsInterface::ETHERNET),
#endif
#if HAL_PLATFORM_WIFI 
    static_cast<network_interface_t>(NetworkDiagnosticsInterface::WIFI_STA),
#endif
#if HAL_PLATFORM_CELLULAR
    static_cast<network_interface_t>(NetworkDiagnosticsInterface::CELLULAR)
#endif
    };

    return interfaceList;
}

}} /* namespace particle::system */

#endif /* HAL_PLATFORM_IFAPI */

