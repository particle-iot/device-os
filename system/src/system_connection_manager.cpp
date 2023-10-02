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
#include "spark_wiring_udp.h"
#include "spark_wiring_vector.h"

// TODO: not use workarounds
#include "spark_wiring_ethernet.h"
#include "spark_wiring_cellular.h"
#include "spark_wiring_wifi.h"

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
    
    network_interface_t defaultInterface = NETWORK_INTERFACE_ALL;
    LOG(INFO, "setPreferredNetwork network: %lu preferredNetwork_: %lu", network, preferredNetwork_);
    preferredNetwork_ = preferred ? network : defaultInterface;
}

network_handle_t ConnectionManager::getPreferredNetwork() {
	// network_handle_t network;
	// size_t n = sizeof(network);
    // // TODO: error handling
	// auto r = spark_get_connection_property(SPARK_CLOUD_BIND_NETWORK_INTERFACE, &network, &n, nullptr);
    // LOG(INFO, "%d getPreferredNetwork %lu", r, network);
	// return network;

    LOG(INFO, "getPreferredNetwork %lu", preferredNetwork_);
	return preferredNetwork_;
}

network_handle_t ConnectionManager::getCloudConnectionNetwork() {
    uint8_t socketNetIfIndex = 0;

    if (system_cloud_is_connected(nullptr) == 0) {
        socklen_t len = sizeof(socketNetIfIndex);
        sock_handle_t cloudSocket = system_cloud_get_socket_handle();
        int r = sock_getsockopt(cloudSocket, SOL_SOCKET, SO_BINDTODEVICE, &socketNetIfIndex, &len);
        LOG(INFO, "getCloudConnectionNetwork %d : %lu", r, socketNetIfIndex);
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
    //LOG(INFO, "%d selectCloudConnectionNetwork %lu", r, boundNetwork);

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
    // 3.2: Network has best criteria based on network tester stats
    // TODO: More correct prioritization than just recreating the eth > wifi > cell order
    // IE use netif tester
    if (spark::Ethernet.ready()) {
        return NETWORK_INTERFACE_ETHERNET;
    }

#if HAL_PLATFORM_WIFI
    if (spark::WiFi.ready()) {
        return NETWORK_INTERFACE_WIFI_STA;
    }
#endif

#if HAL_PLATFORM_CELLULAR
    if (spark::Cellular.ready()) {
        return NETWORK_INTERFACE_CELLULAR;   
    }
#endif

    // TODO: Determine a specific interface to bind to, even in the default case.
    // ie: How should we handle selecting a cloud connection when no interfaces are up/ready?
    // We should have some historical stats to rely on and then bring that network up? 
    return bestNetwork;
}



NetIfTester::NetIfTester() {
    network_interface_t interfaceList[] = { 
        NETWORK_INTERFACE_ETHERNET,
#if HAL_PLATFORM_CELLULAR
        NETWORK_INTERFACE_CELLULAR,
#endif
#if HAL_PLATFORM_WIFI 
        NETWORK_INTERFACE_WIFI_STA 
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
    // GOAL: To maintain a list of which network interface is "best" at any given time
    for (auto& i: ifDiagnostics_) {
        testInterface(&i);
    }
}

int NetIfTester::testInterface(NetIfDiagnostics* diagnostics) {     
    auto network = spark::Network.from(diagnostics->interface);
    if (!network) {
        return SYSTEM_ERROR_NONE;
    }

    // TODO: Make this more CoAP like test message
    uint8_t udpTxBuffer[128] = {'H', 'e', 'l', 'l', 'o', 0};
    uint8_t udpRxBuffer[128] = {};

    uint8_t beginResult = 0;
    int sendResult, receiveResult = -1;
    IPAddress echoServer;

    diagnostics->dnsResolutionAttempts++;
    echoServer = network.resolve(UDP_ECHO_SERVER_HOSTNAME);
    if (!echoServer) {
        diagnostics->dnsResolutionFailures++;
        LOG(WARN, "IF #%d failed to resolve DNS for %s : %s", diagnostics->interface, UDP_ECHO_SERVER_HOSTNAME, echoServer.toString().c_str());
        return SYSTEM_ERROR_PROTOCOL;
    }

    UDP udpInstance = UDP();
    // TODO: What error conditions are the on UDP socket bind (ie the begin call here)?
    beginResult = udpInstance.begin(NetIfTester::UDP_ECHO_PORT, diagnostics->interface);
    
    LOG(INFO, "Testing IF #%d with %s : %s", diagnostics->interface, UDP_ECHO_SERVER_HOSTNAME, echoServer.toString().c_str());

    // TODO: Error conditions on sock_sendto
    // TODO: start packet rount trip timer
    sendResult = udpInstance.sendPacket(udpTxBuffer, strlen((char*)udpTxBuffer), echoServer, UDP_ECHO_PORT);
    if (sendResult > 0) {
        diagnostics->txBytes += strlen((char*)udpTxBuffer);
    }

    receiveResult = udpInstance.receivePacket(udpRxBuffer, strlen((char*)udpTxBuffer), 5000);
    if (receiveResult > 0) {
        diagnostics->rxBytes += strlen((char*)udpRxBuffer);
    }

    LOG(INFO, "UDP begin %d send: %d receive: %d message: %s", beginResult, sendResult, receiveResult, (char*)udpRxBuffer);
    udpInstance.stop();

    return SYSTEM_ERROR_NONE;
}

const Vector<NetIfDiagnostics>* NetIfTester::getDiagnostics(){
    return &ifDiagnostics_;
}

}} /* namespace particle::system */

#endif /* HAL_PLATFORM_IFAPI */

