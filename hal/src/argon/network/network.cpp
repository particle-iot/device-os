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

#define NO_STATIC_ASSERT
#include "ifapi.h"
#include "ot_api.h"
#include "openthread/lwip_openthreadif.h"
#include "wiznet/wiznetif.h"
#include "nat64.h"
#include <mutex>
#include <memory>
#include <nrf52840.h>
#include "random.h"
#include "check.h"
#include "border_router_manager.h"
#include <malloc.h>
#include "esp32_ncp_client.h"
#include "wifi_network_manager.h"
#include "ncp.h"
#include "debug.h"
#include "esp32/esp32ncpnetif.h"
#include "lwip_util.h"
#include "core_hal.h"

using namespace particle;
using namespace particle::net;
using namespace particle::net::nat;

namespace particle {

namespace {

/* th1 - OpenThread */
BaseNetif* th1 = nullptr;
/* en2 - Ethernet FeatherWing */
BaseNetif* en2 = nullptr;
/* wl3 - ESP32 NCP Station */
BaseNetif* wl3 = nullptr;
/* wl4 - ESP32 NCP Access Point */
BaseNetif* wl4 = nullptr;

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
                .eventHandler(Esp32NcpNetif::ncpEventHandlerCb, wl3)
                .dataHandler(Esp32NcpNetif::ncpDataHandlerCb, wl3);
        CHECK(ncpClient->init(std::move(conf)));
        // Initialize network manager
        mgr_.reset(new(std::nothrow) WifiNetworkManager(ncpClient.get()));
        CHECK_TRUE(mgr_, SYSTEM_ERROR_NO_MEMORY);
        ncpClient_ = std::move(ncpClient);
        return 0;
    }
};

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

} // unnamed

WifiNetworkManager* wifiNetworkManager() {
    static WifiNetworkManagerInit mgr;
    return mgr.instance();
}

} // particle

int if_init_platform(void*) {
    CHECK(ot_init(nullptr, nullptr));

    /* lo0 (created by LwIP) */

    /* th1 - OpenThread */
    th1 = new OpenThreadNetif(ot_get_instance());

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
#if PLATFORM_ID == PLATFORM_ARGON
        en2 = new WizNetif(HAL_SPI_INTERFACE1, D5, D3, D4, mac);
#else // A SoM
        en2 = new WizNetif(HAL_SPI_INTERFACE1, D8, A7, D22, mac);
#endif
    }

    uint8_t dummy;
    if (!en2 || if_get_index(en2->interface(), &dummy)) {
        /* No en2 present */
        delete en2;
        en2 = nullptr;
        reserve_netif_index();
    }

    /* wl3 - ESP32 NCP Station */
    wl3 = new Esp32NcpNetif();
    if (wl3) {
        ((Esp32NcpNetif*)wl3)->setWifiManager(wifiNetworkManager());
        ((Esp32NcpNetif*)wl3)->init();
    }

    /* TODO: wl4 - ESP32 NCP Access Point */
    (void)wl4;

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
        } else if (wl3 && netifCanForwardIpv4(wl3->interface())) {
            return wl3->interface();
        }
    }

    return nullptr;
}

int lwip_hook_ip6_forward_pre_routing(struct pbuf* p, struct ip6_hdr* ip6hdr, struct netif* inp, u32_t* flags) {
    auto nat64 = BorderRouterManager::instance()->getNat64();
    if (nat64) {
        return nat64->ip6Input(p, ip6hdr, inp);
    }

    /* Do not forward */
    return 1;
}

int lwip_hook_ip4_input_pre_upper_layers(struct pbuf* p, const struct ip_hdr* iphdr, struct netif* inp) {
    auto nat64 = BorderRouterManager::instance()->getNat64();
    if (nat64) {
        int r = nat64->ip4Input(p, (ip_hdr*)iphdr, inp);
        if (r) {
            /* Ip4 hooks do not free the packet if it has been handled by the hook */
            pbuf_free(p);
        }

        return r;
    }

    /* Try to handle locally if not consumed by NAT64 */
    return 0;
}

}
