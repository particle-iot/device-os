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

#include "logging.h"
LOG_SOURCE_CATEGORY("system.nm")

#undef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ALL

#include "hal_platform.h"

#if HAL_PLATFORM_IFAPI

#include "system_network_manager.h"
#include "system_error.h"
#include <mutex>
#include "system_led_signal.h"
#include "enumclass.h"
#if HAL_PLATFORM_WIFI && HAL_PLATFORM_NCP
#include "network/ncp/wifi/ncp.h"
#include "network/ncp/wifi/wifi_network_manager.h"
#endif // HAL_PLATFORM_WIFI && HAL_PLATFORM_NCP
#if HAL_PLATFORM_NCP && HAL_PLATFORM_CELLULAR
#include "cellular_hal.h"
#endif // HAL_PLATFORM_NCP && HAL_PLATFORM_CELLULAR
#include "check.h"
#include "system_cloud.h"
#include "system_threading.h"
#include "system_event.h"
#include "timer_hal.h"
#include "delay_hal.h"
#include "system_network_diagnostics.h"

#define CHECKV(_expr) \
        ({ \
            const auto _ret = _expr; \
            if (_ret < 0) { \
                return; \
            } \
            _ret; \
        })

#define CHECK_CONT(_expr) \
        ({ \
            const auto _ret = _expr; \
            if (_ret < 0) { \
                continue; \
            } \
            _ret; \
        })

namespace particle { namespace system {

namespace {

template <typename F>
int for_each_iface(F&& f) {
    if_list* ifs = nullptr;
    CHECK(if_get_list(&ifs));

    for (if_list* iface = ifs; iface != nullptr; iface = iface->next) {
        if (iface->iface) {
            unsigned int flags = 0;
            CHECK_CONT(if_get_flags(iface->iface, &flags));
            if (!(flags & IFF_LOOPBACK)) {
                f(iface->iface, flags);
            }
        }
    }

    if_free_list(ifs);

    return 0;
}

void forceCloudPingIfConnected() {
    const auto task = new(std::nothrow) ISRTaskQueue::Task();
    if (!task) {
        return;
    }
    task->func = [](ISRTaskQueue::Task* task) {
        delete task;
        if (spark_cloud_flag_connected()) {
            spark_protocol_command(system_cloud_protocol_instance(), ProtocolCommands::PING, 0, nullptr);
        }
    };
    SystemISRTaskQueue.enqueue(task);
}

} /* anonymous */

NetworkManager::NetworkManager() {
    state_ = State::NONE;
    ip4State_ = ProtocolState::UNCONFIGURED;
    ip6State_ = ProtocolState::UNCONFIGURED;
    dns4State_ = DnsState::UNCONFIGURED;
    dns6State_ = DnsState::UNCONFIGURED;
    networkStatus_ = NetworkStatus::NETWORK_STATUS_OFF;
}

NetworkManager::~NetworkManager() {
    destroy();
}

NetworkManager* NetworkManager::instance() {
    static NetworkManager man;
    return &man;
}

void NetworkManager::init() {
    if (!ifEventHandlerCookie_) {
        ifEventHandlerCookie_ = if_event_handler_add(&ifEventHandlerCb, this);
    }

    if (!resolvEventHandlerCookie_) {
        resolvEventHandlerCookie_ = resolv_event_handler_add(&resolvEventHandlerCb, this);
    }

    transition(State::DISABLED);
}

void NetworkManager::destroy() {
    if (ifEventHandlerCookie_) {
        if_event_handler_del(ifEventHandlerCookie_);
        ifEventHandlerCookie_ = nullptr;
    }

    if (resolvEventHandlerCookie_) {
        resolv_event_handler_del(resolvEventHandlerCookie_);
        resolvEventHandlerCookie_ = nullptr;
    }
}

int NetworkManager::enableNetworking() {
    /* TODO: this should power on all the network hardware if possible */
    if (state_ == State::DISABLED) {
        transition(State::IFACE_DOWN);
    }
    return SYSTEM_ERROR_INVALID_STATE;
}

int NetworkManager::disableNetworking() {
    /* TODO: this should power off all the network hardware if possible */
    switch (state_) {
        case State::DISABLED: {
            return 0;
        }
        case State::IFACE_DOWN: {
            /* Networking is enabled, but no interfaces were brought up. Nothing to do here */
            transition(State::DISABLED);
            break;
        }
    }
    return SYSTEM_ERROR_INVALID_STATE;
}

bool NetworkManager::isNetworkingEnabled() const {
    return state_ != State::DISABLED;
}

int NetworkManager::activateConnections() {
    if (state_ != State::IFACE_DOWN) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    /* Get a list of network interfaces */
    transition(State::IFACE_REQUEST_UP);

    int waitingFor = 0;

    /* Bring all the interfaces up */
    CHECK(for_each_iface([&](if_t iface, unsigned int flags) {
        /* Skip interfaces that don't have configuration */
        if (!haveLowerLayerConfiguration(iface)) {
            return;
        }

        // Ignore disabled interfaces
        if (!isInterfaceEnabled(iface)) {
            return;
        }

        if (!(flags & IFF_UP)) {
            CHECKV(if_set_flags(iface, IFF_UP));

            /* FIXME */
            CHECKV(if_set_xflags(iface, IFXF_DHCP));

            /* TODO: establish lower layer connection, e.g. 802.11 */

            ++waitingFor;
        }
    }));

    if (!waitingFor) {
        /* No interfaces needed to be brought up */
        transition(State::IFACE_UP);
    }

    return 0;
}

int NetworkManager::deactivateConnections(network_disconnect_reason reason) {
    switch (state_) {
        case State::DISABLED:
        case State::IFACE_DOWN: {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        default: {
            break;
        }
    }

    if (isConnectivityAvailable()) {
        const auto diag = NetworkDiagnostics::instance();
        if (reason != NETWORK_DISCONNECT_REASON_NONE) {
            diag->disconnectionReason(reason);
            if (reason == NETWORK_DISCONNECT_REASON_ERROR || reason == NETWORK_DISCONNECT_REASON_RESET) {
                diag->disconnectedUnexpectedly();
            }
        }
    }
    transition(State::IFACE_REQUEST_DOWN);

    int waitingFor = 0;

    /* Bring all the interfaces down */
    CHECK(for_each_iface([&](if_t iface, unsigned int flags) {
        /* Skip interfaces that don't have configuration
         * FIXME: Do we need this here as well?
         */
        // if (!haveLowerLayerConfiguration(iface)) {
        //     return;
        // }

        if (flags & IFF_UP) {
            CHECKV(if_clear_flags(iface, IFF_UP));
            ++waitingFor;
        }
    }));

    if (!waitingFor) {
        /* No interfaces needed to be brought down */
        transition(State::IFACE_DOWN);
    }

    return 0;
}

bool NetworkManager::isEstablishingConnections() const {
    switch (state_) {
        case State::IFACE_REQUEST_UP:
        case State::IFACE_UP:
        case State::IFACE_LINK_UP: {
            return true;
        }
    }

    return false;
}

bool NetworkManager::isConnectivityAvailable() const {
    return state_ == State::IP_CONFIGURED;
}

bool NetworkManager::isIp4ConnectivityAvailable() const {
    return ip4State_ == ProtocolState::CONFIGURED;
}

bool NetworkManager::isIp6ConnectivityAvailable() const {
    return ip6State_ == ProtocolState::CONFIGURED;
}

bool NetworkManager::isConfigured(if_t iface) const {
    bool ret = false;
    if (!iface) {
        for_each_iface([&](if_t iface, unsigned int curFlags) {
            if (haveLowerLayerConfiguration(iface)) {
                ret = true;
            }
        });
        return ret;
    } else {
        return haveLowerLayerConfiguration(iface);
    }
}

int NetworkManager::clearConfiguration(if_t oIface) {
    if (!isConfigured()) {
        return SYSTEM_ERROR_NONE;
    }

    int ret = SYSTEM_ERROR_INVALID_ARGUMENT;

    for_each_iface([&](if_t iface, unsigned int flags) {
        char name[IF_NAMESIZE] = {};
        if_get_name(iface, name);

        if (oIface && iface != oIface) {
            return;
        }

#if HAL_PLATFORM_NCP && HAL_PLATFORM_WIFI
        else if (!strncmp(name, "wl", 2)) {
            auto wifiMan = wifiNetworkManager();
            wifiMan->clearNetworkConfig();
            ret = SYSTEM_ERROR_NONE;
        }
#endif // HAL_PLATFORM_NCP && HAL_PLATFORM_WIFI
#if HAL_PLATFORM_NCP && HAL_PLATFORM_CELLULAR
        else if (!strncmp(name, "pp", 2)) {
            ret = cellular_credentials_clear(nullptr);
        }
#endif // HAL_PLATFORM_NCP && HAL_PLATFORM_CELLULAR
    });

    if (!ret) {
        system_notify_event(network_credentials, network_credentials_cleared);
    }

    return ret;
}

NetworkManager::State NetworkManager::getState() const {
    return state_;
}

void NetworkManager::transition(State state) {
    /* From */
    switch (state_) {
        case State::DISABLED: {
            break;
        }
        case State::IFACE_DOWN: {
            break;
        }
        case State::IFACE_REQUEST_DOWN: {
            break;
        }
        case State::IFACE_REQUEST_UP: {
            break;
        }
        case State::IFACE_UP: {
            break;
        }
        case State::IFACE_LINK_UP: {
            break;
        }
        /* Ensure that IPv4/IPv6 protocol state is reset */
        case State::IP_CONFIGURED: {
            if (state != State::IP_CONFIGURED) {
                ip4State_ = ProtocolState::UNCONFIGURED;
                ip6State_ = ProtocolState::UNCONFIGURED;
                dns4State_ = DnsState::UNCONFIGURED;
                dns6State_ = DnsState::UNCONFIGURED;
            }
            break;
        }
    }

    /* To */
    switch (state) {
        case State::DISABLED: {
            networkStatus_ = NetworkStatus::NETWORK_STATUS_POWERING_OFF;
            LED_SIGNAL_START(NETWORK_OFF, BACKGROUND);
            // FIXME: turning-off events/diagnostics
            system_notify_event(network_status, network_status_powering_off);
            NetworkDiagnostics::instance()->status(NetworkDiagnostics::TURNED_OFF);
            break;
        }
        case State::IFACE_DOWN: {
            LED_SIGNAL_START(NETWORK_ON, BACKGROUND);
            NetworkDiagnostics::instance()->status(NetworkDiagnostics::DISCONNECTED);
            if (state_ == State::IFACE_REQUEST_DOWN) {
                networkStatus_ = NetworkStatus::NETWORK_STATUS_DISCONNECTED;
                system_notify_event(network_status, network_status_disconnected);
            } else if (state_ == State::DISABLED) {
                networkStatus_ = NetworkStatus::NETWORK_STATUS_POWERING_ON;
                // FIXME:
                system_notify_event(network_status, network_status_powering_on);
            }
            break;
        }
        case State::IFACE_REQUEST_DOWN: {
            networkStatus_ = NetworkStatus::NETWORK_STATUS_DISCONNECTING;
            if (state_ == State::IP_CONFIGURED) {
                NetworkDiagnostics::instance()->resetConnectionAttempts();
            }
            system_notify_event(network_status, network_status_disconnecting);
            NetworkDiagnostics::instance()->status(NetworkDiagnostics::DISCONNECTING);
            break;
        }
        case State::IFACE_REQUEST_UP: {
            LED_SIGNAL_START(NETWORK_CONNECTING, BACKGROUND);
            NetworkDiagnostics::instance()->status(NetworkDiagnostics::CONNECTING);
            break;
        }
        case State::IFACE_UP: {
            networkStatus_ = NetworkStatus::NETWORK_STATUS_CONNECTING;
            if (state_ == State::IP_CONFIGURED) {
                NetworkDiagnostics::instance()->disconnectionReason(NETWORK_DISCONNECT_REASON_ERROR);
                NetworkDiagnostics::instance()->disconnectedUnexpectedly();
            }
            LED_SIGNAL_START(NETWORK_CONNECTING, BACKGROUND);
            system_notify_event(network_status, network_status_connecting);
            NetworkDiagnostics::instance()->status(NetworkDiagnostics::CONNECTING);
            NetworkDiagnostics::instance()->connectionAttempt();
            break;
        }
        case State::IFACE_LINK_UP: {
            LED_SIGNAL_START(NETWORK_DHCP, BACKGROUND);
            break;
        }
        case State::IP_CONFIGURED: {
            networkStatus_ = NetworkStatus::NETWORK_STATUS_CONNECTED;
            LED_SIGNAL_START(NETWORK_CONNECTED, BACKGROUND);
            NetworkDiagnostics::instance()->status(NetworkDiagnostics::CONNECTED);
            if (state_ != State::IP_CONFIGURED) {
                system_notify_event(network_status, network_status_connected);
            }
            break;
        }
    }

    LOG(INFO, "State changed: %s -> %s", stateToName(state_), stateToName(state));

    state_ = state;
}

void NetworkManager::ifEventHandlerCb(void* arg, if_t iface, const struct if_event* ev) {
    auto self = static_cast<NetworkManager*>(arg);
    self->ifEventHandler(iface, ev);
}

void NetworkManager::ifEventHandler(if_t iface, const struct if_event* ev) {
    switch (ev->ev_type) {
        case IF_EVENT_IF_ADDED: {
            handleIfAdded(iface, ev);
            break;
        }
        case IF_EVENT_IF_REMOVED: {
            handleIfRemoved(iface, ev);
            break;
        }
        case IF_EVENT_STATE: {
            handleIfState(iface, ev);
            break;
        }
        case IF_EVENT_LINK: {
            handleIfLink(iface, ev);
            break;
        }
        case IF_EVENT_ADDR: {
            handleIfAddr(iface, ev);
            break;
        }
        case IF_EVENT_LLADDR: {
            handleIfLinkLayerAddr(iface, ev);
            break;
        }
        case IF_EVENT_POWER_STATE: {
            handleIfPowerState(iface, ev);
            break;
        }
        case IF_EVENT_PHY_STATE: {
            handleIfPhyState(iface, ev);
            break;
        }
    }
}

void NetworkManager::handleIfAdded(if_t iface, const struct if_event* ev) {
    /* TODO */
}

void NetworkManager::handleIfRemoved(if_t iface, const struct if_event* ev) {
    /* TODO */
}

void NetworkManager::handleIfState(if_t iface, const struct if_event* ev) {
    if (ev->ev_if_state->state) {
        /* Interface administrative state changed to UP */
        if (state_ == State::IFACE_REQUEST_UP) {
            transition(State::IFACE_UP);

            /* FIXME: is this needed? */
            if (countIfacesWithFlags(IFF_LOWER_UP) > 0) {
                transition(State::IFACE_LINK_UP);
            }
        }
    } else {
        /* Interface administrative state changed to DOWN */
        if (state_ == State::IFACE_REQUEST_DOWN) {
            /* FIXME: LwIP issues netif_set_down callback BEFORE clearing the IFF_UP flag,
             * that's why the count of interfaces with IFF_UP should be 1 here
             */
            if (countIfacesWithFlags(IFF_UP) <= 1) {
                transition(State::IFACE_DOWN);
            }
        }
    }
}

void NetworkManager::handleIfLink(if_t iface, const struct if_event* ev) {
    if (ev->ev_if_link->state) {
        /* Interface link state changed to UP */
        if (state_ == State::IFACE_UP) {
            transition(State::IFACE_LINK_UP);
            refreshIpState();
        } else if (state_ == State::IP_CONFIGURED || state_ == State::IFACE_LINK_UP) {
            refreshIpState();
        }
    } else {
        resetInterfaceProtocolState(iface);
        /* Interface link state changed to DOWN */
        if (state_ == State::IP_CONFIGURED || state_ == State::IFACE_LINK_UP) {
            if (countIfacesWithFlags(IFF_UP | IFF_LOWER_UP) == 0) {
                transition(State::IFACE_UP);
            } else {
                refreshIpState();
            }
        }
    }
    forceCloudPingIfConnected();
}

void NetworkManager::handleIfAddr(if_t iface, const struct if_event* ev) {
    if (state_ == State::IP_CONFIGURED || state_ == State::IFACE_LINK_UP) {
        refreshIpState();
    }
    forceCloudPingIfConnected();
}

void NetworkManager::handleIfLinkLayerAddr(if_t iface, const struct if_event* ev) {
    /* We don't care about this */
}

unsigned int NetworkManager::countIfacesWithFlags(unsigned int flags) const {
    unsigned int count = 0;

    for_each_iface([&](if_t iface, unsigned int curFlags) {
        /* Skip interfaces that don't have configuration */
        if (!haveLowerLayerConfiguration(iface)) {
            return;
        }

        // Ignore disabled interfaces
        if (!isInterfaceEnabled(iface)) {
            return;
        }

        if ((curFlags & flags) == flags) {
            ++count;
        }
    });

    return count;
}

void NetworkManager::handleIfPowerState(if_t iface, const struct if_event* ev) {
    // FIXME: this is not really thread-safe, but should probably work
    auto state = getInterfaceRuntimeState(iface);
    if (!state) {
        LOG(ERROR, "Interface is not populated");
        return;
    }
    if (state->pwrState != ev->ev_power_state->state) {
        state->pwrState = static_cast<if_power_state_t>(ev->ev_power_state->state);
        uint8_t index;
        if_get_index(iface, &index);
        LOG(TRACE, "Interface %d power state changed: %d", index, state->pwrState.load());
    }
}

// FIXME: this currently works somewhat properly only if there is just 1 network interface.
void NetworkManager::handleIfPhyState(if_t iface, const struct if_event* ev) {
    if (networkStatus_ == NetworkStatus::NETWORK_STATUS_POWERING_ON && ev->ev_phy_state->state == IF_PHY_STATE_ON) {
        system_notify_event(network_status, network_status_on);
    } else if (networkStatus_ == NetworkStatus::NETWORK_STATUS_POWERING_OFF && ev->ev_phy_state->state == IF_PHY_STATE_OFF) {
        system_notify_event(network_status, network_status_off);
    }
}

void NetworkManager::refreshIpState() {
    ProtocolState ip4 = ProtocolState::UNCONFIGURED;
    ProtocolState ip6 = ProtocolState::UNCONFIGURED;

    resetInterfaceProtocolState();

    if_addrs* addrs = nullptr;
    CHECKV(if_get_if_addrs(&addrs));

    for (auto addr = addrs; addr != nullptr; addr = addr->next) {
        ProtocolState ifIp4 = ProtocolState::UNCONFIGURED;
        ProtocolState ifIp6 = ProtocolState::UNCONFIGURED;

        /* Skip loopback interface */
        if (addr->ifflags & IFF_LOOPBACK) {
            continue;
        }

        /* Skip non-UP and non-LINK_UP interfaces */
        if ((addr->ifflags & (IFF_UP | IFF_LOWER_UP)) != (IFF_UP | IFF_LOWER_UP)) {
            continue;
        }

        auto a = addr->if_addr;
        if (!a || !a->addr) {
            continue;
        }

        if (a->addr->sa_family == AF_INET) {
            if (a->prefixlen > 0 && /* FIXME */ a->gw) {
                ifIp4 = ProtocolState::CONFIGURED;
            }
        } else if (a->addr->sa_family == AF_INET6) {
            sockaddr_in6* sin6 = (sockaddr_in6*)a->addr;
            auto ip6_addr_data = a->ip6_addr_data;

            /* NOTE: we say that IPv6 is configured if there is at least
             * one IPv6 address on an interface without scope and in a VALID state,
             * which is in fact either PREFERRED or DEPRECATED.
             */
            if (sin6->sin6_scope_id == 0 && a->prefixlen > 0 &&
                    ip6_addr_data && (ip6_addr_data->state & IF_IP6_ADDR_STATE_VALID)) {
                ifIp6 = ProtocolState::CONFIGURED;
            } else if (sin6->sin6_scope_id != 0 && a->prefixlen > 0 &&
                    ip6_addr_data && (ip6_addr_data->state & IF_IP6_ADDR_STATE_VALID)) {
                // Otherwise report link-local
                ifIp6 = ProtocolState::LINKLOCAL;
            }
        } else {
            /* Unknown family */
        }

        if ((int)ifIp4 > (int)ip4) {
            ip4 = ifIp4;
        }
        if ((int)ifIp6 > (int)ip6) {
            ip6 = ifIp6;
        }

        {
            if_t iface;
            if (!if_get_by_index(addr->ifindex, &iface)) {
                auto state = getInterfaceRuntimeState(iface);
                if (state) {
                    if ((int)ifIp4 > (int)state->ip4State.load()) {
                        state->ip4State = ifIp4;
                    }
                    if ((int)ifIp6 > (int)state->ip6State.load()) {
                        state->ip6State = ifIp6;
                    }
                }
            }
        }
    }

    if_free_if_addrs(addrs);

    const auto oldIp4State = ip4State_.load();
    const auto oldIp6State = ip6State_.load();

    refreshDnsState();

    ip4State_ = ip4;
    ip6State_ = ip6;

    const bool ipConfigured = (ip4 == ProtocolState::CONFIGURED && dns4State_ == DnsState::CONFIGURED) ||
            (ip6 == ProtocolState::CONFIGURED && dns6State_ == DnsState::CONFIGURED);

    if (state_ == State::IP_CONFIGURED && !ipConfigured) {
        transition(State::IFACE_LINK_UP);
    } else if (state_ == State::IFACE_LINK_UP && ipConfigured) {
        transition(State::IP_CONFIGURED);
    } else if (state_ == State::IP_CONFIGURED && ipConfigured) {
        /* Transition to the same state here. Once we implement events this will be relevant */
        if (ip4 != oldIp4State || ip6 != oldIp6State) {
            transition(State::IP_CONFIGURED);
        }
    }
}

void NetworkManager::refreshDnsState() {
    resolv_dns_servers* servers = nullptr;
    CHECKV(resolv_get_dns_servers(&servers));

    DnsState ip4 = DnsState::UNCONFIGURED;
    DnsState ip6 = DnsState::UNCONFIGURED;

    for (auto server = servers; server != nullptr; server = server->next) {
        if (!server || !server->server) {
            continue;
        }

        if (server->server->sa_family == AF_INET) {
            ip4 = DnsState::CONFIGURED;
        } else if (server->server->sa_family == AF_INET6) {
            ip6 = DnsState::CONFIGURED;
        }
    }

    resolv_free_dns_servers(servers);

    dns4State_ = ip4;
    dns6State_ = ip6;
}

bool NetworkManager::haveLowerLayerConfiguration(if_t iface) const {
    char name[IF_NAMESIZE] = {};
    if_get_name(iface, name);

#if HAL_PLATFORM_WIFI && HAL_PLATFORM_NCP
    if (!strncmp(name, "wl", 2)) {
        auto wifiMan = wifiNetworkManager();
        return wifiMan->hasNetworkConfig();
    }
#endif // HAL_PLATFORM_WIFI

    return true;
}

void NetworkManager::resolvEventHandlerCb(void* arg, const void* data) {
    auto self = static_cast<NetworkManager*>(arg);
    self->resolvEventHandler(data);
}

void NetworkManager::resolvEventHandler(const void* data) {
    refreshIpState();
    // NOTE: we could potentially force a cloud ping on DNS change, but
    // this seems excessive, and it's better to rely on IP state only instead
    // forceCloudPingIfConnected();
}

const char* NetworkManager::stateToName(State state) const {
    static const char* const stateNames[] = {
        "NONE",
        "DISABLED",
        "IFACE_DOWN",
        "IFACE_REQUEST_DOWN",
        "IFACE_REQUEST_UP",
        "IFACE_UP",
        "IFACE_LINK_UP",
        "IP_CONFIGURED"
    };

    return stateNames[::particle::to_underlying(state)];
}

void NetworkManager::populateInterfaceRuntimeState(bool st) {
    for_each_iface([&](if_t iface, unsigned int flags) {
        auto state = getInterfaceRuntimeState(iface);
        if (!state) {
            state = new (std::nothrow) InterfaceRuntimeState;
            if (state) {
                state->iface = iface;
                runState_.pushFront(state);
            }
        }
        if (state) {
            state->enabled = st;
            if_power_state_t pwr = IF_POWER_STATE_NONE;
            if (if_get_power_state(iface, &pwr) != SYSTEM_ERROR_NONE) {
                return;
            }
            if (state->pwrState != pwr) {
                state->pwrState = pwr;
                uint8_t index;
                if_get_index(iface, &index);
                LOG(TRACE, "Interface %d power state: %d", index, state->pwrState.load());
            }
        }
    });
}

int NetworkManager::enableInterface(if_t iface) {
    // Special case - enable all
    if (iface == nullptr) {
        populateInterfaceRuntimeState(true);
    } else {
        auto state = getInterfaceRuntimeState(iface);
        CHECK_TRUE(state, SYSTEM_ERROR_NOT_FOUND);
        state->enabled = true;
    }
    return 0;
}

int NetworkManager::disableInterface(if_t iface, network_disconnect_reason reason) {
    // XXX: This method is only called on platforms with multiple network interfaces as a user
    // request to disable a particular network interface, if there are multiple _active_ network interfaces
    // at the moment, so 'reason' will always contain NETWORK_DISCONNECT_REASON_USER.
    // While disabling a non-primary network interface will not in fact result in a logical disconnect,
    // it might still be a good idea to log a user request here. We can revise this behavior later.
    NetworkDiagnostics::instance()->disconnectionReason(reason);
    // Special case - disable all
    if (iface == nullptr) {
        populateInterfaceRuntimeState(false);
    } else {
        auto state = getInterfaceRuntimeState(iface);
        CHECK_TRUE(state, SYSTEM_ERROR_NOT_FOUND);
        state->enabled = false;
    }
    return 0;
}

int NetworkManager::syncInterfaceStates() {
    if (isEstablishingConnections() || isConnectivityAvailable()) {
        CHECK(for_each_iface([&](if_t iface, unsigned int flags) {
            if (!haveLowerLayerConfiguration(iface)) {
                return;
            }

            auto state = getInterfaceRuntimeState(iface);
            if (state) {
                if (state->enabled && !(flags & IFF_UP)) {
                    CHECKV(if_set_flags(iface, IFF_UP));
                    CHECKV(if_set_xflags(iface, IFXF_DHCP));
                } else if (!state->enabled && (flags & IFF_UP)) {
                    CHECKV(if_clear_flags(iface, IFF_UP));
                }
            }
        }));
    }

    return 0;
}

NetworkManager::InterfaceRuntimeState* NetworkManager::getInterfaceRuntimeState(if_t iface) const {
    for (auto item = runState_.front(); item != nullptr; item = item->next) {
        if (item->iface == iface) {
            return item;
        }
    }
    return nullptr;
}

bool NetworkManager::isInterfacePowerState(if_t iface, if_power_state_t state) const {
    bool ret = false;
    if_power_state_t pwr = IF_POWER_STATE_NONE;
    if (!iface) {
        /* This function checks each network interface for the intended power state,
         * and returns true as long as one that matches the desired state is found */
        for_each_iface([&](if_t iface, unsigned int curFlags) {
            if (if_get_power_state(iface, &pwr) != SYSTEM_ERROR_NONE) {
                return;
            }
            if (pwr == state) {
                ret = true;
            }
        });
        return ret;
    } else {
        if (if_get_power_state(iface, &pwr) != SYSTEM_ERROR_NONE) {
            return false;
        }
        ret = (pwr == state) ? true : false;
    }
    return ret;
}

// Note: This is not the same as what wiring ready() does.
bool NetworkManager::isInterfacePhyReady(if_t iface) const {
    bool ret = false;
    unsigned int xflags = 0;
    if (!iface) {
        /* This function checks each network interface if physical state is ready,
         * and returns true as long as one that matches the desired state is found */
        for_each_iface([&](if_t iface, unsigned int curFlags) {
            xflags = 0;
            if (if_get_xflags(iface, &xflags) != SYSTEM_ERROR_NONE) {
                return;
            }
            if (xflags & IFXF_READY) {
                ret = true;
            }
        });
        return ret;
    } else {
        if (if_get_xflags(iface, &xflags) != SYSTEM_ERROR_NONE) {
            return false;
        }
        ret = (xflags & IFXF_READY) ? true : false;
    }
    return ret;
}

bool NetworkManager::isInterfaceOn(if_t iface) const {
    return isInterfacePowerState(iface, IF_POWER_STATE_UP) && isInterfacePhyReady(iface);
}

bool NetworkManager::isInterfaceOff(if_t iface) const {
    return isInterfacePowerState(iface, IF_POWER_STATE_DOWN) && !isInterfacePhyReady(iface);
}

bool NetworkManager::isInterfaceEnabled(if_t iface) const {
    auto state = getInterfaceRuntimeState(iface);
    if (state) {
        return state->enabled;
    }
    return false;
}

int NetworkManager::countEnabledInterfaces() {
    int count = 0;
    for (auto item = runState_.front(); item != nullptr; item = item->next) {
        if (item->enabled && haveLowerLayerConfiguration(item->iface)) {
            ++count;
        }
    }

    return count;
}

NetworkManager::ProtocolState NetworkManager::getInterfaceIp4State(if_t iface) const {
    auto state = getInterfaceRuntimeState(iface);
    if (state) {
        return state->ip4State;
    }

    return ProtocolState::UNCONFIGURED;
}

NetworkManager::ProtocolState NetworkManager::getInterfaceIp6State(if_t iface) const {
    auto state = getInterfaceRuntimeState(iface);
    if (state) {
        return state->ip6State;
    }

    return ProtocolState::UNCONFIGURED;
}

void NetworkManager::resetInterfaceProtocolState(if_t iface) {
    for (auto item = runState_.front(); item != nullptr; item = item->next) {
        if (!iface || item->iface == iface) {
            item->ip4State = ProtocolState::UNCONFIGURED;
            item->ip6State = ProtocolState::UNCONFIGURED;
        }
    }
}

int NetworkManager::powerInterface(if_t iface, bool enable) {
    auto ifState = getInterfaceRuntimeState(iface);
    if (!ifState) {
        LOG(ERROR, "Interface is not populated");
        return SYSTEM_ERROR_NOT_FOUND;
    }
    if_req_power req = {};
    if (enable) {
        req.state = IF_POWER_STATE_UP;
        // Update the interface power here to avoid race condition
        // The power state will be updated on NCP power events
        // FIXME:
        if (ifState->pwrState != IF_POWER_STATE_UP && ifState->pwrState != IF_POWER_STATE_POWERING_UP) {
            ifState->pwrState = IF_POWER_STATE_POWERING_UP;
        }
        LOG(TRACE, "Request to power on the interface");
    } else {
        req.state = IF_POWER_STATE_DOWN;
        // Update the interface power here to avoid race condition
        // The power state will be updated on NCP power events
        // FIXME:
        if (ifState->pwrState != IF_POWER_STATE_DOWN && ifState->pwrState != IF_POWER_STATE_POWERING_DOWN) {
            ifState->pwrState = IF_POWER_STATE_POWERING_DOWN;
        }
        LOG(TRACE, "Request to power off the interface");
    }
    return if_request(iface, IF_REQ_POWER_STATE, &req, sizeof(req), nullptr);
}

int NetworkManager::waitInterfaceOff(if_t iface, system_tick_t timeout) const {
    auto state = getInterfaceRuntimeState(iface);
    CHECK_TRUE(state, SYSTEM_ERROR_NOT_FOUND);
    if (state->pwrState == IF_POWER_STATE_DOWN) {
        return SYSTEM_ERROR_NONE;
    }
    system_tick_t now = HAL_Timer_Get_Milli_Seconds();
    while (state->pwrState != IF_POWER_STATE_DOWN) {
        HAL_Delay_Milliseconds(100);
        if (HAL_Timer_Get_Milli_Seconds() - now > timeout) {
            break;
        }
    }
    if (state->pwrState != IF_POWER_STATE_DOWN) {
        return SYSTEM_ERROR_TIMEOUT;
    }
    return SYSTEM_ERROR_NONE;
}

}} /* namespace particle::system */

#endif /* HAL_PLATFORM_IFAPI */
