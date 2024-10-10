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

#ifndef LWIP_PPP_SERVER_NETIF_H
#define LWIP_PPP_SERVER_NETIF_H

#include "basenetif.h"
#include "concurrent_hal.h"
#include "static_recursive_mutex.h"
#include <atomic>
#include <lwip/netif.h>
#include <lwip/pbuf.h>
#include "ppp_client.h"
#include "serial_stream.h"
#include "at_server.h"

#ifdef __cplusplus

namespace particle {

class ThreadRunner;
    
namespace net {

class Dns;

namespace nat {

class Nat64;

}

class PppServerNetif : public BaseNetif {
public:
    PppServerNetif();
    virtual ~PppServerNetif();

    void init();

    virtual if_t interface() override;

    virtual int powerUp() override;
    virtual int powerDown() override;

    virtual int getPowerState(if_power_state_t* state) const override;
    virtual int getNcpState(unsigned int* state) const override;

    void notifyPowerState(if_power_state_t state);

    int start();

protected:
    virtual void ifEventHandler(const if_event* ev) override;
    virtual void netifEventHandler(netif_nsc_reason_t reason, const netif_ext_callback_args_t* args) override;

private:
    enum class NetifEvent {
        None = 0,
        Up = 1,
        Down = 2,
        Exit = 3,
        PowerOff = 4,
        PowerOn = 5
    };

    int up();
    int down();

    int upImpl();
    int downImpl();

    void setExpectedInternalState(NetifEvent ev);

    static void loop(void* arg);

    static void pppEventHandlerCb(particle::net::ppp::Client* c, uint64_t ev, int data, void* ctx);
    void pppEventHandler(uint64_t ev, int data);
    static void mempEventHandler(memp_t type, unsigned available, unsigned size, void* ctx);

    int request(if_req_driver_specific* req, size_t size) override;

private:
    os_thread_t thread_ = nullptr;
    std::atomic_bool exit_;
    particle::net::ppp::Client client_;
    std::atomic<if_power_state_t> pwrState_;
    std::unique_ptr<SerialStream> serial_;
    std::unique_ptr<Dns> dns_;
    std::unique_ptr<::particle::ThreadRunner> dnsRunner_;
    std::unique_ptr<particle::net::nat::Nat64> nat_;
    if_req_ppp_server_uart_settings settings_;
    std::unique_ptr<AtServer> server_;

};

} } // namespace particle::net

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWIP_PPP_SERVER_NETIF_H */
