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
#include "spark_wiring_network.h"
#include "spark_wiring_vector.h"
#include "endian_util.h"

// TODO: not use workarounds
#include "spark_wiring_udp.h"

namespace particle { namespace system {

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
    // // TODO: error handling
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
        int r = sock_getsockopt(cloudSocket, SOL_SOCKET, SO_BINDTODEVICE, &socketNetIfIndex, &len);
        LOG(TRACE, "getCloudConnectionNetwork %d : %lu", r, socketNetIfIndex);
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
    for (auto& i: *NetIfTester::instance()->getDiagnostics()) {
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

NetIfTester::NetIfTester() {
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
        struct NetIfDiagnostics interfaceDiagnostics = {};
        interfaceDiagnostics.interface = i;
        ifDiagnostics_.append(interfaceDiagnostics);
    }
}

NetIfTester* NetIfTester::instance() {
    static NetIfTester* tester = new NetIfTester();
    return tester;
}

void NetIfTester::testInterfaces() {
    LOG(INFO, "Connecting to %s#%d ", DEVICE_SERVICE_HOSTNAME, DEVICE_SERVICE_PORT);
    // GOAL: To maintain a list of which network interface is "best" at any given time
    for (auto& i: ifDiagnostics_) {
        testInterface(&i);
    }
    // sort list by packet latency
    std::sort(ifDiagnostics_.begin(), ifDiagnostics_.end(), [](const NetIfDiagnostics& dg1, const NetIfDiagnostics& dg2) {
        return (dg1.avgPacketRoundTripTime < dg2.avgPacketRoundTripTime); // In ascending order, ie fastest to slowest
    });

    for (auto& i: ifDiagnostics_) {
        LOG(TRACE, "If %lu latency %lu", i.interface, i.avgPacketRoundTripTime);
    }
}

    // TODO: Make size dynamic / realistic for typical coap payload lengths
    const unsigned REACHABILITY_TEST = 252;
    const unsigned REACHABILITY_PAYLOAD_SIZE = 32;

    typedef struct {
        uint8_t type;
        uint8_t version[2];
        uint16_t epoch;
        uint8_t sequence_number[6];
        uint16_t length;
        uint8_t fragment[REACHABILITY_PAYLOAD_SIZE];
    } __attribute__((__packed__)) DTLSPlaintext_t;


static const char* netifToName(uint8_t interfaceNumber) {
    switch(interfaceNumber) {
        case NETWORK_INTERFACE_ETHERNET:
            return "Ethernet";
        case NETWORK_INTERFACE_CELLULAR:
            return "Cellular";
        case NETWORK_INTERFACE_WIFI_STA:
            return "WiFi    ";
        default:
            return "";
    }
}

int NetIfTester::testInterface(NetIfDiagnostics* diagnostics) {
    auto network = spark::Network.from(diagnostics->interface);
    if (!network) {
        return SYSTEM_ERROR_BAD_DATA;
    }

    DTLSPlaintext_t reachabilityMessage = {
        REACHABILITY_TEST, // DTLS Message Type
        {0xfe, 0xfd},      // DTLS type 1.2
        0x8005,            // TODO: Differentiate interfaces by epoch field
        {},                // TODO: Sequence number
        REACHABILITY_PAYLOAD_SIZE,
        {}
    };
    memset(reachabilityMessage.fragment, 0xFF, sizeof(reachabilityMessage.fragment));

    reachabilityMessage.epoch = nativeToBigEndian(reachabilityMessage.epoch);
    reachabilityMessage.length = nativeToBigEndian(reachabilityMessage.length);

    uint8_t udpTxBuffer[128] = {};
    size_t udpTxMessageSize = sizeof(reachabilityMessage);
    memcpy(udpTxBuffer, &reachabilityMessage, udpTxMessageSize);

    uint8_t udpRxBuffer[128] = {};

    int sendResult, receiveResult = -1;
    IPAddress echoServer;

    diagnostics->dnsResolutionAttempts++;
    echoServer = network.resolve(DEVICE_SERVICE_HOSTNAME);
    if (!echoServer) {
        LOG(WARN, "%s failed to resolve DNS", netifToName(diagnostics->interface));
        diagnostics->avgPacketRoundTripTime = 0;
        diagnostics->dnsResolutionFailures++;
        return SYSTEM_ERROR_NETWORK;
    }

    // TODO: Dont use UDP wiring instance, use sockets directly and use LWIP poll()
    UDP udpInstance = UDP();
    udpInstance.begin(NetIfTester::DEVICE_SERVICE_PORT, diagnostics->interface);

    // TODO: Error conditions on sock_sendto
    sendResult = udpInstance.sendPacket(udpTxBuffer, udpTxMessageSize, echoServer, DEVICE_SERVICE_PORT);
    auto startMillis = HAL_Timer_Get_Milli_Seconds();
    if (sendResult > 0) {
        diagnostics->txBytes += udpTxMessageSize;
    } 

    bool timedOut = true;
    receiveResult = udpInstance.receivePacket(udpRxBuffer, udpTxMessageSize, 5000);
    if (receiveResult > 0) {
        diagnostics->rxBytes += udpTxMessageSize;
        timedOut = false;
    } 

    // TODO: Better metrics for latency rather than single packet timing
    // TODO: Validate recevied data, ie sequence number, packet size + contents
    auto endMillis = HAL_Timer_Get_Milli_Seconds();
    diagnostics->packetCount++;
    diagnostics->avgPacketRoundTripTime = endMillis - startMillis;

    LOG(TRACE, "%s bytes tx: %d rx: %d roundtrip time: %d %s", 
        netifToName(diagnostics->interface), 
        diagnostics->txBytes,
        diagnostics->rxBytes,
        diagnostics->avgPacketRoundTripTime,
        timedOut ? "timed out" : "");

    udpInstance.stop();

    return SYSTEM_ERROR_NONE;
}

const Vector<NetIfDiagnostics>* NetIfTester::getDiagnostics(){
    return &ifDiagnostics_;
}

}} /* namespace particle::system */

#endif /* HAL_PLATFORM_IFAPI */

