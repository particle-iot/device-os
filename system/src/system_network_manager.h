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
#include <atomic>

namespace particle { namespace system {

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    void init();
    void destroy();

    int enableNetworking();
    int disableNetworking();
    bool isNetworkingEnabled();

    int activateConnections();
    int deactivateConnections();
    bool isEstablishingConnections();

    bool isConnectivityAvailable();
    bool isIp4ConnectivityAvailable();
    bool isIp6ConnectivityAvailable();

    int enterListeningMode(unsigned int timeout = 0);
    int exitListeningMode();
    bool isInListeningMode();

    int setListeningModeTimeout(unsigned int timeout);
    unsigned int getListeningModeTimeout();

private:
    enum class State {
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

    enum class ProtocolState {
        UNCONFIGURED,
        CONFIGURED
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

    unsigned int countIfacesWithFlags(unsigned int flags, bool ignoreLoopback = true);
    void refreshIpState();

private:
    if_event_handler_cookie_t ifEventHandlerCookie_ = {};

    std::atomic<State> state_;
    std::atomic<ProtocolState> ip4State_;
    std::atomic<ProtocolState> ip6State_;
};


} } /* particle::system */

#endif /* HAL_PLATFORM_IFAPI */

#endif /* SYSTEM_NETWORK_MANAGER_H */
