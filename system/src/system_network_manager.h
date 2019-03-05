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
    int deactivateConnections();
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

    bool isConfigured(if_t iface = nullptr) const;
    int clearConfiguration(if_t iface = nullptr);

    int enableInterface(if_t iface = nullptr);
    int disableInterface(if_t iface = nullptr);
    bool isInterfaceEnabled(if_t iface) const;
    int countEnabledInterfaces();
    int syncInterfaceStates();

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

protected:
    NetworkManager();

private:

    const char* stateToName(State state) const;

    enum class DnsState {
        UNCONFIGURED,
        CONFIGURED
    };

    struct InterfaceRuntimeState {
        InterfaceRuntimeState()
                : ip4State(ProtocolState::UNCONFIGURED),
                  ip6State(ProtocolState::UNCONFIGURED) {
        }
        InterfaceRuntimeState* next = nullptr;
        bool enabled = false;
        if_t iface = nullptr;
        std::atomic<ProtocolState> ip4State;
        std::atomic<ProtocolState> ip6State;
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

    unsigned int countIfacesWithFlags(unsigned int flags) const;
    void refreshIpState();
    void refreshDnsState();

    bool haveLowerLayerConfiguration(if_t iface) const;

    static void resolvEventHandlerCb(void* arg, const void* data);
    void resolvEventHandler(const void* data);

    InterfaceRuntimeState* getInterfaceRuntimeState(if_t iface) const;
    void populateInterfaceRuntimeState(bool enabled);
    bool isDisabled(if_t iface);
    void resetInterfaceProtocolState(if_t iface = nullptr);

private:
    if_event_handler_cookie_t ifEventHandlerCookie_ = {};
    resolv_event_handler_cookie_t resolvEventHandlerCookie_ = {};

    std::atomic<State> state_;
    std::atomic<ProtocolState> ip4State_;
    std::atomic<ProtocolState> ip6State_;
    std::atomic<DnsState> dns4State_;
    std::atomic<DnsState> dns6State_;

    IntrusiveList<InterfaceRuntimeState> runState_;
};

#if HAL_PLATFORM_MESH
void setBorderRouterPermitted(bool permitted);
#endif /* HAL_PLATFORM_MESH */

} } /* particle::system */

#endif /* HAL_PLATFORM_IFAPI */

#endif /* SYSTEM_NETWORK_MANAGER_H */
