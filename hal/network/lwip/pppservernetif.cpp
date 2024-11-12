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
#include "at_server.h"
#include "device_code.h"
#include "deviceid_hal.h"
#include "bytes2hexbuf.h"
#include "network/ncp/cellular/cellular_network_manager.h"
#include "network/ncp/cellular/cellular_ncp_client.h"
#include "network/ncp/cellular/ncp.h"
#include "cellular_hal.h"


using namespace particle::net;

extern nat::Nat64* g_natInstance;

namespace {

enum PppServerNetifEvent {
    PPP_SERVER_NETIF_EVENT_EXIT = 0x01 << __builtin_ffs(HAL_USART_PVT_EVENT_MAX),
};

constexpr auto DEFAULT_SERIAL_BUFFER_SIZE = 4096;

constexpr auto SIM_FILE_ID_ICCID = 12258;

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

    AtServerConfig conf;
    auto server = std::make_unique<AtServer>();
    SPARK_ASSERT(server);
    server_ = std::move(server);
    conf.stream(serial_.get());
    conf.streamTimeout(1000);
    server_->init(conf);

    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::EXEC, "H", [](AtServerRequest* request, AtServerCommandType type, const char* command, void* data) -> int {
        return 0;
    }, nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::EXEC, "+CGMI", "Particle"));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::EXEC, "+CGMM", [](AtServerRequest* request, AtServerCommandType type, const char* command, void* data) -> int {
        char buf[64] = {};
        get_device_usb_name(buf, sizeof(buf));
        request->sendResponse(buf);
        return 0;
    }, nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::EXEC, "E0", [](AtServerRequest* request, AtServerCommandType type, const char* command, void* data) -> int {
        auto conf = request->server()->config();
        conf.echoEnabled(false);
        request->server()->init(conf);
        return 0;
    }, nullptr));
    auto enableFlowControl = [](AtServerRequest* request, AtServerCommandType type, const char* command, void* data) -> int {
        auto conf = request->server()->config();
        conf.echoEnabled(true);
        request->server()->init(conf);
        return 0;
    };
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::EXEC, "E", enableFlowControl, nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::EXEC, "E1", enableFlowControl, nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::ONE_INT_ARG, "X", nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::ONE_INT_ARG, "V", nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::ONE_INT_ARG, "&C", nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::EXEC, "+GCAP", "+GCAP: +CGSM,+CLTE"));
    // +CSCS=?
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::EXEC, "+CGMR", PP_STR(SYSTEM_VERSION_STRING)));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::EXEC, "+CGSN", [](AtServerRequest* request, AtServerCommandType type, const char* command, void* data) -> int {
        uint8_t id[HAL_DEVICE_ID_SIZE] = {};
        const auto n = hal_get_device_id(id, sizeof(id));
        if (n != HAL_DEVICE_ID_SIZE) {
            return SYSTEM_ERROR_UNKNOWN;
        }
        char deviceId[HAL_DEVICE_ID_SIZE * 2 + 1] = {};
        bytes2hexbuf_lower_case(id, sizeof(id), deviceId);
        request->sendResponse(deviceId);
        return 0;
    }, nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::WRITE, "+CMEE", nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::EXEC, "I", [](AtServerRequest* request, AtServerCommandType type, const char* command, void* data) -> int {
        char buf[64] = {};
        get_device_usb_name(buf, sizeof(buf));
        request->sendResponse("%s %s", "Particle", buf);
        return 0;
    }, nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::TEST, "+URAT", "+URAT: (0-9)")); // Pretend to support all
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::READ, "+URAT", "+URAT: 3")); // Pretend to be LTE
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::TEST, "+CGDCONT", "+CGDCONT: (0-1),\"IP\",,,(0-2),(0-4),(0,1),(0,3),(0,1),(0,1),(0,1),(0,1),(0,1),(0,1)"));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::READ, "+CGDCONT", "+CGDCONT: 1,\"IP\",\"particle\",0,0"));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::WRITE, "+CGDCONT", nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::READ, "+CFUN", "+CFUN: 1,0")); // Full functionality
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::WRITE, "+CMER", nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::WRITE, "+CGACT", nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::READ, "+CGACT", "+CGACT: 1,1"));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::TEST, "+CMER", "+CMER: (0-3),(0),(0),(0-2),(0,1)"));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::READ, "+CPIN", "+CPIN: READY"));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::READ, "+CCID", [](AtServerRequest* request, AtServerCommandType type, const char* command, void* data) -> int {
        CellularDevice dev;
        CHECK(cellular_device_info(&dev, nullptr));
        return request->sendResponse("+CCID: %s", dev.iccid);
    }, nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::WRITE, "+CRSM", [](AtServerRequest* request, AtServerCommandType type, const char* command, void* data) -> int {
        unsigned req[5] = {};
        int r = request->scanf("%u,%u,%u,%u,%u", &req[0], &req[1], &req[2], &req[3], &req[4]);
        if (r >= 2) {
            if (req[0] == 176 /* read cmd */) {
                switch (req[1] /* file id */) {
                    case SIM_FILE_ID_ICCID: {
                        CellularDevice dev;
                        CHECK(cellular_device_info(&dev, nullptr));
                        return request->sendResponse("+CRSM: 144,0,\"%s\"", dev.iccid);
                    }
                }
            }
        }
        return SYSTEM_ERROR_AT_NOT_OK;
    }, nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::EXEC, "+CIMI", "000000000000000"));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::EXEC, "+CNUM", nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::EXEC, "PARTICLE1", nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::EXEC, "PARTICLE2", nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::EXEC, "Z", nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::EXEC, "+CSQ", [](AtServerRequest* request, AtServerCommandType type, const char* command, void* data) -> int {
        const auto mgr = cellularNetworkManager();
        CHECK_TRUE(mgr, SYSTEM_ERROR_UNKNOWN);
        const auto client = mgr->ncpClient();
        CHECK_TRUE(client, SYSTEM_ERROR_UNKNOWN);
        CellularSignalQuality s;
        CHECK(client->getSignalQuality(&s));
        const auto strn = s.strength();
        const auto qual = s.quality();
        request->sendResponse("+CSQ: %d,%d", strn, qual);
        return 0;
    }, nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::TEST, "+IFC", "+IFC: (0-2)(0-2)"));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::READ, "+IFC", [](AtServerRequest* request, AtServerCommandType type, const char* command, void* data) -> int {
        auto stream = (SerialStream*)data;
        CHECK_TRUE(stream, SYSTEM_ERROR_INVALID_STATE);
        if (stream->config() & SERIAL_FLOW_CONTROL_RTS_CTS) {
            request->sendResponse("+IFC: 2,2");
        } else {
            request->sendResponse("+IFC: 0,0");
        }
        return 0;
    }, serial_.get()));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::WRITE, "+IFC", [](AtServerRequest* request, AtServerCommandType type, const char* command, void* data) -> int {
        auto stream = (SerialStream*)data;
        CHECK_TRUE(stream, SYSTEM_ERROR_INVALID_STATE);
        int v[2] = {};
        CHECK_TRUE(request->scanf("%d,%d", &v[0], &v[1]) == 2, SYSTEM_ERROR_INVALID_ARGUMENT);
        if (v[0] == 2 || v[1] == 2) {
            auto c = stream->config() | SERIAL_FLOW_CONTROL_RTS_CTS;
            return stream->setConfig(c);
        } else if (v[0] == 0 || v[1] == 0) {
            auto c = stream->config() & (~SERIAL_FLOW_CONTROL_RTS_CTS);
            return stream->setConfig(c);
        }
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }, serial_.get()));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::WRITE, "+CGEREP", nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::WRITE, "+CREG", nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::WRITE, "+CGREG", nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::WRITE, "+CEREG", nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::READ, "+CREG", "+CREG: 2,1")); // Dummy
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::READ, "+CGREG", "+CGREG: 2,1")); // Dummy
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::READ, "+CEREG", "+CEREG: 2,1")); // Dummy
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::WRITE, "+CRC", nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::WRITE, "+CCWA", nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::WRITE, "+COPS", nullptr));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::READ, "+COPS", "+COPS: 0,0,\"Particle\""));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::TEST, "+IPR", "+IPR: (0,9600,19200,38400,57600,115200,230400,460800,921600),()"));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::READ, "+IPR", [](AtServerRequest* request, AtServerCommandType type, const char* command, void* data) -> int {
        auto stream = (SerialStream*)data;
        CHECK_TRUE(stream, SYSTEM_ERROR_INVALID_STATE);
        request->sendResponse("+IPR: %u", stream->baudrate());
        return 0;
    }, serial_.get()));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::WRITE, "+IPR", [](AtServerRequest* request, AtServerCommandType type, const char* command, void* data) -> int {
        auto stream = (SerialStream*)data;
        CHECK_TRUE(stream, SYSTEM_ERROR_INVALID_STATE);
        int v = 0;
        CHECK_TRUE(request->scanf("%d", &v) == 1, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK_TRUE(v > 0, SYSTEM_ERROR_INVALID_ARGUMENT);
        CHECK(stream->setBaudRate(v));
        return 0;
    }, serial_.get()));
    auto connectRequest = [](AtServerRequest* request, AtServerCommandType type, const char* command, void* data) -> int {
        auto client = (net::ppp::Client*)data;
        request->sendResponse("CONNECT %u", ((SerialStream*)request->server()->config().stream())->baudrate());
        request->setFinalResponse(AtServerRequest::CONNECT);
        request->server()->suspend();
        client->setOutputCallback([](const uint8_t* data, size_t size, void* ctx) -> int {
            // LOG(INFO, "output %u", size);
            auto c = (Stream*)ctx;
            int r = c->writeAll((const char*)data, size, 1000);
            if (!r) {
                return size;
            }
            return r;
        }, request->server()->config().stream());
        client->connect();
        client->notifyEvent(ppp::Client::EVENT_LOWER_UP);
        return 0;
    };
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::WILDCARD, "D*", connectRequest, &client_));
    server_->addCommandHandler(AtServerCommandHandler(AtServerCommandType::ONE_INT_ARG, "D", connectRequest, &client_));
    
    SPARK_ASSERT(os_thread_create(&thread_, "pppserver", OS_THREAD_PRIORITY_NETWORK, &PppServerNetif::loop, this, OS_THREAD_STACK_SIZE_DEFAULT_HIGH) == 0);
    return 0;
}

if_t PppServerNetif::interface() {
    return (if_t)client_.getIf();
}

void PppServerNetif::loop(void* arg) {
    PppServerNetif* self = static_cast<PppServerNetif*>(arg);
    while(!self->exit_) {
        auto ev = self->serial_->waitEvent(PPP_SERVER_NETIF_EVENT_EXIT, 0);
        if (ev & PPP_SERVER_NETIF_EVENT_EXIT) {
            break;
        }
        if (!self->server_->suspended()) {
            self->server_->process();
        } else {
            auto ev = self->serial_->waitEvent(SerialStream::READABLE | PPP_SERVER_NETIF_EVENT_EXIT, 1000);
            if (ev & PPP_SERVER_NETIF_EVENT_EXIT) {
                break;
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
    }

    self->down();

    os_thread_exit(nullptr);
}

int PppServerNetif::up() {
    CHECK(start());
    return SYSTEM_ERROR_NONE;
}

int PppServerNetif::down() {
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
    // FIXME: As long as the interface is initialized,
    // it must have been powered up as of right now.
    // The system network manager transit the interface to powering up,
    // we should always notify the event to transit the interface to powered up.
    notifyPowerState(IF_POWER_STATE_UP);
    return SYSTEM_ERROR_NONE;
}

int PppServerNetif::powerDown() {
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
        // Drop back into AT mode
        client_.notifyEvent(ppp::Client::EVENT_LOWER_DOWN);
        server_->resume();
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