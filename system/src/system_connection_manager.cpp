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

typedef struct DTLSPlaintext_t {
    uint8_t type;
    uint8_t version[2];
    uint16_t epoch;
    uint8_t sequence_number[6];
    uint16_t length;
} __attribute__((__packed__)) DTLSPlaintext_t;

constexpr uint16_t EPOCH_BASE = 0x8000;

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
            return "WiFi";
#endif
        default:
            return "";
    }
}

bool isValidScore(uint32_t score) {
    return std::numeric_limits<uint32_t>::max() != score;
}

static int getCloudHostnameAndPort(addrinfo** info, CloudServerAddressType* type, bool allowCached = false, network_interface_t interface = NETWORK_INTERFACE_ALL) {
    ServerAddress server_addr = {};
    HAL_FLASH_Read_ServerAddress(&server_addr);
    if (server_addr.port == 0 || server_addr.port == 0xFFFF) {
        server_addr.port = spark_cloud_udp_port_get();
    }

    return system_cloud_resolv_address(IPPROTO_UDP, &server_addr, allowCached ? (sockaddr*)&g_system_cloud_session_data.address : nullptr, info, type, false /* useCachedAddrInfo */, interface, !allowCached /* flushDnsCache*/);
}

size_t ConnectionTester::roundRobinInterfaceForDns_ = 0;

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

            // If cloud is already connected, and a preferred network is set, and it is up, attempt to move cloud connection to it immediately
            if (spark_cloud_flag_connected() && network_ready(preferredNetwork_, 0, nullptr)) {
                scheduleCloudConnectionNetworkCheck();
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

    bool canUsePreferred = false;
    // If no preferred network, use the 'best' network based on criteria
    // Network is ready: ie configured + connected (see ipv4 routable hook)
    // Network has best criteria based on network tester results
    for (auto& i: bestNetworks_) {
        if (network_ready(i.first, 0, nullptr)) {
            if (bestNetwork == NETWORK_INTERFACE_ALL) {
                bestNetwork = i.first;
            }
            if (preferredNetwork_ != NETWORK_INTERFACE_ALL && preferredNetwork_ == i.first && isValidScore(i.second) /* score */) {
                canUsePreferred = true;
            }
        }
    }

    // TODO: Determine a specific interface to bind to, even in the default case.
    // ie: How should we handle selecting a cloud connection when no interfaces are up/ready?
    // We should have some historical stats to rely on and then bring that network up? 

    if (!canUsePreferred) {
        if (preferredNetwork_ != NETWORK_INTERFACE_ALL && network_ready(preferredNetwork_, 0, nullptr)) {
            nextPeriodicCheck_ = HAL_Timer_Get_Milli_Seconds() + PERIODIC_CHECK_PERIOD_MS;
            LOG_DEBUG(TRACE, "Scheduled a periodic check @ %lu ms", nextPeriodicCheck_);
        }
        LOG(TRACE, "Using best network: %s", netifToName(bestNetwork));
        return bestNetwork;
    } else {
        nextPeriodicCheck_ = 0;
        LOG(TRACE, "Using preferred network: %s", netifToName(preferredNetwork_));
        return preferredNetwork_;
    }
}

int ConnectionManager::testConnections(bool background) {
    if (!background) {
        LOG_DEBUG(INFO, "testConnections full");
    }
    if (!background && testResultsActual_) {
        // Skip the test once
        testResultsActual_ = false;
        LOG_DEBUG(TRACE, "Skipping connection test as there are valid cached results");
        return 0;
    }

    int r = SYSTEM_ERROR_NETWORK;
    Vector<ConnectionMetrics> metrics;

    if (!background) {
        if (backgroundTestInProgress_) {
            LOG_DEBUG(WARN, "Background reachability test aborted");
        }
        backgroundTestInProgress_ = false;
        backgroundTester_.reset();
        testResultsActual_ = false;

        LOG_DEBUG(INFO, "Full reachability test started");
        ConnectionTester tester;
        CHECK(tester.prepare(true /* full test */, lastTestFailed_));
        // Blocking call
        r = tester.runTest();
        LOG_DEBUG(INFO, "Full reachability test finished (%d)", r);
        metrics = tester.getConnectionMetrics();
    } else {
        // Background test
        testResultsActual_ = false;
        if (!backgroundTestInProgress_) {
            LOG_DEBUG(INFO, "Background reachability test started");
            backgroundTester_ = std::make_unique<ConnectionTester>();
            CHECK_TRUE(backgroundTester_, SYSTEM_ERROR_NO_MEMORY);
            CHECK(backgroundTester_->prepare(false /* full test*/));
            backgroundTestInProgress_ = true;
        }
        r = backgroundTester_->runTest(0 /* non blocking */);
        if (!r || r != SYSTEM_ERROR_BUSY) {
            LOG_DEBUG(INFO, "Background reachability test finished (%d)", r);
            backgroundTestInProgress_ = false;
            metrics = backgroundTester_->getConnectionMetrics();
            backgroundTester_.reset();
        }
    }
    if (r == 0) {
        bestNetworks_.clear();
        bool hasValidScore = false;
        for (auto& i: metrics) {
            bestNetworks_.append(std::make_pair(i.interface, i.resultingScore));
            if (isValidScore(i.resultingScore)) {
                hasValidScore = true;
            }
        }
        if (background) {
            // Disable this for now
            // testResultsActual_ = true;
        } else {
            if (!hasValidScore) {
                lastTestFailed_ = true;
            }
        }
    }
    return r;
}

int ConnectionManager::scheduleCloudConnectionNetworkCheck() {
    testResultsActual_ = false;
    checkScheduled_ = true;
    return 0;
}

void ConnectionManager::handlePeriodicCheck() {
    if (nextPeriodicCheck_ != 0 && HAL_Timer_Get_Milli_Seconds() >= nextPeriodicCheck_) {
        if (!testIsAllowed()) {
            return;
        }
        if (preferredNetwork_ != NETWORK_INTERFACE_ALL && getCloudConnectionNetwork() != preferredNetwork_) {
            LOG_DEBUG(TRACE, "Periodic check because preferred interface was not picked during last run");
            scheduleCloudConnectionNetworkCheck();
        }
        nextPeriodicCheck_ = 0;
    }
}

bool ConnectionManager::testIsAllowed() const {
    uint8_t resetPending = 0;
    system_get_flag(SYSTEM_FLAG_RESET_PENDING, &resetPending, nullptr);

    return spark_cloud_flag_connected() && !resetPending && !SPARK_FLASH_UPDATE;
}

int ConnectionManager::checkCloudConnectionNetwork() {
    bool finishedBackgroundTest = false;
    if (backgroundTestInProgress_) {
        int r = testConnections(true /* background */);
        if (checkScheduled_) {
            // Invalidate results
            r = SYSTEM_ERROR_ABORTED;
        }
        if (!r) {
            // Finished without errors
            finishedBackgroundTest = true;
        } else if (r == SYSTEM_ERROR_BUSY) {
            // Wait to complete
            return 0;
        } else if (r != SYSTEM_ERROR_ABORTED) {
            // Finished with an error, reschedule a check, if aborted - do nothing
            return scheduleCloudConnectionNetworkCheck();
        }
    }

    if (!checkScheduled_) {
        handlePeriodicCheck();
    }

    if (!checkScheduled_ && !finishedBackgroundTest) {
        return 0;
    }

    if (!testIsAllowed()) {
        // Postpone until cloud connection is established or while in OTA
        return SYSTEM_ERROR_INVALID_STATE;
    }

    checkScheduled_ = false;

    unsigned countReady = 0;
    bool matchesCurrent = false;
    network_handle_t best = NETWORK_INTERFACE_ALL;
    for (const auto& i: bestNetworks_) {
        LOG_DEBUG(TRACE, "%s - ready=%d (getCloudConnectionNetwork()=%s)", netifToName(i.first), network_ready(i.first, 0, nullptr), netifToName(getCloudConnectionNetwork()));
        if (network_ready(i.first, 0, nullptr)) {
            countReady++;
            if (i.first == getCloudConnectionNetwork()) {
                matchesCurrent = true;
            }
            best = i.first;
        }
    }
    if (countReady == 0) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Simple case, just perform a cloud ping
    if (matchesCurrent && (countReady == 1 || getCloudConnectionNetwork() == getPreferredNetwork())) {
        spark_protocol_command(system_cloud_protocol_instance(), ProtocolCommands::PING, 0, nullptr);
        LOG_DEBUG(TRACE, "Still using the same network interface (%s) for the cloud connection - perform a cloud ping", netifToName(getCloudConnectionNetwork()));
        return 0;
    }
    if (countReady > 1) {
        if (!finishedBackgroundTest) {
            // Re-test connections
            backgroundTestInProgress_ = false;
            return testConnections(true /* background */);
        } else {
            best = selectCloudConnectionNetwork();
            // If matches current again just perform a ping
            if (best == getCloudConnectionNetwork()) {
                spark_protocol_command(system_cloud_protocol_instance(), ProtocolCommands::PING, 0, nullptr);
                LOG_DEBUG(TRACE, "Best network interface candidate for the cloud connection is still the same (%s) - perform a cloud ping", netifToName(best));
                return 0;
            }
        }
    }
    // If best candidate doesn't match current network interface - reconnect
    LOG(TRACE, "Best network interface for cloud connection changed (to %s) - move the cloud session", netifToName(best));
    auto options = CloudDisconnectOptions().reconnect(true);
    auto systemOptions = options.toSystemOptions();
    spark_cloud_disconnect(&systemOptions, nullptr);
    return 0;
}

ConnectionTester::ConnectionTester() {
    for (const auto& i: getSupportedInterfaces()) {
        struct ConnectionMetrics interfaceDiagnostics = {};
        interfaceDiagnostics.interface = i.first;
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
}

ConnectionMetrics* ConnectionTester::metricsFromSocketDescriptor(int socketDescriptor) {
    for (auto& i : metrics_) {
        if (i.socketDescriptor == socketDescriptor && i.txBuffer && i.rxBuffer /* sanity check */) {
            return &i;
        }
    }
    return nullptr;
}

bool ConnectionTester::testPacketsOutstanding() {
    for (auto& i : metrics_) {
        if (i.txPacketCount != REACHABILITY_TEST_MAX_TX_PACKET_COUNT) {
            return true;
        } else {
            if (i.txPacketCount != i.rxPacketCount) {
                return true;
            }
        }
    }
    return false;
}

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
        LOG_DEBUG(ERROR, "%s failed to allocate connection test buffers of size %d", netifToName(metrics->interface), maxMessageLength);
        return SYSTEM_ERROR_NO_MEMORY;
    }

    metrics->txBuffer = txBuffer;
    metrics->rxBuffer = rxBuffer;
    return 0;
}

int ConnectionTester::sendTestPacket(ConnectionMetrics* metrics) {
    int r = 0;
    // Only send a new packet every REACHABILITY_TEST_PACKET_TX_TIMEOUT_MS milliseconds
    if (HAL_Timer_Get_Milli_Seconds() >= (metrics->txPacketStartMillis + REACHABILITY_TEST_PACKET_TX_TIMEOUT_MS) && metrics->txPacketCount < REACHABILITY_TEST_MAX_TX_PACKET_COUNT) {
        size_t testPacketSize = CHECK(generateTestPacket(metrics));

        int r = sock_send(metrics->socketDescriptor, metrics->txBuffer, testPacketSize, 0);
        // Take TX errors into account too
        metrics->txPacketStartMillis = HAL_Timer_Get_Milli_Seconds();
        metrics->txPacketCount++;
        metrics->testPacketSequenceNumber++;
        if (r > 0) {
            metrics->txBytes += testPacketSize;
        } else {
            metrics->txPacketErrors++;
            LOG_DEBUG(WARN, "Test sock_send failed %d errno %d interface %d", r, errno, metrics->interface);
            return SYSTEM_ERROR_NETWORK;
        }
        
        // LOG_DEBUG(TRACE, "Sock %d packet # %d tx > %d", metrics->socketDescriptor, metrics->txPacketCount, r);
    }
    return r;
}

int ConnectionTester::receiveTestPacket(ConnectionMetrics* metrics) {
    msghdr msg = {};
    iovec iov = {};
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    iov.iov_base = metrics->rxBuffer;
    iov.iov_len = REACHABILITY_MAX_PAYLOAD_SIZE + sizeof(DTLSPlaintext_t);
    char controlBuf[CMSG_SPACE(sizeof(timespec))] = {};
    msg.msg_control = controlBuf;
    msg.msg_controllen = sizeof(controlBuf);

    int r = sock_recvmsg(metrics->socketDescriptor, &msg, MSG_DONTWAIT);
    if (r >= (int)sizeof(DTLSPlaintext_t)) {
        system_tick_t rxTimestamp = 0;
        for (cmsghdr* cm = CMSG_FIRSTHDR(&msg); cm != nullptr; cm = CMSG_NXTHDR(&msg, cm)) {
            if (cm->cmsg_level == SOL_SOCKET && cm->cmsg_type == SO_TIMESTAMPING) {
                auto ts = (timespec*)CMSG_DATA(cm);
                rxTimestamp = (ts->tv_sec * 1000 + ts->tv_nsec / 1000000);
            }
        }
        if (!rxTimestamp) {
            // Just in case
            LOG_DEBUG(WARN, "No RX timestamp from SO_TIMESTAMPING");
            rxTimestamp = HAL_Timer_Get_Milli_Seconds();
        }
        // Parse packet
        auto header = (DTLSPlaintext_t*)metrics->rxBuffer;
        header->epoch = bigEndianToNative(header->epoch);
        // LOG(TRACE, "epoch=%04x", header->epoch);
        CHECK_TRUE(header->epoch >= EPOCH_BASE, SYSTEM_ERROR_BAD_DATA);
        CHECK_TRUE((header->epoch & ~(EPOCH_BASE)) == metrics->interface, SYSTEM_ERROR_BAD_DATA);
        header->length = bigEndianToNative(header->length);
        // LOG(TRACE, "length=%u (r=%d)", header->length, r);
        CHECK_TRUE(header->length == (r - sizeof(DTLSPlaintext_t)), SYSTEM_ERROR_BAD_DATA);
        uint32_t sentTimestamp = 0;
        uint16_t seqNum = 0;
        memcpy(&sentTimestamp, header->sequence_number, sizeof(sentTimestamp));
        memcpy(&seqNum, header->sequence_number + sizeof(uint32_t), sizeof(uint16_t));
        seqNum = bigEndianToNative(seqNum);
        sentTimestamp = bigEndianToNative(sentTimestamp);
        // LOG(TRACE, "seqNum=%u testSeqNum=%u sentTimestamp=%u now=%u", seqNum, metrics->testPacketSequenceNumber, sentTimestamp, HAL_Timer_Get_Milli_Seconds());
        CHECK_TRUE(seqNum <= metrics->testPacketSequenceNumber, SYSTEM_ERROR_BAD_DATA);
        CHECK_TRUE(sentTimestamp < HAL_Timer_Get_Milli_Seconds(), SYSTEM_ERROR_BAD_DATA);
        if (metrics->rxPacketMask & (1 << seqNum)) {
            LOG(TRACE, "Duplicate packet seq=%u mask=%04x", seqNum, metrics->rxPacketMask);
            // Already seen this seq num
            return 0;
        }
        metrics->rxPacketMask |= (1 << seqNum);
        metrics->totalPacketWaitMillis += (rxTimestamp - sentTimestamp);
        metrics->rxPacketCount++;
        metrics->rxBytes += header->length;
        // LOG_DEBUG(TRACE, "Sock %d packet # %u rx < %d", metrics->socketDescriptor, seqNum, r);
    } else {
        LOG_DEBUG(WARN, "Test sock_recv failed %d errno %d interface %d", r, errno, metrics->interface);
        return SYSTEM_ERROR_NETWORK;
    }
    
    return r;
}

int ConnectionTester::generateTestPacket(ConnectionMetrics* metrics) {
    unsigned packetDataLength = random(1, REACHABILITY_MAX_PAYLOAD_SIZE);

    DTLSPlaintext_t msg = {
        REACHABILITY_TEST_MSG, // DTLS Message Type
        {0xfe, 0xfd},          // DTLS type 1.2
        EPOCH_BASE,            // Differentiate interfaces by epoch field
        {},                    // Sequence number
        0                      // Payload length
    };

    int headerLength = sizeof(msg);
    int totalMessageLength = packetDataLength + headerLength;

    msg.epoch |= metrics->interface;
    msg.length = packetDataLength;
    uint16_t sequenceNumber = nativeToBigEndian((uint16_t)metrics->testPacketSequenceNumber);

    msg.epoch = nativeToBigEndian(msg.epoch);
    msg.length = nativeToBigEndian(msg.length);
    uint32_t ts = nativeToBigEndian(HAL_Timer_Get_Milli_Seconds());
    memcpy(msg.sequence_number, &ts, sizeof(ts));
    memcpy(msg.sequence_number + sizeof(ts), &sequenceNumber, sizeof(sequenceNumber));

    Random rand;
    rand.gen((char*)metrics->txBuffer + sizeof(msg), packetDataLength);    
    memcpy(metrics->txBuffer, &msg, headerLength);
    return totalMessageLength;
}

int ConnectionTester::pollSockets(struct pollfd* pfds, int socketCount) {
    int pollCount = sock_poll(pfds, socketCount, 1 /* ms */);

    if (pollCount < 0) {
        LOG_DEBUG(ERROR, "Connection test poll error %d", pollCount);
        return 0;
    }

    for (int i = 0; i < socketCount; i++) {
        ConnectionMetrics* connection = metricsFromSocketDescriptor(pfds[i].fd);
        if (!connection) {
            LOG_DEBUG(ERROR, "No connection associated with socket descriptor %d", pfds[i].fd);
            return SYSTEM_ERROR_BAD_DATA;
        }

        // Ignore errors
        sendTestPacket(connection);

        if (pfds[i].revents & POLLIN) {
            int r = receiveTestPacket(connection);
            if (r == SYSTEM_ERROR_BAD_DATA) {
                LOG_DEBUG(WARN, "Reachability packet failed validation");
            }
        }
    }
    return 0;
}

// GOAL: To maintain a list of which network interface is "best" at any given time
// 1) Retrieve the server hostname and port. Resolve the hostname to an addrinfo list (ie IP addresses of server)
// 2) Create a socket for each network interface to test. Bind this socket to the specific interface. Connect the socket
// 3) Add these created+connected sockets to a pollfd structure. Allocate buffers for the reachability test messages.
// 4) Poll all the sockets. Polling sends a reachability test message and waits for the response. The test continues for the test duration
// 5) After polling completes, free the allocated buffers, reset diagnostics and calculate updated metrics. 
int ConnectionTester::prepare(bool fullTest, bool lastTestFailed) {
    struct addrinfo* info = nullptr;
    CloudServerAddressType type = CLOUD_SERVER_ADDRESS_TYPE_NONE;

    // Step 1: Retrieve the server hostname and port. Resolve the hostname to an addrinfo list (ie IP addresses of server)
    network_handle_t iface = NETWORK_INTERFACE_ALL;
    if (lastTestFailed) {
        for (int i = 0; i < metrics_.size(); i++) {
            int idx = roundRobinInterfaceForDns_ % metrics_.size();
            if (network_ready(metrics_[idx].interface, 0, nullptr)) {
                iface = metrics_[idx].interface;
                struct ifreq ifr = {};
                if_index_to_name(iface, ifr.ifr_name);
                LOG(TRACE, "Last rechability test failed, using interface %s for DNS queries explicitly", ifr.ifr_name);
                roundRobinInterfaceForDns_++;
                break;
            }
            roundRobinInterfaceForDns_++;
        }
    }
    CHECK(getCloudHostnameAndPort(&info, &type, !fullTest, iface));

    SCOPE_GUARD({
        netdb_freeaddrinfo(info);
    });

    int socketCount = 0;
    auto pfds = std::make_unique<pollfd[]>(metrics_.size());
    CHECK_TRUE(pfds, SYSTEM_ERROR_NO_MEMORY);

    int r = SYSTEM_ERROR_NETWORK;
    
    // Step 2: Create, bind, and connect sockets for each network interface to test
    for (struct addrinfo* a = info; a != nullptr; a = a->ai_next) {
        bool ok = true;

        switch (a->ai_family) {
            case AF_INET: {
                if (!network_ready(NETWORK_INTERFACE_ALL, NETWORK_READY_TYPE_IPV4, nullptr)) {
                    LOG_DEBUG(WARN, "skipping resolved AF_INET address, as ipv4 networking is not ready");
                    continue;
                }
                break;
            }
            case AF_INET6: {
                if (!network_ready(NETWORK_INTERFACE_ALL, NETWORK_READY_TYPE_IPV6, nullptr)) {
                    LOG_DEBUG(WARN, "skipping resolved AF_INET6 address, as ipv6 networking is not ready");
                    continue;
                }
                break;
            }
        }

        // For each network interface to test, create + open a socket with the retrieved server address
        // If any of the sockets fail to be created + opened with this server address, return an error
        for (auto& connectionMetrics: metrics_) {
            if (!network_ready(connectionMetrics.interface, 0, nullptr)) {
                LOG_DEBUG(TRACE,"%s not ready, skipping test", netifToName(connectionMetrics.interface));
                continue;
            }

            int s = sock_socket(a->ai_family, a->ai_socktype, a->ai_protocol);
            NAMED_SCOPE_GUARD(guard, {
                sock_close(s);
                ok = false;
            });

            if (s < 0) {
                LOG_DEBUG(ERROR, "test socket failed, family=%d, type=%d, protocol=%d, errno=%d", a->ai_family, a->ai_socktype, a->ai_protocol, errno);
                return SYSTEM_ERROR_NETWORK;
            }
#ifdef DEBUG_BUILD
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
#endif
            struct ifreq ifr = {};
            if_index_to_name(connectionMetrics.interface, ifr.ifr_name);
            r = sock_setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr));
            if (r) {
                LOG_DEBUG(ERROR, "test socket=%d, failed to sock_setsockopt to IF %s, errno=%d", s, ifr.ifr_name, errno);
                return SYSTEM_ERROR_NETWORK;
            }

            // Enable timestamps on recvd packets
            int dummy = 1;
            r = sock_setsockopt(s, SOL_SOCKET, SO_TIMESTAMPING, &dummy, sizeof(dummy));
            if (r) {
                LOG_DEBUG(WARN, "test socket=%d, failed to enable timestamping");
                // Not a critical error
            }

            connectionMetrics.socketConnAttempts++;
            r = sock_connect(s, a->ai_addr, a->ai_addrlen);
            if (r) {
                LOG_DEBUG(ERROR, "test socket=%d, failed to connect to %s#%u, errno=%d", s, serverHost, serverPort, errno);
                connectionMetrics.socketConnFailures++;
                return SYSTEM_ERROR_NETWORK;
            }
            LOG_DEBUG(INFO, "test socket # %d, %s bound to %s connected to %s#%u", s, ifr.ifr_name, netifToName(connectionMetrics.interface), serverHost, serverPort); 

            // Step 3: Use the socket descriptor for the polling structure, allocate our test buffers
            connectionMetrics.socketDescriptor = s;
            pfds[socketCount].fd = connectionMetrics.socketDescriptor;
            pfds[socketCount].events = (POLLIN);
            socketCount++;

            guard.dismiss();
            CHECK(allocateTestPacketBuffers(&connectionMetrics));
        }
        if (ok) {
            r = SYSTEM_ERROR_NONE;
            break;
        }
    }

    if (!r) {
        socketCount_ = socketCount;
        pfds_ = std::move(pfds);
    }

    endTime_ = HAL_Timer_Get_Milli_Seconds() + REACHABILITY_TEST_DURATION_MS;

    return r;
}

int ConnectionTester::runTest(system_tick_t maxBlockTime) {
    if (finished_) {
        return 0;
    }

    auto start = HAL_Timer_Get_Milli_Seconds();

    // Step 4: Send/Receive data on the sockets for the duration of the test time
    while(testPacketsOutstanding() && HAL_Timer_Get_Milli_Seconds() < endTime_) {
        pollSockets(pfds_.get(), socketCount_);
        SystemISRTaskQueue.process();
        if (HAL_Timer_Get_Milli_Seconds() - start >= maxBlockTime) {
            break;
        }
    }

    finished_ = !testPacketsOutstanding() || HAL_Timer_Get_Milli_Seconds() >= endTime_;
    if (finished_) {
        // Step 5: calculate updated metrics
        for (auto& i: metrics_) {
            if (i.rxPacketCount > 0) {
                i.avgPacketRoundTripTime = (i.totalPacketWaitMillis / i.rxPacketCount);
                i.resultingScore = i.totalPacketWaitMillis;
                unsigned penalty = 0;
                unsigned consecutive = 0;
                for (unsigned j = 0; j < i.txPacketCount; j++) {
                    if (i.rxPacketMask & (1 << j)) {
                        // Received
                        penalty = 0;
                        consecutive = 0;
                    } else {
                        penalty = i.avgPacketRoundTripTime * (2 << consecutive++) /* 2^(conscutive++) */;
                        LOG_DEBUG(TRACE, "%d: total=%u consecutive=%u penalty=%u resultingScore=%u new=%u", i.interface, i.totalPacketWaitMillis, consecutive, penalty, i.resultingScore, i.resultingScore + penalty);
                        i.resultingScore += penalty;
                    }
                }
                i.resultingScore /= i.rxPacketCount;
            } else {
                i.avgPacketRoundTripTime = 0;
                i.resultingScore = std::numeric_limits<decltype(i.resultingScore)>::max();
            }
            LOG(INFO,"%s: %lu/%lu packets (%lu tx errors) %lu/%lu bytes received, avg rtt: %lu, mask=%04x, score=%lu",
                    netifToName(i.interface), 
                    i.rxPacketCount,
                    i.txPacketCount, 
                    i.txPacketErrors,
                    i.rxBytes,
                    i.txBytes,
                    i.avgPacketRoundTripTime,
                    i.rxPacketMask,
                    i.resultingScore);
        }
        // Sort list by packet latency in ascending order, ie fastest to slowest
        std::sort(metrics_.begin(), metrics_.end(), [](const ConnectionMetrics& dg1, const ConnectionMetrics& dg2) {
            return (dg1.resultingScore < dg2.resultingScore); 
        });
        return 0;
    }
    return SYSTEM_ERROR_BUSY;
}

const Vector<ConnectionMetrics> ConnectionTester::getConnectionMetrics(){
    return metrics_;
}

const Vector<std::pair<network_interface_t, uint32_t>> ConnectionTester::getSupportedInterfaces() {
    const Vector<std::pair<network_interface_t, uint32_t>> interfaceList = { 
#if HAL_PLATFORM_ETHERNET
    {NETWORK_INTERFACE_ETHERNET, 0},
#endif
#if HAL_PLATFORM_WIFI 
    {NETWORK_INTERFACE_WIFI_STA, 0},
#endif
#if HAL_PLATFORM_CELLULAR
    {NETWORK_INTERFACE_CELLULAR, 0}
#endif
    };

    return interfaceList;
}

}} /* namespace particle::system */

#endif /* HAL_PLATFORM_IFAPI */

