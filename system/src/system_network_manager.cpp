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

#include "system_network_manager.h"
#include "system_error.h"

#define CHECK(_expr) \
        ({ \
            const auto _ret = _expr; \
            if (_ret < 0) { \
                return _ret; \
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

using namespace particle::system;

namespace {

template <typename F>
int for_each_iface(F&& f) {
    if_list* ifs = nullptr;
    CHECK(if_get_list(&ifs));

    for (if_list* iface = ifs; iface != nullptr; iface = iface->next) {
        f(iface->iface);
    }

    if_free_list(ifs);

    return 0;
}

} /* anonymous */

NetworkManager::NetworkManager() {
    state_ = State::DISABLED;
    ip4State_ = ProtocolState::UNCONFIGURED;
    ip6State_ = ProtocolState::UNCONFIGURED;
}

NetworkManager::~NetworkManager() {
    destroy();
}

void NetworkManager::init() {
    if (!ifEventHandlerCookie_) {
        ifEventHandlerCookie_ = if_event_handler_add(&ifEventHandlerCb, this);
    }
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
    CHECK(for_each_iface([&](if_t iface) -> int {
        unsigned int flags = 0;
        CHECK(if_get_flags(iface, &flags));
        if (!(flags & IFF_LOOPBACK)) {
            if (!(flags & IFF_UP)) {
                CHECK(if_set_flags(iface, IFF_UP));
                /* FIXME */
                CHECK(if_set_xflags(iface, IFXF_DHCP));
                ++waitingFor;
            }
        }
        return 0;
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
    CHECK(for_each_iface([&](if_t iface) -> int {
        unsigned int flags = 0;
        CHECK(if_get_flags(iface, &flags));
        if (!(flags & IFF_LOOPBACK)) {
            if (flags & IFF_UP) {
                CHECK(if_clear_flags(iface, IFF_UP));
                ++waitingFor;
            }
        }
        return 0;
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

int NetworkManager::enterListeningMode(unsigned int timeout) {
    return 0;
}

int NetworkManager::exitListeningMode() {
    return 0;
}

bool NetworkManager::isInListeningMode() {
    return false;
}

int NetworkManager::setListeningModeTimeout(unsigned int timeout) {
    return 0;
}

unsigned int NetworkManager::getListeningModeTimeout() {
    return 0;
}

void NetworkManager::transition(State state) {
    /* From */
    switch (state_) {
        /* Ensure that IPv4/IPv6 protocol state is reset */
        case State::IP_CONFIGURED: {
            ip4State_ = ProtocolState::UNCONFIGURED;
            ip6State_ = ProtocolState::UNCONFIGURED;
            break;
        }
    }

    /* To */
    switch (state) {
    }

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
            if (countIfacesWithFlags(IFF_UP) == 0) {
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

unsigned int NetworkManager::countIfacesWithFlags(unsigned int flags, bool ignoreLoopback) {
    unsigned int count = 0;

    for_each_iface([&](if_t iface) -> int {
        unsigned int curFlags = 0;
        CHECK(if_get_flags(iface, &flags));
        if (!ignoreLoopback || !(curFlags & IFF_LOOPBACK)) {
            if ((curFlags & flags) == flags) {
                ++count;
            }
        }
        return 0;
    });

    return count;
}

void NetworkManager::refreshIpState() {
    /* TODO */
}

#endif /* HAL_PLATFORM_IFAPI */
