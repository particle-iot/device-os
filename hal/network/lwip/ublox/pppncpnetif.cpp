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

class MuxerDataChannel : public particle::net::ppp::DataChannel {
public:
  MuxerDataChannel(gsm0710::Muxer<particle::Stream, StaticRecursiveMutex>* muxer, int chan)
      : muxer_(muxer),
        chan_(chan),
        rxBuf_(rxBufData_, sizeof(rxBufData_)) {
    muxer->openChannel(chan_, channelDataHandlerCb, this);
    muxer->resumeChannel(chan_);
  }

  ~MuxerDataChannel() {
    muxer_->closeChannel(chan_);
  }

  virtual int readSome(char* data, size_t size) {
    std::lock_guard<std::mutex> lk(m_);
    size_t canRead = CHECK(rxBuf_.data());
    size_t toRead = std::min(size, canRead);
    if (toRead > 0) {
        return rxBuf_.get((uint8_t*)data, toRead);
    }
    return 0;
  }

  virtual int writeSome(const char* data, size_t size) override {
    muxer_->writeChannel(chan_, (const uint8_t*)data, size);
    return size;
  }

  virtual int waitEvent(unsigned flags, unsigned timeout = 0) override {
    std::unique_lock<std::mutex> lk(m_);
    if (rxBuf_.data() > 0) {
        return DataChannel::READABLE;
    } else {
        lk.unlock();
        HAL_Delay_Milliseconds(timeout);
        lk.lock();
        if (rxBuf_.data() > 0) {
            return DataChannel::READABLE;
        }
    }
    return 0;
  }

  static int channelDataHandlerCb(const uint8_t* data, size_t size, void* ctx) {
    LOG(TRACE, "channel data handler %x %u", data, size);
    auto self = (MuxerDataChannel*)ctx;
    std::lock_guard<std::mutex> lk(self->m_);
    if (self->rxBuf_.space() >= (ssize_t)size) {
        self->rxBuf_.put(data, size);
    }
    return 0;
  }

private:
    std::mutex m_;
    gsm0710::Muxer<particle::Stream, StaticRecursiveMutex>* muxer_;
    int chan_;
  particle::services::RingBuffer<uint8_t> rxBuf_;
  uint8_t rxBufData_[2048];
};

PppNcpNetif::PppNcpNetif(particle::services::at::BoronNcpAtClient* atclient)
        : BaseNetif(),
          exit_(false),
          atClient_(atclient),
          origStream_(atclient->getStream()),
          muxer_(origStream_) {

    registerHandlers();

    LOG(INFO, "Creating PppNcpNetif LwIP interface");
    start_ = false;
    stop_ = false;

    client_.start();

    // if (!netifapi_netif_add(interface(), nullptr, nullptr, nullptr, this, initCb, ethernet_input)) {
        /* FIXME: */
        SPARK_ASSERT(os_queue_create(&queue_, sizeof(void*), 256, nullptr) == 0);
        SPARK_ASSERT(os_thread_create(&thread_, "pppncp", OS_THREAD_PRIORITY_NETWORK, &PppNcpNetif::loop, this, OS_THREAD_STACK_SIZE_DEFAULT) == 0);
    // }
}

PppNcpNetif::~PppNcpNetif() {
    exit_ = true;
    if (thread_ && queue_) {
        const void* dummy = nullptr;
        os_queue_put(queue_, &dummy, 1000, nullptr);
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

int PppNcpNetif::up() {
    start_ = true;
    return 0;
}

int PppNcpNetif::down() {
    stop_ = true;
    return 0;
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
    muxer_.start(true);
    auto ch = new MuxerDataChannel(&muxer_, 1);
    ch->writeSome("AT+CGDATA=\"PPP\",1\r\n", strlen("AT+CGDATA=\"PPP\",1\r\n"));
    client_.setDataChannel(ch);
    client_.connect();
    client_.notifyEvent(particle::net::ppp::Client::EVENT_LOWER_UP);
    // muxer_.openChannel(1, channelDataHandlerCb, this);
    // muxer_.resumeChannel(1);

    // LwipTcpIpCoreLock lk;
    // netif_set_link_up(interface());

    return 0;
}

int PppNcpNetif::channelDataHandlerCb(const uint8_t* data, size_t size, void* ctx) {
    LOG(TRACE, "channel data handler %x %u", data, size);

    return 0;
}

int PppNcpNetif::downImpl() {
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
