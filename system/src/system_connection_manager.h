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


struct NetIfDiagnostics {
    network_interface_t interface;
    uint32_t dnsResolutionAttempts;
    uint32_t dnsResolutionFailures;
    uint32_t socketConnAttempts;
    uint32_t socketConnFailures;
    uint32_t packetCount;
    uint32_t txBytes;
    uint32_t rxBytes;
    uint32_t rxTimeouts;
    uint32_t avgPacketRoundTripTime;
};

class NetIfTester {
public:
    NetIfTester();
    ~NetIfTester();

    static NetIfTester* instance();
    void testInterfaces();

    const Vector<NetIfDiagnostics>* getDiagnostics();

private:
    int testInterface(NetIfDiagnostics* diagnostics);

    const uint16_t UDP_ECHO_PORT = 40000;
    const char * UDP_ECHO_SERVER_HOSTNAME = "publish-receiver-udp.particle.io";
    
    // TODO: Query at runtime
    const uint16_t DEVICE_SERVICE_PORT = 5684;
    const char * DEVICE_SERVICE_HOSTNAME = "a10aced202194944a0429fc.v5.udp-mesh.staging.particle.io";

    Vector<NetIfDiagnostics> ifDiagnostics_;

};

} } /* particle::system */

#endif /* HAL_PLATFORM_IFAPI */

#endif /* SYSTEM_CONNECTION_MANAGER_H */
