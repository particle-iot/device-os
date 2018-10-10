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

#include "concurrent_hal.h"

#include "platform_config.h"

using namespace particle::net;

namespace {

enum class NetifEvent {
    None = 0,
    Up = 1,
    Down = 2,
    Exit = 3
};

} // anonymous


PppNcpNetif::PppNcpNetif(particle::services::at::BoronNcpAtClient* atclient)
        : BaseNetif(),
          exit_(false),
          atClient_(atclient),
          origStream_(atclient->getStream()),
          muxer_(origStream_) {

    registerHandlers();

    LOG(INFO, "Creating PppNcpNetif LwIP interface");

    client_.setNotifyCallback(pppEventHandlerCb, this);
    client_.start();

    SPARK_ASSERT(os_queue_create(&queue_, sizeof(void*), 4, nullptr) == 0);
    SPARK_ASSERT(os_thread_create(&thread_, "pppncp", OS_THREAD_PRIORITY_NETWORK, &PppNcpNetif::loop, this, OS_THREAD_STACK_SIZE_DEFAULT) == 0);
}

PppNcpNetif::~PppNcpNetif() {
    exit_ = true;
    if (thread_ && queue_) {
        auto ex = NetifEvent::Exit;
        os_queue_put(queue_, &ex, 1000, nullptr);
        os_thread_join(thread_);
        os_queue_destroy(queue_, nullptr);
    }
}

if_t PppNcpNetif::interface() {
    return (if_t)client_.getIf();
}

void PppNcpNetif::hwReset() {
    HAL_Pin_Mode(UBPWR, OUTPUT);
    HAL_Pin_Mode(UBRST, OUTPUT);
    HAL_Pin_Mode(BUFEN, OUTPUT);
    HAL_Pin_Mode(ANTSW1, OUTPUT);

    HAL_GPIO_Write(ANTSW1, 1);
    HAL_GPIO_Write(BUFEN, 0);

    HAL_GPIO_Write(UBRST, 0);
    HAL_Delay_Milliseconds(100);
    HAL_GPIO_Write(UBRST, 1);

    HAL_GPIO_Write(UBPWR, 0); HAL_Delay_Milliseconds(50);
    HAL_GPIO_Write(UBPWR, 1); HAL_Delay_Milliseconds(10);
}

void PppNcpNetif::loop(void* arg) {
    PppNcpNetif* self = static_cast<PppNcpNetif*>(arg);
    unsigned int timeout = 100;
    while(!self->exit_) {
        NetifEvent ev;
        if (!os_queue_take(self->queue_, &ev, timeout, nullptr)) {
            // Event
            switch (ev) {
                case NetifEvent::Up: {
                    self->upImpl();
                    // if (self->upImpl()) {
                    //     self->wifiMan_->ncpClient()->off();
                    // }
                    break;
                }
                case NetifEvent::Down: {
                    self->downImpl();
                    // if (self->downImpl()) {
                    //     self->wifiMan_->ncpClient()->off();
                    // }
                    break;
                }
            }
        } else {
            // if (self->up_) {
            //     LwipTcpIpCoreLock lk;
            //     if (!netif_is_link_up(self->interface())) {
            //         self->upImpl();
            //     }
            // }
        }
        // self->wifiMan_->ncpClient()->processEvents();
    }

    self->down();

    os_thread_exit(nullptr);
}

int PppNcpNetif::up() {
    NetifEvent ev = NetifEvent::Up;
    return os_queue_put(queue_, &ev, CONCURRENT_WAIT_FOREVER, nullptr);
}

int PppNcpNetif::down() {
    NetifEvent ev = NetifEvent::Down;
    return os_queue_put(queue_, &ev, CONCURRENT_WAIT_FOREVER, nullptr);
}

int PppNcpNetif::upImpl() {
    hwReset();
    atClient_->setStream(origStream_);
    atClient_->reset();
    atClient_->setTimeout(60000);

    CHECK(atClient_->selectSim(true));
    while (atClient_->waitReady(1)) {
        HAL_Delay_Milliseconds(1000);
    }
    CHECK(atClient_->getImsi());
    CHECK(atClient_->getCcid());

    CHECK(atClient_->registerNet());
    while (atClient_->isRegistered(false)) {
        HAL_Delay_Milliseconds(1000);
    }
    while (atClient_->isRegistered(true)) {
        HAL_Delay_Milliseconds(1000);
    }
    CHECK(atClient_->startMuxer());
    // Initiator
    muxer_.setMaxFrameSize(1509);
    muxer_.setMaxRetransmissions(10);
    muxer_.setAckTimeout(1000);
    muxer_.setControlResponseTimeout(1000);
    muxer_.setKeepAlivePeriod(10000);
    muxer_.setKeepAliveMaxMissed(6);
    CHECK(muxer_.start(true));
    CHECK(muxer_.openChannel(1, [](const uint8_t* data, size_t size, void* ctx) -> int {
        auto client = (particle::net::ppp::Client*)ctx;
        return client->input(data, size);
    }, &client_));
    muxer_.writeChannel(1, (const uint8_t*)"AT+CGDATA=\"PPP\",1\r\n", strlen("AT+CGDATA=\"PPP\",1\r\n"));
    client_.setOutputCallback([](const uint8_t* data, size_t size, void* ctx) -> int {
        auto self = (PppNcpNetif*)ctx;
        auto r = self->muxer_.writeChannel(1, data, size);
        if (r == 0) {
            return size;
        }
        return r;
    }, this);
    client_.connect();
    client_.notifyEvent(particle::net::ppp::Client::EVENT_LOWER_UP);
    // muxer_.openChannel(1, channelDataHandlerCb, this);
    // muxer_.resumeChannel(1);

    // LwipTcpIpCoreLock lk;
    // netif_set_link_up(interface());

    return 0;
}

int PppNcpNetif::downImpl() {
    client_.disconnect();
    muxer_.stop();
    hwReset();
    // LwipTcpIpCoreLock lk;
    // netif_set_link_down(interface());
    return 0;
}

void PppNcpNetif::ifEventHandler(const if_event* ev) {
    if (ev->ev_type == IF_EVENT_STATE) {
        if (ev->ev_if_state->state) {
            up();
        } else {
            down();
        }
    }
}

void PppNcpNetif::netifEventHandler(netif_nsc_reason_t reason, const netif_ext_callback_args_t* args) {
    /* Nothing to do here */
}

void PppNcpNetif::pppEventHandlerCb(particle::net::ppp::Client* c, uint64_t ev, void* ctx) {
    auto self = (PppNcpNetif*)ctx;
    self->pppEventHandler(ev);
}

void PppNcpNetif::pppEventHandler(uint64_t ev) {
    LOG(TRACE, "pppp ev %llu", ev);
}
