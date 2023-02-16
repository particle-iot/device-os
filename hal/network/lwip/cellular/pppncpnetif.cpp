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
LOG_SOURCE_CATEGORY("net.pppncp")

#include "pppncpnetif.h"
#include <lwip/opt.h>
#include <lwip/mem.h>
#include <lwip/pbuf.h>
#include <lwip/ethip6.h>
#include <lwip/etharp.h>
#include "delay_hal.h"
#include "timer_hal.h"
#include "gpio_hal.h"
#include "service_debug.h"
#include <netif/ethernet.h>
#include <lwip/netifapi.h>
#include <lwip/dhcp.h>
#include <lwip/dns.h>
#include <algorithm>
#include "lwiplock.h"
#include "interrupts_hal.h"
#include "ringbuffer.h"
#include "network/ncp/cellular/cellular_ncp_client.h"
#include "concurrent_hal.h"
#include "platform_config.h"
#include "memp_hook.h"

using namespace particle::net;

namespace {

// 5 minutes
const unsigned PPP_CONNECT_TIMEOUT = 5 * 60 * 1000;

int pppErrorToSystem(int err) {
    using namespace particle::net::ppp;
    switch(err) {
        case Client::ERROR_NONE: {
            return SYSTEM_ERROR_NONE;
        }
        case Client::ERROR_PARAM: {
            return SYSTEM_ERROR_PPP_PARAM;
        }
        case Client::ERROR_OPEN: {
            return SYSTEM_ERROR_PPP_OPEN;
        }
        case Client::ERROR_DEVICE: {
            return SYSTEM_ERROR_PPP_DEVICE;
        }
        case Client::ERROR_ALLOC: {
            return SYSTEM_ERROR_PPP_ALLOC;
        }
        case Client::ERROR_USER: {
            return SYSTEM_ERROR_PPP_USER;
        }
        case Client::ERROR_CONNECT: {
            return SYSTEM_ERROR_PPP_CONNECT;
        }
        case Client::ERROR_AUTHFAIL: {
            return SYSTEM_ERROR_PPP_AUTH_FAIL;
        }
        case Client::ERROR_PROTOCOL: {
            return SYSTEM_ERROR_PPP_PROTOCOL;
        }
        case Client::ERROR_PEERDEAD: {
            return SYSTEM_ERROR_PPP_PEER_DEAD;
        }
        case Client::ERROR_IDLETIMEOUT: {
            return SYSTEM_ERROR_PPP_IDLE_TIMEOUT;
        }
        case Client::ERROR_CONNECTTIME: {
            return SYSTEM_ERROR_PPP_CONNECT_TIME;
        }
        case Client::ERROR_LOOPBACK: {
            return SYSTEM_ERROR_PPP_LOOPBACK;
        }
        case Client::ERROR_NO_CARRIER_IN_NETWORK_PHASE: {
            return SYSTEM_ERROR_PPP_NO_CARRIER_IN_NETWORK_PHASE;
        }
    }

    return SYSTEM_ERROR_UNKNOWN;
}

} // anonymous


PppNcpNetif::PppNcpNetif()
        : BaseNetif(),
          exit_(false),
          lastNetifEvent_(NetifEvent::None),
          expectedNcpState_(NcpState::OFF),
          expectedConnectionState_(NcpConnectionState::DISCONNECTED) {

    LOG(INFO, "Creating PppNcpNetif LwIP interface");
    // A boolean semaphore is sufficient to synchronize the internal thread.
    SPARK_ASSERT(os_semaphore_create(&netifSemaphore_, 1, 0) == 0);

    client_.setNotifyCallback(pppEventHandlerCb, this);
    client_.start();
}

PppNcpNetif::~PppNcpNetif() {
    exit_ = true;
    if (thread_ && netifSemaphore_) {
        os_semaphore_give(netifSemaphore_, false);
        os_thread_join(thread_);
        os_semaphore_destroy(netifSemaphore_);
    }
}

void PppNcpNetif::init() {
    registerHandlers();
    SPARK_ASSERT(lwip_memp_event_handler_add(mempEventHandler, MEMP_PBUF_POOL, this) == 0);
    SPARK_ASSERT(os_thread_create(&thread_, "pppncp", OS_THREAD_PRIORITY_NETWORK, &PppNcpNetif::loop, this, OS_THREAD_STACK_SIZE_DEFAULT_HIGH) == 0);
}

void PppNcpNetif::setCellularManager(CellularNetworkManager* celMan) {
    celMan_ = celMan;
}

if_t PppNcpNetif::interface() {
    return (if_t)client_.getIf();
}

void PppNcpNetif::setExpectedInternalState(NetifEvent ev) {
    switch (ev) {
        case NetifEvent::Up: {
            expectedNcpState_ = NcpState::ON;
            expectedConnectionState_ = NcpConnectionState::CONNECTED;
            break;
        }
        case NetifEvent::Down: {
            expectedConnectionState_ = NcpConnectionState::DISCONNECTED;
            break;
        }
        case NetifEvent::PowerOff: {
            expectedNcpState_ = NcpState::OFF;
            expectedConnectionState_ = NcpConnectionState::DISCONNECTED;
            break;
        }
        case NetifEvent::PowerOn: {
            expectedNcpState_ = NcpState::ON;
            break;
        }
    }
}

void PppNcpNetif::loop(void* arg) {
    PppNcpNetif* self = static_cast<PppNcpNetif*>(arg);
    unsigned int timeout = 100;
    while(!self->exit_) {
        os_semaphore_take(self->netifSemaphore_, timeout, false);
        self->celMan_->ncpClient()->enable(); // Make sure the client is enabled

        // We shouldn't be enforcing state on boot!
        if (self->lastNetifEvent_ != NetifEvent::None) {
            if (self->expectedNcpState_ == NcpState::ON && self->celMan_->ncpClient()->ncpState() != NcpState::ON) {
                auto r = self->celMan_->ncpClient()->on();
                if (r != SYSTEM_ERROR_NONE && r != SYSTEM_ERROR_ALREADY_EXISTS) {
                    LOG(ERROR, "Failed to initialize cellular NCP client: %d", r);
                }
            }
            if (self->expectedConnectionState_ == NcpConnectionState::CONNECTED &&
                self->celMan_->ncpClient()->connectionState() == NcpConnectionState::DISCONNECTED &&
                self->celMan_->ncpClient()->ncpState() == NcpState::ON) {
                self->upImpl();
            }

            if (self->expectedConnectionState_ == NcpConnectionState::DISCONNECTED &&
                self->celMan_->ncpClient()->connectionState() != NcpConnectionState::DISCONNECTED) {
                self->downImpl();
            }
            if (self->expectedNcpState_ == NcpState::OFF && self->celMan_->ncpClient()->connectionState() == NcpConnectionState::DISCONNECTED) {
                if_power_state_t pwrState = IF_POWER_STATE_NONE;
                self->getPowerState(&pwrState);
                if (pwrState == IF_POWER_STATE_UP) {
                    auto r = self->celMan_->ncpClient()->off();
                    if (r != SYSTEM_ERROR_NONE) {
                        LOG(ERROR, "Failed to turn off NCP client: %d", r);
                    }
                }
            }
        }

        self->celMan_->ncpClient()->processEvents();

        if (self->expectedConnectionState_ == NcpConnectionState::CONNECTED && self->celMan_->ncpClient()->connectionState() == NcpConnectionState::CONNECTED) {
            auto start = self->connectStart_;
            if (start != 0 && HAL_Timer_Get_Milli_Seconds() - start >= PPP_CONNECT_TIMEOUT) {
                LOG(ERROR, "Failed to bring up PPP session after %lu seconds", PPP_CONNECT_TIMEOUT / 1000);
                self->client_.disconnect();
                self->celMan_->ncpClient()->disable();
            }
        }
    }

    self->downImpl();
    self->celMan_->ncpClient()->off();

    os_thread_exit(nullptr);
}

int PppNcpNetif::up() {
    // FIXME:
    // The following separate sequential atomic operations do not make the whole change atomic.
    // We may end up in an inconsistent state. Same to wherever the changes being made.
    if (lastNetifEvent_.exchange(NetifEvent::Up, std::memory_order_acq_rel) == NetifEvent::Up) {
        return SYSTEM_ERROR_NONE;
    }
    setExpectedInternalState(lastNetifEvent_);
    // It's fine even if we failed to give the semaphore, as we specify a timeout taking the semaphore.
    os_semaphore_give(netifSemaphore_, false);
    return SYSTEM_ERROR_NONE;
}

int PppNcpNetif::down() {
    if (lastNetifEvent_.exchange(NetifEvent::Down, std::memory_order_acq_rel) == NetifEvent::Down) {
        return SYSTEM_ERROR_NONE;
    }
    const auto client = celMan_->ncpClient();
    /* Note: It's possible that we have powered on the modem but the NCP client initialization is still in progress.
     * For example, invokes Cellular.connect() followed by Cellular.disconnect().
     * If we interrupted the operation, we will probably fail to turn off the modem in enable() that is invoked in the PPP thread.
     * It would result in a hard reset of the modem, which will take longer to turn off the modem. Simply let the NCP client
     * finalize the initialization procedure and make it reside in ON state. */
    if (client->connectionState() != NcpConnectionState::CONNECTED && client->ncpState() == NcpState::ON) {
        // Disable the client to interrupt its current operation
        client->disable();
    }
    setExpectedInternalState(lastNetifEvent_);
    // It's fine even if we failed to give the semaphore, as we specify a timeout taking the semaphore.
    os_semaphore_give(netifSemaphore_, false);
    return SYSTEM_ERROR_NONE;
}

int PppNcpNetif::powerUp() {
    if (lastNetifEvent_.exchange(NetifEvent::PowerOn, std::memory_order_acq_rel) == NetifEvent::PowerOn) {
        return SYSTEM_ERROR_NONE;
    }
    setExpectedInternalState(lastNetifEvent_);
    // It's fine even if we failed to give the semaphore, as we specify a timeout taking the semaphore.
    os_semaphore_give(netifSemaphore_, false);
    return SYSTEM_ERROR_NONE;
}

int PppNcpNetif::powerDown() {
    if (lastNetifEvent_.exchange(NetifEvent::PowerOff, std::memory_order_acq_rel) == NetifEvent::PowerOff) {
        return SYSTEM_ERROR_NONE;
    }
    /* Do not abort the on-going NCP initialization, otherwise, turning off the modem using AT command
     * or hardware pins will fail in some case, which will result a hardreset of the modem. That takes
     * longer to turn off the modem. Simply let the NCP initialization complete and then execute the power
     * off sequence. */
    setExpectedInternalState(lastNetifEvent_);
    // It's fine even if we failed to give the semaphore, as we specify a timeout taking the semaphore.
    os_semaphore_give(netifSemaphore_, false);
    return SYSTEM_ERROR_NONE;
}

int PppNcpNetif::getPowerState(if_power_state_t* state) const {
    auto s = celMan_->ncpClient()->ncpPowerState();
    if (s == NcpPowerState::ON) {
        *state = IF_POWER_STATE_UP;
    } else if (s == NcpPowerState::OFF) {
        *state = IF_POWER_STATE_DOWN;
    } else if (s == NcpPowerState::TRANSIENT_ON) {
        *state = IF_POWER_STATE_POWERING_UP;
    } else if (s == NcpPowerState::TRANSIENT_OFF) {
        *state = IF_POWER_STATE_POWERING_DOWN;
    } else {
        *state = IF_POWER_STATE_NONE;
    }
    return SYSTEM_ERROR_NONE;
}

int PppNcpNetif::getNcpState(unsigned int* state) const {
    auto s = celMan_->ncpClient()->ncpState();
    if (s == NcpState::ON) {
        *state = 1;
    } else {
        // NcpState::OFF or NcpState::DISABLED
        *state = 0;
    }
    return SYSTEM_ERROR_NONE;
}

int PppNcpNetif::upImpl() {
    client_.setOutputCallback([](const uint8_t* data, size_t size, void* ctx) -> int {
        auto c = (CellularNcpClient*)ctx;
        int r = c->dataChannelWrite(0, data, size);
        if (!r) {
            return size;
        }
        return r;
    }, celMan_->ncpClient());
    client_.setEnterDataModeCallback([](void* ctx) -> int {
        auto c = (CellularNcpClient*)ctx;
        return c->enterDataMode();
    }, celMan_->ncpClient());
    // Initialize PPP client
    client_.connect();

    auto r = celMan_->connect();
    if (r) {
        LOG(TRACE, "Failed to connect to cellular network: %d", r);
        // Make sure to re-enable NCP client
        celMan_->ncpClient()->enable();
        // And turn it off just in case
        celMan_->ncpClient()->off();
    }
    return r;
}

int PppNcpNetif::downImpl() {
    int r = SYSTEM_ERROR_NONE;
    client_.notifyEvent(ppp::Client::EVENT_LOWER_DOWN);
    client_.disconnect();

    if (celMan_->ncpClient()->connectionState() != NcpConnectionState::DISCONNECTED) {
        r = celMan_->ncpClient()->disconnect();
        if (r != SYSTEM_ERROR_NONE) {
            // Make sure to re-enable NCP client
            celMan_->ncpClient()->enable();
        }
    }
    return r;
}

void PppNcpNetif::ifEventHandler(const if_event* ev) {
    if (ev->ev_type == IF_EVENT_STATE) {
        int r = 0;
        if (ev->ev_if_state->state) {
            r = up();
        } else {
            r = down();
        }
        if (r) {
            LOG(ERROR, "Failed to post iface event");
        }
    }
}

void PppNcpNetif::netifEventHandler(netif_nsc_reason_t reason, const netif_ext_callback_args_t* args) {
    /* Nothing to do here */
}

void PppNcpNetif::pppEventHandlerCb(particle::net::ppp::Client* c, uint64_t ev, int data, void* ctx) {
    auto self = (PppNcpNetif*)ctx;
    self->pppEventHandler(ev, data);
}

void PppNcpNetif::pppEventHandler(uint64_t ev, int data) {
    if (ev == particle::net::ppp::Client::EVENT_UP) {
        unsigned mtu = client_.getIf()->mtu;
        LOG(TRACE, "Negotiated MTU: %u", mtu);
        // Reset
        connectStart_ = 0;
        const auto ncpMtu = celMan_->ncpClient()->getMtu();
        if (ncpMtu > 0 && (unsigned)ncpMtu < mtu) {
            LOG(TRACE, "Updating MTU to: %u", ncpMtu);
            client_.getIf()->mtu = ncpMtu;
        }
    } else if (ev == particle::net::ppp::Client::EVENT_CONNECTING) {
        if (connectStart_ == 0) {
            connectStart_ = HAL_Timer_Get_Milli_Seconds();
        }
    } else if (ev == particle::net::ppp::Client::EVENT_ERROR) {
        LOG(ERROR, "PPP error event data=%d", data);
        celMan_->ncpClient()->dataModeError(pppErrorToSystem(data));
    }
}

void PppNcpNetif::ncpEventHandlerCb(const NcpEvent& ev, void* ctx) {
    // This is a static method and the ctx may be nullptr if PppNcpNetif is not created but
    // we have accessed the NCP client for certain operations, such as getting the NCP firmware version.
    // Simply assert to catch the error if we're doing something wrong
    SPARK_ASSERT(ctx);
    LOG(TRACE, "NCP event %d", (int)ev.type);
    auto self = (PppNcpNetif*)ctx;
    if (ev.type == NcpEvent::CONNECTION_STATE_CHANGED) {
        const auto& cev = static_cast<const NcpConnectionStateChangedEvent&>(ev);
        LOG(TRACE, "State changed event: %d", (int)cev.state);
        switch (cev.state) {
            case NcpConnectionState::DISCONNECTED:
            case NcpConnectionState::CONNECTING: {
                self->client_.notifyEvent(ppp::Client::EVENT_LOWER_DOWN);
                self->connectStart_ = 0;
                break;
            }
            case NcpConnectionState::CONNECTED: {
                self->connectStart_ = 0;
                self->client_.notifyEvent(ppp::Client::EVENT_LOWER_UP);
            }
        }
    } else if (ev.type == CellularNcpEvent::AUTH) {
        const auto& cev = static_cast<const CellularNcpAuthEvent&>(ev);
        LOG(TRACE, "New auth info");
        self->client_.setAuth(cev.user, cev.password);
    } else if (ev.type == NcpEvent::POWER_STATE_CHANGED) {
        const auto& cev = static_cast<const NcpPowerStateChangedEvent&>(ev);
        if (cev.state != NcpPowerState::UNKNOWN) {
            if_event evt = {};
            struct if_event_power_state ev_if_power_state = {};
            evt.ev_len = sizeof(if_event);
            evt.ev_type = IF_EVENT_POWER_STATE;
            evt.ev_power_state = &ev_if_power_state;
            if (cev.state == NcpPowerState::ON) {
                evt.ev_power_state->state = IF_POWER_STATE_UP;
                LOG(TRACE, "NCP power state changed: IF_POWER_STATE_UP");
            } else if (cev.state == NcpPowerState::OFF) {
                evt.ev_power_state->state = IF_POWER_STATE_DOWN;
                LOG(TRACE, "NCP power state changed: IF_POWER_STATE_DOWN");
            } else if (cev.state == NcpPowerState::TRANSIENT_ON) {
                evt.ev_power_state->state = IF_POWER_STATE_POWERING_UP;
                LOG(TRACE, "NCP power state changed: IF_POWER_STATE_POWERING_UP");
            } else {
                evt.ev_power_state->state = IF_POWER_STATE_POWERING_DOWN;
                LOG(TRACE, "NCP power state changed: IF_POWER_STATE_POWERING_DOWN");
            }
            if_notify_event(self->interface(), &evt, nullptr);
        } else {
            LOG(ERROR, "NCP power state unknown");
        }
    } else if (ev.type == NcpEvent::NCP_STATE_CHANGED) {
        const auto& cev = static_cast<const NcpStateChangedEvent&>(ev);
        if_event evt = {};
        struct if_event_phy_state ev_if_phy_state = {};
        evt.ev_len = sizeof(if_event);
        evt.ev_type = IF_EVENT_PHY_STATE;
        evt.ev_phy_state = &ev_if_phy_state;
        if (cev.state == NcpState::ON) {
            evt.ev_phy_state->state = IF_PHY_STATE_ON;
        } else if (cev.state == NcpState::OFF) {
            evt.ev_phy_state->state = IF_PHY_STATE_OFF;
        } else {
            evt.ev_phy_state->state = IF_PHY_STATE_UNKNOWN;
        }
        if_notify_event(self->interface(), &evt, nullptr);
    }
}

int PppNcpNetif::ncpDataHandlerCb(int id, const uint8_t* data, size_t size, void* ctx) {
    // This is a static method and the ctx may be nullptr if PppNcpNetif is not created but
    // we have accessed the NCP client for certain operations, such as getting the NCP firmware version.
    // Simply assert to catch the error if we're doing something wrong
    SPARK_ASSERT(ctx);
    PppNcpNetif* self = static_cast<PppNcpNetif*>(ctx);
    int r = self->client_.input(data, size);
    if (r == SYSTEM_ERROR_NO_MEMORY) {
        self->celMan_->ncpClient()->dataChannelFlowControl(true);
    }
    return r;
}

void PppNcpNetif::mempEventHandler(memp_t type, unsigned available, unsigned size, void* ctx) {
    PppNcpNetif* self = static_cast<PppNcpNetif*>(ctx);
    if (available > HAL_PLATFORM_PACKET_BUFFER_FLOW_CONTROL_THRESHOLD) {
        self->celMan_->ncpClient()->dataChannelFlowControl(false);
    }
}
