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

namespace {

enum class NetifEvent {
    None = 0,
    Up = 1,
    Down = 2,
    Exit = 3,
    PowerOff = 4,
    PowerOn = 5
};

} // anonymous

using namespace particle::net;

Esp32NcpNetif::Esp32NcpNetif()
        : BaseNetif(),
          exit_(false) {

    LOG(INFO, "Creating Esp32NcpNetif LwIP interface");

    if (!netifapi_netif_add(interface(), nullptr, nullptr, nullptr, this, initCb, ethernet_input)) {
        SPARK_ASSERT(os_queue_create(&queue_, sizeof(NetifEvent), 4, nullptr) == 0);
    }
}

Esp32NcpNetif::~Esp32NcpNetif() {
    exit_ = true;
    if (thread_ && queue_) {
        auto ex = NetifEvent::Exit;
        os_queue_put(queue_, &ex, 1000, nullptr);
        os_thread_join(thread_);
        os_queue_destroy(queue_, nullptr);
    }
}

void Esp32NcpNetif::init() {
    registerHandlers();
    SPARK_ASSERT(lwip_memp_event_handler_add(mempEventHandler, MEMP_PBUF_POOL, this) == 0);
    SPARK_ASSERT(os_thread_create(&thread_, "esp32ncp", OS_THREAD_PRIORITY_NETWORK, &Esp32NcpNetif::loop, this, OS_THREAD_STACK_SIZE_DEFAULT) == 0);
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
    uint8_t deviceIdLen = HAL_device_ID(deviceId, sizeof(deviceId));
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

int Esp32NcpNetif::queryMacAddress() {
    auto r = wifiMan_->ncpClient()->on();
    if (r) {
        LOG(TRACE, "Failed to initialize ESP32 NCP client: %d", r);
        return r;
    }
    MacAddress mac = {};
    if (!memcmp(interface()->hwaddr, mac.data, interface()->hwaddr_len)) {
        // Query MAC address
        r = wifiMan_->ncpClient()->getMacAddress(&mac);
        if (r) {
            LOG(TRACE, "Failed to query ESP32 MAC address: %d", r);
            return r;
        }
        memcpy(interface()->hwaddr, mac.data, interface()->hwaddr_len);
    }

    return r;
}

void Esp32NcpNetif::loop(void* arg) {
    Esp32NcpNetif* self = static_cast<Esp32NcpNetif*>(arg);
    unsigned int timeout = 100;
    self->queryMacAddress();
    self->wifiMan_->ncpClient()->off();
    while(!self->exit_) {
        self->wifiMan_->ncpClient()->enable(); // Make sure the client is enabled
        NetifEvent ev;
        const int r = os_queue_take(self->queue_, &ev, timeout, nullptr);
        if (!r) {
            // Event
            switch (ev) {
                case NetifEvent::Up: {
                    if (self->upImpl()) {
                        self->wifiMan_->ncpClient()->off();
                    }
                    break;
                }
                case NetifEvent::Down: {
                    self->downImpl();
                    // self->wifiMan_->ncpClient()->off();
                    break;
                }
                case NetifEvent::PowerOff: {
                    self->downImpl();
                    self->wifiMan_->ncpClient()->off();
                    break;
                }
                case NetifEvent::PowerOn: {
                    self->wifiMan_->ncpClient()->on();
                    break;
                }
            }
        } else {
            if (self->up_) {
                LwipTcpIpCoreLock lk;
                if (!netif_is_link_up(self->interface())) {
                    // If we don't unlock the mutex here, we can easily cause a deadlock
                    lk.unlock();
                    if (self->upImpl()) {
                        self->wifiMan_->ncpClient()->off();
                    }
                }
            }
        }
        self->wifiMan_->ncpClient()->processEvents();
    }

    self->downImpl();
    self->wifiMan_->ncpClient()->off();

    os_thread_exit(nullptr);
}

int Esp32NcpNetif::up() {
    NetifEvent ev = NetifEvent::Up;
    return os_queue_put(queue_, &ev, CONCURRENT_WAIT_FOREVER, nullptr);
}

int Esp32NcpNetif::down() {
    const auto client = wifiMan_->ncpClient();
    if (client->connectionState() != NcpConnectionState::CONNECTED) {
        // Disable the client to interrupt its current operation
        client->disable();
    }
    NetifEvent ev = NetifEvent::Down;
    return os_queue_put(queue_, &ev, CONCURRENT_WAIT_FOREVER, nullptr);
}

int Esp32NcpNetif::powerUp() {
    NetifEvent ev = NetifEvent::PowerOn;
    return os_queue_put(queue_, &ev, CONCURRENT_WAIT_FOREVER, nullptr);
}

int Esp32NcpNetif::powerDown() {
    NetifEvent ev = NetifEvent::PowerOff;
    return os_queue_put(queue_, &ev, CONCURRENT_WAIT_FOREVER, nullptr);
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

int Esp32NcpNetif::upImpl() {
    up_ = true;
    auto r = queryMacAddress();
    if (r) // Failed to query MAC address
        return r;
    // Ensure that we are disconnected
    downImpl();
    // Restore up flag
    up_ = true;
    r = wifiMan_->connect();
    if (r) {
        LOG(TRACE, "Failed to connect to WiFi: %d", r);
    }
    return r;
}

void Esp32NcpNetif::ncpEventHandlerCb(const NcpEvent& ev, void* ctx) {
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
    }
}

int Esp32NcpNetif::ncpDataHandlerCb(int id, const uint8_t* data, size_t size, void* ctx) {
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
    up_ = false;
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
        if (ev->ev_if_state->state) {
            up();
        } else {
            down();
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
