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

#include "system_network_manager.h"
#include "system_error.h"
#include <mutex>
#include "system_led_signal.h"
#include "enumclass.h"

#define CHECK(_expr) \
        ({ \
            const auto _ret = _expr; \
            if (_ret < 0) { \
                return _ret; \
            } \
            _ret; \
        })

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

#if HAL_PLATFORM_IFAPI

#if HAL_PLATFORM_OPENTHREAD
#include "system_openthread.h"
#include <openthread/dataset.h>
#include <openthread/thread.h>
#include <openthread/instance.h>
#endif /* HAL_PLATFORM_OPENTHREAD */

using namespace particle::system;

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

} /* anonymous */

NetworkManager::NetworkManager() {
    state_ = State::NONE;
    ip4State_ = ProtocolState::UNCONFIGURED;
    ip6State_ = ProtocolState::UNCONFIGURED;
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

    transition(State::DISABLED);
}

void NetworkManager::destroy() {
    if (ifEventHandlerCookie_) {
        if_event_handler_del(ifEventHandlerCookie_);
        ifEventHandlerCookie_ = nullptr;
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

bool NetworkManager::isNetworkingEnabled() {
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

int NetworkManager::deactivateConnections() {
    switch (state_) {
        case State::DISABLED:
        case State::IFACE_DOWN: {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        default: {
            break;
        }
    }

    transition(State::IFACE_REQUEST_DOWN);

    int waitingFor = 0;

    /* Bring all the interfaces down */
    CHECK(for_each_iface([&](if_t iface, unsigned int flags) {
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

bool NetworkManager::isEstablishingConnections() {
    switch (state_) {
        case State::IFACE_REQUEST_UP:
        case State::IFACE_UP:
        case State::IFACE_LINK_UP: {
            return true;
        }
    }

    return false;
}

bool NetworkManager::isConnectivityAvailable() {
    return state_ == State::IP_CONFIGURED;
}

bool NetworkManager::isIp4ConnectivityAvailable() {
    return ip4State_ == ProtocolState::CONFIGURED;
}

bool NetworkManager::isIp6ConnectivityAvailable() {
    return ip6State_ == ProtocolState::CONFIGURED;
}

bool NetworkManager::isConfigured() {
    bool ret = true;
    for_each_iface([&](if_t iface, unsigned int curFlags) {
        if (ret && !haveLowerLayerConfiguration(iface)) {
            ret = false;
        }
    });

    return ret;
}

int NetworkManager::clearConfiguration(if_t iface) {
    if (state_ != State::IFACE_DOWN && state_ != State::DISABLED) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    if (!iface) {
        for_each_iface([](if_t iface, unsigned int flags) {
            char name[IF_NAMESIZE] = {};
            if_get_name(iface, name);

#if HAL_PLATFORM_OPENTHREAD
            if (!strncmp(name, "th", 2)) {
                /* OpenThread iface */
                std::lock_guard<ThreadLock> lk(ThreadLock());
                otMasterKey key = {};
                otThreadSetMasterKey(threadInstance(), &key);
                otInstanceErasePersistentInfo(threadInstance());
            }
#endif /* HAL_PLATFORM_OPENTHREAD */
        });
    }

    return SYSTEM_ERROR_INVALID_ARGUMENT;
}

NetworkManager::State NetworkManager::getState() const {
    return state_;
}

void NetworkManager::transition(State state) {
    if (state_ == state) {
        /* Just in case */
        LOG_DEBUG(ERROR, "Transitioning to the same state");
    }

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
            ip4State_ = ProtocolState::UNCONFIGURED;
            ip6State_ = ProtocolState::UNCONFIGURED;
            break;
        }
    }

    /* To */
    switch (state) {
        case State::DISABLED: {
            LED_SIGNAL_START(NETWORK_OFF, BACKGROUND);
            break;
        }
        case State::IFACE_DOWN: {
            LED_SIGNAL_START(NETWORK_ON, BACKGROUND);
            break;
        }
        case State::IFACE_REQUEST_DOWN: {
            /* TODO: ? */
            break;
        }
        case State::IFACE_REQUEST_UP: {
            LED_SIGNAL_START(NETWORK_CONNECTING, BACKGROUND);
            break;
        }
        case State::IFACE_UP: {
            LED_SIGNAL_START(NETWORK_CONNECTING, BACKGROUND);
            break;
        }
        case State::IFACE_LINK_UP: {
            LED_SIGNAL_START(NETWORK_DHCP, BACKGROUND);
            break;
        }
        case State::IP_CONFIGURED: {
            LED_SIGNAL_START(NETWORK_CONNECTED, BACKGROUND);
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
            if (countIfacesWithFlags(IFF_UP) == 1) {
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
        }
    } else {
        /* Interface link state changed to DOWN */
        if (state_ == State::IP_CONFIGURED || state_ == State::IFACE_LINK_UP) {
            if (countIfacesWithFlags(IFF_UP | IFF_LOWER_UP) == 0) {
                transition(State::IFACE_UP);
            } else {
                refreshIpState();
            }
        }
    }
}

void NetworkManager::handleIfAddr(if_t iface, const struct if_event* ev) {
    if (state_ == State::IP_CONFIGURED || state_ == State::IFACE_LINK_UP) {
        refreshIpState();
    }
}

void NetworkManager::handleIfLinkLayerAddr(if_t iface, const struct if_event* ev) {
    /* We don't care about this */
}

unsigned int NetworkManager::countIfacesWithFlags(unsigned int flags) {
    unsigned int count = 0;

    for_each_iface([&](if_t iface, unsigned int curFlags) {
        if ((curFlags & flags) == flags) {
            ++count;
        }
    });

    return count;
}

void NetworkManager::refreshIpState() {
    ProtocolState ip4 = ProtocolState::UNCONFIGURED;
    ProtocolState ip6 = ProtocolState::UNCONFIGURED;

    if_addrs* addrs = nullptr;
    CHECKV(if_get_if_addrs(&addrs));

    for (auto addr = addrs; addr != nullptr; addr = addr->next) {
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
                ip4 = ProtocolState::CONFIGURED;
            }
        } else if (a->addr->sa_family == AF_INET6) {
            sockaddr_in6* sin6 = (sockaddr_in6*)a->addr;
            auto ip6_addr_data = a->ip6_addr_data;

            /* NOTE: we say that IPv6 is configured if there is at least
             * one IPv6 address on an interface without scope and in a VALID state,
             * which is in fact either PREFERRED or DEPRECATED.
             */
            if (sin6->sin6_scope_id == 0 && a->prefixlen > 0 &&
                ip6_addr_data && ip6_addr_data->state & IF_IP6_ADDR_STATE_VALID) {
                ip6 = ProtocolState::CONFIGURED;
            }
        } else {
            /* Unknown family */
        }
    }

    if_free_if_addrs(addrs);

    const auto oldIp4State = ip4State_.load();
    const auto oldIp6State = ip6State_.load();

    ip4State_ = ip4;
    ip6State_ = ip6;

    const bool ipConfigured = (ip4 == ProtocolState::CONFIGURED) || (ip6 == ProtocolState::CONFIGURED);

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

bool NetworkManager::haveLowerLayerConfiguration(if_t iface) {
    char name[IF_NAMESIZE] = {};
    if_get_name(iface, name);

#if HAL_PLATFORM_OPENTHREAD
    if (!strncmp(name, "th", 2)) {
        /* OpenThread iface */
        std::lock_guard<ThreadLock> lk(ThreadLock());
        return otDatasetIsCommissioned(threadInstance());
    }
#endif /* HAL_PLATFORM_OPENTHREAD */

    return true;
}


const char* NetworkManager::stateToName(State state) {
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
#endif /* HAL_PLATFORM_IFAPI */
