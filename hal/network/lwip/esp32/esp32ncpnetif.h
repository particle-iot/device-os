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

#ifndef LWIP_ESP32_NCP_NETIF_H
#define LWIP_ESP32_NCP_NETIF_H

#include "basenetif.h"
#include "concurrent_hal.h"
#include <atomic>
#include <lwip/netif.h>
#include <lwip/pbuf.h>
#include "network/ncp/wifi/wifi_network_manager.h"
#include "ncp_client.h"
#include <memory>

#ifdef __cplusplus

namespace particle { namespace net {

class Esp32NcpNetif : public BaseNetif {
public:
    Esp32NcpNetif();
    virtual ~Esp32NcpNetif();

    void init();
    void setWifiManager(particle::WifiNetworkManager* wifiMan);

    virtual int powerUp() override;
    virtual int powerDown() override;

    virtual int getPowerState(if_power_state_t* state) const override;
    virtual int getNcpState(unsigned int* state) const override;

    static int ncpDataHandlerCb(int id, const uint8_t* data, size_t size, void* ctx);
    static void ncpEventHandlerCb(const NcpEvent& ev, void* ctx);

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

    /* LwIP netif init callback */
    static err_t initCb(netif *netif);
    err_t initInterface();

    int queryMacAddress(MacAddress& mac);
    int configureMacAddress();
    
    static void loop(void* arg);

    /* LwIP netif linkoutput callback */
    static err_t linkOutputCb(netif* netif, pbuf* p);
    err_t linkOutput(pbuf* p);

    static void mempEventHandler(memp_t type, unsigned available, unsigned size, void* ctx);

private:
    os_thread_t thread_ = nullptr;
    os_semaphore_t netifSemaphore_ = nullptr;
    std::atomic_bool exit_;
    std::atomic<NetifEvent> lastNetifEvent_;
    std::atomic<NcpState> expectedNcpState_;
    std::atomic<NcpConnectionState> expectedConnectionState_;
    particle::WifiNetworkManager* wifiMan_ = nullptr;
    std::unique_ptr<char[]> hostname_;
};

} } // namespace particle::net

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWIP_ESP32_NCP_NETIF_H */
