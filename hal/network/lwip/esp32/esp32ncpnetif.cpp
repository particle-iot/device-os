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
#include "interrupts_hal.h"

#include "concurrent_hal.h"

#include "platform_config.h"

using namespace particle::net;

Esp32NcpNetif::Esp32NcpNetif(particle::services::at::ArgonNcpAtClient* atclient)
        : BaseNetif(),
          exit_(false),
          atClient_(atclient),
          origStream_(atclient->getStream()),
          muxer_(origStream_) {

    LOG(INFO, "Creating Esp32NcpNetif LwIP interface");
    start_ = false;
    stop_ = false;

    if (!netifapi_netif_add(interface(), nullptr, nullptr, nullptr, this, initCb, ethernet_input)) {
        /* FIXME: */
        SPARK_ASSERT(os_queue_create(&queue_, sizeof(void*), 256, nullptr) == 0);
        SPARK_ASSERT(os_thread_create(&thread_, "esp32ncp", OS_THREAD_PRIORITY_NETWORK, &Esp32NcpNetif::loop, this, OS_THREAD_STACK_SIZE_DEFAULT) == 0);
    }
}

Esp32NcpNetif::~Esp32NcpNetif() {
    exit_ = true;
    if (thread_ && queue_) {
        const void* dummy = nullptr;
        os_queue_put(queue_, &dummy, 1000, nullptr);
        os_thread_join(thread_);
        os_queue_destroy(queue_, nullptr);
    }
}

netif* Esp32NcpNetif::interface() {
    return &netif_;
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

    netif_.output = etharp_output;
    netif_.output_ip6 = ethip6_output;
    netif_.linkoutput = &Esp32NcpNetif::linkOutputCb;

    return ERR_OK;
}

void Esp32NcpNetif::hwReset() {
    HAL_Pin_Mode(ESPBOOT, OUTPUT);
    HAL_Pin_Mode(ESPEN, OUTPUT);
    // De-assert the BOOT pin
    HAL_GPIO_Write(ESPBOOT, 1);
    HAL_Delay_Milliseconds(100);
     // Secondly, assert the EN pin
    HAL_GPIO_Write(ESPEN, 0);
    HAL_Delay_Milliseconds(100);
     // Thirdly, dessert EN pin
    HAL_GPIO_Write(ESPEN, 1);
    HAL_Delay_Milliseconds(100);
}

void Esp32NcpNetif::loop(void* arg) {
    Esp32NcpNetif* self = static_cast<Esp32NcpNetif*>(arg);
    unsigned int timeout = 100;
    while(!self->exit_ || self->muxer_.isRunning()) {
        if (self->start_) {
            if (!self->upImpl()) {
                self->start_ = false;
            }
        }
        if (self->stop_) {
            self->stop_ = false;
            self->downImpl();
        }
        self->muxer_.notifyInput(0);
        HAL_Delay_Milliseconds(timeout);
    }

    self->down();

    os_thread_exit(nullptr);
}

int Esp32NcpNetif::up() {
    start_ = true;
    return 0;
}

int Esp32NcpNetif::down() {
    stop_ = true;
    return 0;
}

int Esp32NcpNetif::upImpl() {
    hwReset();
    atClient_->setStream(origStream_);
    atClient_->reset();

    CHECK(atClient_->getMac(0, netif_.hwaddr));
    CHECK(atClient_->connect());
    CHECK(atClient_->startMuxer());
    // Initiator
    muxer_.setMaxFrameSize(1536);
    muxer_.setMaxRetransmissions(10);
    muxer_.setAckTimeout(1000);
    muxer_.setControlResponseTimeout(1000);
    muxer_.setKeepAlivePeriod(10000);
    muxer_.setKeepAliveMaxMissed(6);
    muxer_.start(true);
    muxer_.openChannel(2, channelDataHandlerCb, this);
    muxer_.resumeChannel(2);

    LwipTcpIpCoreLock lk;
    netif_set_link_up(interface());

    return 0;
}

int Esp32NcpNetif::channelDataHandlerCb(const uint8_t* data, size_t size, void* ctx) {
    LOG(TRACE, "channel data handler %x %u", data, size);
    Esp32NcpNetif* self = static_cast<Esp32NcpNetif*>(ctx);
    size_t pktSize = size;
#if ETH_PAD_SIZE
        /* allow room for Ethernet padding */
    pktSize += ETH_PAD_SIZE;
#endif /* ETH_PAD_SIZE */

    pbuf* p = pbuf_alloc(PBUF_RAW, pktSize, PBUF_POOL);
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
        if (self->interface()->input(p, self->interface()) != ERR_OK) {
            LOG(ERROR, "Error inputing packet");
            pbuf_free(p);
        }
    }

    return 0;
}

int Esp32NcpNetif::downImpl() {
    muxer_.stop();
    hwReset();
    LwipTcpIpCoreLock lk;
    netif_set_link_down(interface());
    return 0;
}

err_t Esp32NcpNetif::linkOutput(pbuf* p) {
    if (!(netif_is_up(interface()) && netif_is_link_up(interface()))) {
        return ERR_IF;
    }

    LOG(TRACE, "link output %x %u", p->payload, p->tot_len);

#if ETH_PAD_SIZE
    pbuf_remove_header(p, ETH_PAD_SIZE); /* drop the padding word */
#endif

    if (p->len == p->tot_len) {
        // non-queue packet
        muxer_.writeChannel(2, (const uint8_t*)p->payload, p->tot_len);
    } else {
        pbuf* q = pbuf_clone(PBUF_LINK, PBUF_RAM, p);
        if (q) {
            muxer_.writeChannel(2, (const uint8_t*)q->payload, q->tot_len);
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
