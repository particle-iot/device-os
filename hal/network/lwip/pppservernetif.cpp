/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#undef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL (LOG_LEVEL_ALL)

#include "hal_platform.h"

#if HAL_PLATFORM_PPP_SERVER

#include "logging.h"
LOG_SOURCE_CATEGORY("net.pppserver");

#include "pppservernetif.h"
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
#include "memp_hook.h"
#include "dnsproxy.h"
#include "thread_runner.h"
#include "nat.h"

using namespace particle::net;

extern nat::Nat64* g_natInstance;

namespace {

enum PppServerNetifEvent {
    PPP_SERVER_NETIF_EVENT_EXIT = 0x01 << __builtin_ffs(HAL_USART_PVT_EVENT_MAX),
};

constexpr auto DEFAULT_SERIAL_BUFFER_SIZE = 4096;

#if 0
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
#endif

} // anonymous


PppServerNetif::PppServerNetif()
        : BaseNetif(),
          exit_(false),
          pwrState_(IF_POWER_STATE_NONE),
          settings_{} {

    LOG(INFO, "Creating PppServerNetif LwIP interface");

    client_.setServer(true);
    client_.setNotifyCallback(pppEventHandlerCb, this);
    client_.start();

    // Defaults
    settings_.serial = HAL_PLATFORM_PPP_SERVER_USART;
    settings_.baud = HAL_PLATFORM_PPP_SERVER_USART_BAUDRATE;
    settings_.config = HAL_PLATFORM_PPP_SERVER_USART_FLAGS;
}

PppServerNetif::~PppServerNetif() {
    exit_ = true;
    if (thread_ && serial_) {
        xEventGroupSetBits(serial_->eventGroup(), PPP_SERVER_NETIF_EVENT_EXIT);
        os_thread_join(thread_);
    }
}

void PppServerNetif::init() {
    registerHandlers();

    SPARK_ASSERT(lwip_memp_event_handler_add(mempEventHandler, MEMP_PBUF_POOL, this) == 0);
}

int PppServerNetif::start() {
    if (thread_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    LOG(TRACE, "Starting PppServerNetif interface");

    auto serial = std::make_unique<SerialStream>((hal_usart_interface_t)settings_.serial, settings_.baud, settings_.config, DEFAULT_SERIAL_BUFFER_SIZE, DEFAULT_SERIAL_BUFFER_SIZE);
    SPARK_ASSERT(serial);
    serial_ = std::move(serial);

    dns_ = std::make_unique<Dns>();
    SPARK_ASSERT(dns_);

    dnsRunner_ = std::make_unique<ThreadRunner>();
    SPARK_ASSERT(dnsRunner_);

    nat_ = std::make_unique<nat::Nat64>();
    SPARK_ASSERT(nat_);

    g_natInstance = nat_.get();

    SPARK_ASSERT(os_thread_create(&thread_, "pppserver", OS_THREAD_PRIORITY_NETWORK, &PppServerNetif::loop, this, OS_THREAD_STACK_SIZE_DEFAULT_HIGH) == 0);
    return 0;
}

if_t PppServerNetif::interface() {
    return (if_t)client_.getIf();
}

void PppServerNetif::loop(void* arg) {
    PppServerNetif* self = static_cast<PppServerNetif*>(arg);
    unsigned int timeout = 1000;
    while(!self->exit_) {
        auto ev = self->serial_->waitEvent(SerialStream::READABLE | PPP_SERVER_NETIF_EVENT_EXIT, timeout);
        if (ev & PPP_SERVER_NETIF_EVENT_EXIT) {
            //break;
        }

        if (ev & SerialStream::READABLE) {
            char tmp[256] = {};
            while (self->serial_->availForRead() > 0) {
                int sz = self->serial_->read(tmp, sizeof(tmp));
                // LOG(INFO, "input %d", sz);
                auto r = self->client_.input((const uint8_t*)tmp, sz);
                (void)r;
                // LOG(INFO, "input result = %d", r);
            }
        }
    }

    self->down();

    os_thread_exit(nullptr);
}

int PppServerNetif::up() {
    LOG(INFO, "up");
    CHECK(start());
    client_.setOutputCallback([](const uint8_t* data, size_t size, void* ctx) -> int {
        // LOG(INFO, "output %u", size);
        auto c = (PppServerNetif*)ctx;
        int r = c->serial_->writeAll((const char*)data, size, 1000);
        if (!r) {
            return size;
        }
        return r;
    }, this);
    client_.connect();
    client_.notifyEvent(ppp::Client::EVENT_LOWER_UP);
    return SYSTEM_ERROR_NONE;
}

int PppServerNetif::down() {
    LOG(INFO, "down");
    if (dnsRunner_) {
        dnsRunner_->destroy();
    }
    if (dns_) {
        dns_->destroy();
    }
    client_.setOutputCallback(nullptr, nullptr);
    client_.disconnect();
    return SYSTEM_ERROR_NONE;
}

int PppServerNetif::powerUp() {
    LOG(INFO, "powerUp");
    // FIXME: As long as the interface is initialized,
    // it must have been powered up as of right now.
    // The system network manager transit the interface to powering up,
    // we should always notify the event to transit the interface to powered up.
    notifyPowerState(IF_POWER_STATE_UP);
    return SYSTEM_ERROR_NONE;
}

int PppServerNetif::powerDown() {
    LOG(INFO, "powerDown");
    int ret = down();
    // FIXME: This don't really power off the module.
    // Notify the system network manager that the module is powered down
    // to bypass waitInterfaceOff() as required by system sleep.
    // The system network manager transit the interface to powering down,
    // we should always notify the event to transit the interface to powered down.
    notifyPowerState(IF_POWER_STATE_DOWN);
    return ret;
}

void PppServerNetif::notifyPowerState(if_power_state_t state) {
    pwrState_ = state;
    if_event evt = {};
    struct if_event_power_state ev_if_power_state = {};
    evt.ev_len = sizeof(if_event);
    evt.ev_type = IF_EVENT_POWER_STATE;
    evt.ev_power_state = &ev_if_power_state;
    evt.ev_power_state->state = pwrState_;
    if_notify_event(interface(), &evt, nullptr);
}

int PppServerNetif::getPowerState(if_power_state_t* state) const {
    *state = pwrState_;
    return SYSTEM_ERROR_NONE;
}

int PppServerNetif::getNcpState(unsigned int* state) const {
    // TODO: implement it
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

void PppServerNetif::ifEventHandler(const if_event* ev) {
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

void PppServerNetif::netifEventHandler(netif_nsc_reason_t reason, const netif_ext_callback_args_t* args) {
    /* Nothing to do here */
}

void PppServerNetif::pppEventHandlerCb(particle::net::ppp::Client* c, uint64_t ev, int data, void* ctx) {
    auto self = (PppServerNetif*)ctx;
    self->pppEventHandler(ev, data);
}

void PppServerNetif::pppEventHandler(uint64_t ev, int data) {
    LOG(INFO, "ppp event %llx %d", ev, data);
    if (ev == particle::net::ppp::Client::EVENT_UP) {
        unsigned mtu = client_.getIf()->mtu;
        LOG(TRACE, "Negotiated MTU: %u", mtu);
        ip6_addr_t dummy = {};
        LOG(INFO, "dns init: %d", dns_->init(interface(), dummy));
        LOG(INFO, "dns runner init: %d", dnsRunner_->init(dns_.get()));
        LOG(TRACE, "Enabling NAT");
        particle::net::nat::Rule r(interface(), nullptr);
        nat_->enable(r);
    } else if (ev == particle::net::ppp::Client::EVENT_DOWN) {
        if (dnsRunner_) {
            dnsRunner_->destroy();
        }
        if (dns_) {
            dns_->destroy();
        }
        nat_->disable(interface());
    } else if (ev == particle::net::ppp::Client::EVENT_CONNECTING) {
    } else if (ev == particle::net::ppp::Client::EVENT_ERROR) {
        LOG(ERROR, "PPP error event data=%d", data);
    }
}

void PppServerNetif::mempEventHandler(memp_t type, unsigned available, unsigned size, void* ctx) {
    //
}

int PppServerNetif::request(if_req_driver_specific* req, size_t size) {
    CHECK_TRUE(req, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(req->type == IF_REQ_DRIVER_SPECIFIC_PPP_SERVER_UART_SETTINGS, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(size >= sizeof(if_req_ppp_server_uart_settings), SYSTEM_ERROR_INVALID_ARGUMENT);

    auto settings = (if_req_ppp_server_uart_settings*)req;
    memcpy(&settings_, settings, std::min(sizeof(settings_), size));
    LOG(INFO, "Update PPP server netif settings: serial=%u baud=%u config=%08x", (unsigned)settings->serial, settings->baud, settings->config);
    return 0;
}

#endif // HAL_PLATFORM_PPP_SERVER