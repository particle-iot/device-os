/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include "ifapi.h"
#include "wiznet/wiznetif.h"
#include <mutex>
#include <nrf52840.h>
#include "random.h"
#include <malloc.h>
#include "serial_stream.h"
#include "usart_hal.h"
#include "network/ncp/cellular/ncp.h"
#include "pppncpnetif.h"
#include "network/ncp_client/quectel/quectel_ncp_client.h"
#include "network/ncp_client/esp32/esp32_ncp_client.h"
#include "network/ncp/wifi/wifi_network_manager.h"
#include "network/ncp/wifi/ncp.h"
#include "esp32/esp32ncpnetif.h"
#include "platform_ncp.h"
#include "lwip_util.h"
#include "core_hal.h"

using namespace particle;
using namespace particle::net;

namespace {

/* en2 - Ethernet FeatherWing */
BaseNetif* en2 = nullptr;
/* pp3 - Cellular */
BaseNetif* pp3 = nullptr;
/* wl4 - ESP32 NCP Station */
BaseNetif* wl4 = nullptr;

bool netifCanForwardIpv4(netif* iface) {
    if (iface && netif_is_up(iface) && netif_is_link_up(iface)) {
        auto addr = netif_ip_addr4(iface);
        auto mask = netif_ip_netmask4(iface);
        auto gw = netif_ip_gw4(iface);
        if (!ip_addr_isany(addr) && !ip_addr_isany(mask) && !ip_addr_isany(gw)) {
            return true;
        }
    }

    return false;
}

class CellularNetworkManagerInit {
public:
    CellularNetworkManagerInit() {
        const int r = init();
        SPARK_ASSERT(r == 0);
    }

    CellularNetworkManager* instance() const {
        return mgr_.get();
    }

private:
    std::unique_ptr<CellularNcpClient> client_;
    std::unique_ptr<CellularNetworkManager> mgr_;

    int init() {
        // Get active SIM card
        SimType sim = SimType::INVALID;
        CHECK(CellularNetworkManager::getActiveSim(&sim));
        CellularNcpClientConfig conf;
        conf.simType(sim);
        conf.ncpIdentifier(platform_primary_ncp_identifier());
        conf.eventHandler(PppNcpNetif::ncpEventHandlerCb, pp3);
        conf.dataHandler(PppNcpNetif::ncpDataHandlerCb, pp3);
        // Initialize NCP client
        std::unique_ptr<CellularNcpClient> client;
        client.reset(new(std::nothrow) QuectelNcpClient);
        CHECK_TRUE(client, SYSTEM_ERROR_NO_MEMORY);
        CHECK(client->init(conf));
        // Initialize network manager
        mgr_.reset(new(std::nothrow) CellularNetworkManager(client.get()));
        CHECK_TRUE(mgr_, SYSTEM_ERROR_NO_MEMORY);
        client_ = std::move(client);
        return 0;
    }
};

class WifiNetworkManagerInit {
public:
    WifiNetworkManagerInit() {
        const int ret = init();
        SPARK_ASSERT(ret == 0);
    }

    WifiNetworkManager* instance() const {
        return mgr_.get();
    }

private:
    std::unique_ptr<WifiNcpClient> ncpClient_;
    std::unique_ptr<WifiNetworkManager> mgr_;

    int init() {
        // Initialize NCP client
        std::unique_ptr<WifiNcpClient> ncpClient(new(std::nothrow) Esp32NcpClient);
        CHECK_TRUE(ncpClient, SYSTEM_ERROR_NO_MEMORY);
        auto conf = NcpClientConfig()
                .eventHandler(Esp32NcpNetif::ncpEventHandlerCb, wl4)
                .dataHandler(Esp32NcpNetif::ncpDataHandlerCb, wl4);
        CHECK(ncpClient->init(std::move(conf)));
        // Initialize network manager
        mgr_.reset(new(std::nothrow) WifiNetworkManager(ncpClient.get()));
        CHECK_TRUE(mgr_, SYSTEM_ERROR_NO_MEMORY);
        ncpClient_ = std::move(ncpClient);
        return 0;
    }
};

} /* anonymous */

namespace particle {

CellularNetworkManager* cellularNetworkManager() {
    static CellularNetworkManagerInit mgr;
    return mgr.instance();
}

WifiNetworkManager* wifiNetworkManager() {
    static WifiNetworkManagerInit mgr;
    return mgr.instance();
}

} // particle

int if_init_platform(void*) {
    /* lo0 (created by LwIP) */

    /* th1 - OpenThread (Deprecated) */
    reserve_netif_index();

    /* en2 - Ethernet FeatherWing (optional) */
    uint8_t mac[6] = {};
    {
        const uint32_t lsb = __builtin_bswap32(NRF_FICR->DEVICEADDR[0]);
        const uint32_t msb = NRF_FICR->DEVICEADDR[1] & 0xffff;
        memcpy(mac + 2, &lsb, sizeof(lsb));
        mac[0] = msb >> 8;
        mac[1] = msb;
        /* Drop 'multicast' bit */
        mac[0] &= 0b11111110;
        /* Set 'locally administered' bit */
        mac[0] |= 0b10;
    }

    if (HAL_Feature_Get(FEATURE_ETHERNET_DETECTION)) {
        en2 = new WizNetif(HAL_SPI_INTERFACE1, D2, D6, D7, mac);
    }

    uint8_t dummy;
    if (!en2 || if_get_index(en2->interface(), &dummy)) {
        /* No en2 present */
        delete en2;
        en2 = nullptr;
        reserve_netif_index();
    }

    /* pp3 - Cellular */
    pp3 = new PppNcpNetif();
    if (pp3) {
        ((PppNcpNetif*)pp3)->setCellularManager(cellularNetworkManager());
        ((PppNcpNetif*)pp3)->init();
    }

    /* wl4 - ESP32 NCP Station */
    wl4 = new Esp32NcpNetif();
    if (wl4) {
        ((Esp32NcpNetif*)wl4)->setWifiManager(wifiNetworkManager());
        ((Esp32NcpNetif*)wl4)->init();
    }

    auto m = mallinfo();
    const size_t total = m.uordblks + m.fordblks;
    LOG(TRACE, "Heap: %lu/%lu Kbytes used", m.uordblks / 1000, total / 1000);

    return 0;
}

extern "C" {

struct netif* lwip_hook_ip4_route_src(const ip4_addr_t* src, const ip4_addr_t* dst) {
    if (src == nullptr) {
        if (en2 && netifCanForwardIpv4(en2->interface())) {
            return en2->interface();
        } else if (pp3 && netifCanForwardIpv4(pp3->interface())) {
            return pp3->interface();
        }
    }

    return nullptr;
}

}
