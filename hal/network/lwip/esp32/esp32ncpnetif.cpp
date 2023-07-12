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

// DO NOT REMOVE
#include "interrupts_hal.h"

#include "logging.h"
LOG_SOURCE_CATEGORY("net.esp32ncp")

#include "esp32ncpnetif.h"
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
#include "network/ncp/wifi/wifi_ncp_client.h"
#include "concurrent_hal.h"
#include "deviceid_hal.h"
#include "bytes2hexbuf.h"
#include "platform_config.h"
#include "check.h"
#include <lwip/stats.h>
#include "memp_hook.h"
#include "ncp.h"

using namespace particle::net;

Esp32NcpNetif::Esp32NcpNetif()
        : BaseNetif(),
          exit_(false),
          lastNetifEvent_(NetifEvent::None),
          expectedNcpState_(NcpState::OFF),
          expectedConnectionState_(NcpConnectionState::DISCONNECTED) {

    LOG(INFO, "Creating Esp32NcpNetif LwIP interface");

    if (!netifapi_netif_add(interface(), nullptr, nullptr, nullptr, this, initCb, ethernet_input)) {
        // A boolean semaphore is sufficient to synchronize the internal thread.
        SPARK_ASSERT(os_semaphore_create(&netifSemaphore_, 1, 0) == 0);
    }
}

Esp32NcpNetif::~Esp32NcpNetif() {
    exit_ = true;
    if (thread_ && netifSemaphore_) {
        os_semaphore_give(netifSemaphore_, false);
        os_thread_join(thread_);
        os_semaphore_destroy(netifSemaphore_);
    }
}

void Esp32NcpNetif::init() {
    registerHandlers();
    SPARK_ASSERT(lwip_memp_event_handler_add(mempEventHandler, MEMP_PBUF_POOL, this) == 0);
    SPARK_ASSERT(os_thread_create(&thread_, "esp32ncp", OS_THREAD_PRIORITY_NETWORK, &Esp32NcpNetif::loop, this, OS_THREAD_STACK_SIZE_DEFAULT_HIGH) == 0);
}

void Esp32NcpNetif::setWifiManager(particle::WifiNetworkManager* wifiMan) {
    wifiMan_ = wifiMan;
}

err_t Esp32NcpNetif::initCb(netif* netif) {
    Esp32NcpNetif* self = static_cast<Esp32NcpNetif*>(netif->state);

    return self->initInterface();
}

err_t Esp32NcpNetif::initInterface() {
    netif_.name[0] = 'w';
    netif_.name[1] = 'l';

    netif_.hwaddr_len = ETHARP_HWADDR_LEN;
    netif_.mtu = 1500;
    netif_.flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;
    /* FIXME: Remove once we enable IPv6 */
    netif_.flags |= NETIF_FLAG_NO_ND6;
    /* netif_.flags |= NETIF_FLAG_MLD6 */

    uint8_t deviceId[HAL_DEVICE_ID_SIZE] = {};
    uint8_t deviceIdLen = hal_get_device_id(deviceId, sizeof(deviceId));
    hostname_ = std::make_unique<char[]>(deviceIdLen * 2 + 1);
    if (hostname_) {
        bytes2hexbuf_lower_case(deviceId, deviceIdLen, hostname_.get());
        hostname_[deviceIdLen * 2] = '\0';
    }
    netif_set_hostname(interface(), hostname_.get());

    netif_.output = etharp_output;
    netif_.output_ip6 = ethip6_output;
    netif_.linkoutput = &Esp32NcpNetif::linkOutputCb;

    return ERR_OK;
}

int Esp32NcpNetif::queryMacAddress(MacAddress& mac) {
    auto r = wifiNcpGetCachedMacAddress(&mac);
    if (!r && mac != INVALID_MAC_ADDRESS && mac != INVALID_ZERO_MAC_ADDRESS) {
        return 0;
    }

    const auto ncpClient = wifiMan_->ncpClient();
    const NcpClientLock lock(ncpClient);
    r = ncpClient->on();
    if (r) {
        LOG(TRACE, "Failed to initialize ESP32 NCP client: %d", r);
        return r;
    }
    r = ncpClient->getMacAddress(&mac);
    if (r) {
        LOG(ERROR, "Failed to query MAC address: %d", r);
    }
    return r;
}

int Esp32NcpNetif::configureMacAddress() {
    MacAddress mac = {};
    CHECK(queryMacAddress(mac));
    if (memcmp(interface()->hwaddr, mac.data, interface()->hwaddr_len)) {
        char macStr[MAC_ADDRESS_STRING_SIZE + 1] = {};
        macAddressToString(mac, macStr, sizeof(macStr));
        LOG(INFO, "Setting WiFi interface MAC to %s", macStr);
        memcpy(interface()->hwaddr, mac.data, interface()->hwaddr_len);
    }
    return 0;
}

void Esp32NcpNetif::setExpectedInternalState(NetifEvent ev) {
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

void Esp32NcpNetif::loop(void* arg) {
    Esp32NcpNetif* self = static_cast<Esp32NcpNetif*>(arg);
    unsigned int timeout = 100;
#if !HAL_PLATFORM_WIFI_SCAN_ONLY
    // Fetches from cache, fall back to querying ESP32 directly
    self->configureMacAddress();
#endif // !HAL_PLATFORM_WIFI_SCAN_ONLY
    while(!self->exit_) {
        self->wifiMan_->ncpClient()->enable(); // Make sure the client is enabled
        os_semaphore_take(self->netifSemaphore_, timeout, false);

        // FIXME: this is run before any other actions is taken to make sure that this
        // thread is not performing any state changes if something has acquired an exclusive
        // lock to the NCP Client.
        self->wifiMan_->ncpClient()->processEvents();

        // We shouldn't be enforcing state on boot!
        if (self->lastNetifEvent_ != NetifEvent::None) {
            if (self->expectedNcpState_ == NcpState::ON && self->wifiMan_->ncpClient()->ncpState() != NcpState::ON) {
                auto r = self->wifiMan_->ncpClient()->on();
                if (r != SYSTEM_ERROR_NONE && r != SYSTEM_ERROR_ALREADY_EXISTS) {
                    LOG(ERROR, "Failed to initialize wifi NCP client: %d", r);
                }
            }
            if (self->expectedConnectionState_ == NcpConnectionState::CONNECTED &&
                self->wifiMan_->ncpClient()->connectionState() == NcpConnectionState::DISCONNECTED &&
                self->wifiMan_->ncpClient()->ncpState() == NcpState::ON) {
                self->upImpl();
            }

            if (self->expectedConnectionState_ == NcpConnectionState::DISCONNECTED &&
                self->wifiMan_->ncpClient()->connectionState() != NcpConnectionState::DISCONNECTED) {
                self->downImpl();
            }
            if (self->expectedNcpState_ == NcpState::OFF && self->wifiMan_->ncpClient()->connectionState() == NcpConnectionState::DISCONNECTED) {
                if_power_state_t pwrState = IF_POWER_STATE_NONE;
                self->getPowerState(&pwrState);
                if (pwrState == IF_POWER_STATE_UP) {
                    auto r = self->wifiMan_->ncpClient()->off();
                    if (r != SYSTEM_ERROR_NONE) {
                        LOG(ERROR, "Failed to turn off NCP client: %d", r);
                    }
                }
            }
        }
    }

    self->downImpl();
    self->wifiMan_->ncpClient()->off();

    os_thread_exit(nullptr);
}

int Esp32NcpNetif::getCurrentProfile(spark::Vector<char>* profile) const {
    const auto client = wifiMan_->ncpClient();
    CHECK_TRUE(client, SYSTEM_ERROR_UNKNOWN);
    CHECK_TRUE(profile, SYSTEM_ERROR_INVALID_ARGUMENT);

    CHECK_TRUE(client->connectionState() == NcpConnectionState::CONNECTED, SYSTEM_ERROR_INVALID_STATE);

    WifiNetworkInfo info;
    CHECK(client->getNetworkInfo(&info));
    *profile = spark::Vector<char>(info.ssid(), strlen(info.ssid()));
    return 0;
}

int Esp32NcpNetif::up() {
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

int Esp32NcpNetif::down() {
    if (lastNetifEvent_.exchange(NetifEvent::Down, std::memory_order_acq_rel) == NetifEvent::Down) {
        return SYSTEM_ERROR_NONE;
    }
    const auto client = wifiMan_->ncpClient();
    if (client->connectionState() != NcpConnectionState::CONNECTED) {
        // Disable the client to interrupt its current operation
        client->disable();
    }
    setExpectedInternalState(lastNetifEvent_);
    // It's fine even if we failed to give the semaphore, as we specify a timeout taking the semaphore.
    os_semaphore_give(netifSemaphore_, false);
    return SYSTEM_ERROR_NONE;
}

int Esp32NcpNetif::powerUp() {
    if (lastNetifEvent_.exchange(NetifEvent::PowerOn, std::memory_order_acq_rel) == NetifEvent::PowerOn) {
        return SYSTEM_ERROR_NONE;
    }
    setExpectedInternalState(lastNetifEvent_);
    // It's fine even if we failed to give the semaphore, as we specify a timeout taking the semaphore.
    os_semaphore_give(netifSemaphore_, false);
    return SYSTEM_ERROR_NONE;
}

int Esp32NcpNetif::powerDown() {
    if (lastNetifEvent_.exchange(NetifEvent::PowerOff, std::memory_order_acq_rel) == NetifEvent::PowerOff) {
        return SYSTEM_ERROR_NONE;
    }
    setExpectedInternalState(lastNetifEvent_);
    // It's fine even if we failed to give the semaphore, as we specify a timeout taking the semaphore.
    os_semaphore_give(netifSemaphore_, false);
    return SYSTEM_ERROR_NONE;
}

int Esp32NcpNetif::getPowerState(if_power_state_t* state) const {
    auto s = wifiMan_->ncpClient()->ncpPowerState();
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

int Esp32NcpNetif::getNcpState(unsigned int* state) const {
    auto s = wifiMan_->ncpClient()->ncpState();
    if (s == NcpState::ON) {
        *state = 1;
    } else {
        // NcpState::OFF or NcpState::DISABLED
        *state = 0;
    }
    return SYSTEM_ERROR_NONE;
}

int Esp32NcpNetif::upImpl() {
    auto r = configureMacAddress();
    if (r) { // Failed to query MAC address
        return r;
    }
    // Ensure that we are disconnected
    downImpl();
    r = wifiMan_->connect();
    if (r) {
        LOG(TRACE, "Failed to connect to WiFi: %d", r);
    }
    return r;
}

void Esp32NcpNetif::ncpEventHandlerCb(const NcpEvent& ev, void* ctx) {
    // This is a static method and the ctx may be nullptr if Esp32NcpNetif is not created but
    // we have accessed the NCP client for certain operations, such as getting the NCP firmware version.
    // Simply assert to catch the error if we're doing something wrong
    SPARK_ASSERT(ctx);
    LOG(TRACE, "NCP event %d", (int)ev.type);
    auto self = (Esp32NcpNetif*)ctx;
    if (ev.type == NcpEvent::CONNECTION_STATE_CHANGED) {
        LwipTcpIpCoreLock lk;
        if (!netif_is_up(self->interface())) {
            // Ignore
            return;
        }
        const auto& cev = static_cast<const NcpConnectionStateChangedEvent&>(ev);
        LOG(TRACE, "State changed event: %d", (int)cev.state);
        if (cev.state == NcpConnectionState::DISCONNECTED) {
            netif_set_link_down(self->interface());
        } else if (cev.state == NcpConnectionState::CONNECTED) {
            netif_set_link_up(self->interface());
        }
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

int Esp32NcpNetif::ncpDataHandlerCb(int id, const uint8_t* data, size_t size, void* ctx) {
    // This is a static method and the ctx may be nullptr if Esp32NcpNetif is not created but
    // we have accessed the NCP client for certain operations, such as getting the NCP firmware version.
    // Simply assert to catch the error if we're doing something wrong
    SPARK_ASSERT(ctx);
    Esp32NcpNetif* self = static_cast<Esp32NcpNetif*>(ctx);
    size_t pktSize = size;
#if ETH_PAD_SIZE
        /* allow room for Ethernet padding */
    pktSize += ETH_PAD_SIZE;
#endif /* ETH_PAD_SIZE */

    pbuf* p = pbuf_alloc(PBUF_RAW, pktSize, PBUF_POOL);
    err_t err = ERR_OK;
    if (p != nullptr) {
#if ETH_PAD_SIZE
        /* drop the padding word */
        pbuf_remove_header(p, ETH_PAD_SIZE);
#endif /* ETH_PAD_SIZE */
        memcpy(p->payload, data, size);
#if ETH_PAD_SIZE
        /* reclaim the padding word */
        pbuf_add_header(p, ETH_PAD_SIZE);
#endif /* ETH_PAD_SIZE */

        LwipTcpIpCoreLock lk;
        err = self->interface()->input(p, self->interface());
        if (err != ERR_OK) {
            LOG_DEBUG(WARN, "Error inputing packet %d", err);
            pbuf_free(p);
        }
    }

    int poolAvail = MEMP_STATS_GET(avail, MEMP_PBUF_POOL) - MEMP_STATS_GET(used, MEMP_PBUF_POOL);

    if (!p || err == ERR_MEM || poolAvail <= HAL_PLATFORM_PACKET_BUFFER_FLOW_CONTROL_THRESHOLD) {
        self->wifiMan_->ncpClient()->dataChannelFlowControl(true);
#ifdef DEBUG_BUILD
        if (!p || err == ERR_MEM) {
            LOG_DEBUG(WARN, "May have dropped %u bytes", size);
        }
#endif // DEBUG_BUILD
    }

    return 0;
}

int Esp32NcpNetif::downImpl() {
    wifiMan_->ncpClient()->disconnect();
    LwipTcpIpCoreLock lk;
    netif_set_link_down(interface());
    return 0;
}

err_t Esp32NcpNetif::linkOutput(pbuf* p) {
    if (!(netif_is_up(interface()) && netif_is_link_up(interface()))) {
        return ERR_IF;
    }

    // LOG_DEBUG(TRACE, "link output %x %u", p->payload, p->tot_len);

#if ETH_PAD_SIZE
    pbuf_remove_header(p, ETH_PAD_SIZE); /* drop the padding word */
#endif

    if (p->len == p->tot_len) {
        // non-queue packet
        wifiMan_->ncpClient()->dataChannelWrite(0, (const uint8_t*)p->payload, p->tot_len);
    } else {
        pbuf* q = pbuf_clone(PBUF_LINK, PBUF_RAM, p);
        if (q) {
            wifiMan_->ncpClient()->dataChannelWrite(0, (const uint8_t*)q->payload, q->tot_len);
            pbuf_free(q);
        }
    }

#if ETH_PAD_SIZE
    pbuf_add_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    return ERR_OK;
}

err_t Esp32NcpNetif::linkOutputCb(netif* netif, pbuf* p) {
    Esp32NcpNetif* self = static_cast<Esp32NcpNetif*>(netif->state);

    return self->linkOutput(p);
}

void Esp32NcpNetif::ifEventHandler(const if_event* ev) {
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

void Esp32NcpNetif::netifEventHandler(netif_nsc_reason_t reason, const netif_ext_callback_args_t* args) {
    /* Nothing to do here */
}

void Esp32NcpNetif::mempEventHandler(memp_t type, unsigned available, unsigned size, void* ctx) {
    Esp32NcpNetif* self = static_cast<Esp32NcpNetif*>(ctx);
    if (available > HAL_PLATFORM_PACKET_BUFFER_FLOW_CONTROL_THRESHOLD) {
        self->wifiMan_->ncpClient()->dataChannelFlowControl(false);
    }
}
