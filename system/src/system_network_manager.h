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

#ifndef SYSTEM_NETWORK_MANAGER_H
#define SYSTEM_NETWORK_MANAGER_H

#include "hal_platform.h"

#if HAL_PLATFORM_IFAPI

#include "ifapi.h"
#include "resolvapi.h"
#include <atomic>
#include "intrusive_list.h"
#include "system_defs.h"
#include "system_network_configuration.h"
#include "spark_wiring_vector.h"

namespace particle { namespace system {

class NetworkManager {
public:
    ~NetworkManager();

    static NetworkManager* instance();

    void init();
    void destroy();

    int enableNetworking();
    int disableNetworking();
    bool isNetworkingEnabled() const;

    int activateConnections();
    int deactivateConnections(network_disconnect_reason reason = NETWORK_DISCONNECT_REASON_UNKNOWN);
    bool isEstablishingConnections() const;

    bool isConnectivityAvailable() const;
    bool isIp4ConnectivityAvailable() const;
    bool isIp6ConnectivityAvailable() const;

    enum class ProtocolState {
        UNCONFIGURED = 0,
        LINKLOCAL = 1,
        CONFIGURED = 2
    };
    static_assert((int)ProtocolState::CONFIGURED > (int)ProtocolState::LINKLOCAL &&
            (int)ProtocolState::LINKLOCAL > (int)ProtocolState::UNCONFIGURED, "UNCONFIGURED < LINKLOCAL < CONFIGURED");

    ProtocolState getInterfaceIp4State(if_t iface) const;
    ProtocolState getInterfaceIp6State(if_t iface) const;

    bool isInterfacePowerState(if_t iface = nullptr, if_power_state_t state = IF_POWER_STATE_NONE) const;
    bool isInterfacePhyReady(if_t iface = nullptr) const;
    bool isInterfaceOn(if_t iface = nullptr) const;
    bool isInterfaceOff(if_t iface = nullptr) const;

    bool isConfigured(if_t iface = nullptr) const;
    int clearConfiguration(if_t iface = nullptr);

    int enableInterface(if_t iface = nullptr);
    int disableInterface(if_t iface = nullptr, network_disconnect_reason reason = NETWORK_DISCONNECT_REASON_UNKNOWN);
    bool isInterfaceEnabled(if_t iface) const;
    int countEnabledInterfaces();
    int syncInterfaceStates();

    int powerInterface(if_t iface = nullptr, bool enable = true);

    int waitInterfaceOff(if_t iface, system_tick_t timeout) const;

    enum class State {
        NONE,
        /* Networking is disabled */
        DISABLED,
        /* Networking is enabled, all interfaces are administratively down */
        IFACE_DOWN,
        /* Interfaces are being brought down */
        IFACE_REQUEST_DOWN,
        /* Interfaces are being brought up */
        IFACE_REQUEST_UP,
        /* At least one of the interfaces came up */
        IFACE_UP,
        /* At least one of the interface links came up */
        IFACE_LINK_UP,
        /* At least one of the interfaces has IPv4 or IPv6 configuration */
        IP_CONFIGURED
    };

    State getState() const;

    int setConfiguration(if_t iface, const NetworkInterfaceConfig& conf);
    int getConfiguration(if_t iface, spark::Vector<NetworkInterfaceConfig>& conf, const char* profile = nullptr, size_t profileLen = 0);
    int getConfiguration(if_t iface, NetworkInterfaceConfig* conf, const char* profile = nullptr, size_t profileLen = 0);
    int clearStoredConfiguration();

protected:
    NetworkManager();

private:

    const char* stateToName(State state) const;

    enum class DnsState {
        UNCONFIGURED,
        CONFIGURED
    };

    enum class NetworkStatus {
        NETWORK_STATUS_POWERING_OFF,
        NETWORK_STATUS_OFF,
        NETWORK_STATUS_POWERING_ON,
        NETWORK_STATUS_ON,
        NETWORK_STATUS_CONNECTING,
        NETWORK_STATUS_CONNECTED,
        NETWORK_STATUS_DISCONNECTING,
        NETWORK_STATUS_DISCONNECTED
    };

    struct InterfaceRuntimeState {
        InterfaceRuntimeState()
                : ip4State(ProtocolState::UNCONFIGURED),
                  ip6State(ProtocolState::UNCONFIGURED),
                  pwrState(IF_POWER_STATE_NONE) {
        }
        InterfaceRuntimeState* next = nullptr;
        bool enabled = false;
        if_t iface = nullptr;
        std::atomic<ProtocolState> ip4State;
        std::atomic<ProtocolState> ip6State;
        std::atomic<if_power_state_t> pwrState;
    };

    void transition(State state);
    static void ifEventHandlerCb(void* arg, if_t iface, const struct if_event* ev);
    void ifEventHandler(if_t iface, const struct if_event* ev);

    void handleIfAdded(if_t iface, const struct if_event* ev);
    void handleIfRemoved(if_t iface, const struct if_event* ev);
    void handleIfState(if_t iface, const struct if_event* ev);
    void handleIfLink(if_t iface, const struct if_event* ev);
    void handleIfAddr(if_t iface, const struct if_event* ev);
    void handleIfLinkLayerAddr(if_t iface, const struct if_event* ev);
    void handleIfPowerState(if_t iface, const struct if_event* ev);
    void handleIfPhyState(if_t iface, const struct if_event* ev);

    unsigned int countIfacesWithFlags(unsigned int flags) const;
    void refreshIpState();
    void refreshDnsState();

    void clearDnsConfiguration(if_t iface);

    bool haveLowerLayerConfiguration(if_t iface) const;

    static void resolvEventHandlerCb(void* arg, const void* data);
    void resolvEventHandler(const void* data);

    InterfaceRuntimeState* getInterfaceRuntimeState(if_t iface) const;
    void populateInterfaceRuntimeState(bool enabled);
    bool isDisabled(if_t iface);
    void resetInterfaceProtocolState(if_t iface = nullptr);

    struct StoredConfiguration {
        unsigned idx;
        NetworkInterfaceConfig conf;
    };

    int loadStoredConfiguration(spark::Vector<StoredConfiguration>& conf);
    int saveStoredConfiguration(const spark::Vector<StoredConfiguration>& conf);

private:
    if_event_handler_cookie_t ifEventHandlerCookie_ = {};
    resolv_event_handler_cookie_t resolvEventHandlerCookie_ = {};

    std::atomic<State> state_;
    std::atomic<ProtocolState> ip4State_;
    std::atomic<ProtocolState> ip6State_;
    std::atomic<DnsState> dns4State_;
    std::atomic<DnsState> dns6State_;
    std::atomic<NetworkStatus> networkStatus_;

    IntrusiveList<InterfaceRuntimeState> runState_;
};

} } /* particle::system */

#endif /* HAL_PLATFORM_IFAPI */

#endif /* SYSTEM_NETWORK_MANAGER_H */
