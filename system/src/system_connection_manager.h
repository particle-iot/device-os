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

#ifndef SYSTEM_CONNECTION_MANAGER_H
#define SYSTEM_CONNECTION_MANAGER_H

#include "hal_platform.h"

#if HAL_PLATFORM_IFAPI

#include "system_network.h"
#include "spark_wiring_vector.h"

namespace particle { namespace system {

struct ConnectionMetrics {
    network_interface_t interface;
    int socketDescriptor;
    uint8_t *txBuffer;
    uint8_t *rxBuffer;
    int testPacketSize;
    uint32_t testPacketSequenceNumber;
    int txPacketCount;
    int rxPacketCount;
    int txPacketStartMillis;
    int totalPacketWaitMillis;

    // uint32_t dnsResolutionAttempts;
    // uint32_t dnsResolutionFailures;
    // uint32_t socketConnAttempts;
    // uint32_t socketConnFailures;
    uint32_t txBytes;
    uint32_t rxBytes;
    uint32_t avgPacketRoundTripTime;
};

class ConnectionManager {
public:
    ConnectionManager();

    static ConnectionManager* instance();

    // Support Particle.prefered() / Particle.isPrefered() --> Use that network IF as the one for cloud socket binding
    void setPreferredNetwork(network_handle_t network, bool preferred);
    network_handle_t getPreferredNetwork();

    // Support Particle.connectionInterface() --> Get sock opt from cloud socket
    network_handle_t getCloudConnectionNetwork();

    // Provide interface for "best" network interface --> use that for cloud socket
    network_handle_t selectCloudConnectionNetwork();
    
private:
    network_handle_t preferredNetwork_;
};

class ConnectionTester {
public:
    ConnectionTester();
    ~ConnectionTester();

    static ConnectionTester* instance();
    int testConnections();

    const Vector<ConnectionMetrics>* getConnectionMetrics();

private:
    int allocateTestPacketBuffers(ConnectionMetrics* metrics);
    int generateTestPacket(ConnectionMetrics* metrics, int packetDataLength);
    int pollSockets(struct pollfd * pfds, int socketCount, int packetDataLength);
    int sendTestPacket(ConnectionMetrics* metrics, int length);
    int receiveTestPacket(ConnectionMetrics* metrics);
    void cleanupSockets(bool recalculateMetrics = true);
    ConnectionMetrics* metricsFromSocketDescriptor(int socketDescriptor);
    bool testPacketsOutstanding();

    const uint8_t REACHABILITY_TEST_MSG = 252;
    const unsigned REACHABILITY_MAX_PAYLOAD_SIZE = 256;
    const unsigned REACHABILITY_TEST_DURATION_MS = 5000;

    Vector<ConnectionMetrics> metrics_;
};

} } /* particle::system */

#endif /* HAL_PLATFORM_IFAPI */

#endif /* SYSTEM_CONNECTION_MANAGER_H */
