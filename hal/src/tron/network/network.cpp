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
#include "wiznet/wiznetif.h"
#include "wiznet/wiznetif_config.h"
#include <mutex>
#include <memory>
#include "random.h"
#include "check.h"
#include <malloc.h>
#include "network/ncp_client/realtek/rtl_ncp_client.h"
#include "network/ncp/wifi/wifi_network_manager.h"
#include "network/ncp/wifi/ncp.h"
#include "debug.h"
#include "realtek/rtlncpnetif.h"
#include "lwip_util.h"
#include "core_hal.h"
#include "deviceid_hal.h"


using namespace particle;
using namespace particle::net;

namespace particle {

namespace {

/* en2 - Ethernet FeatherWing */
BaseNetif* en2 = nullptr;
/* wl3 - Realtek NCP Station */
BaseNetif* wl3 = nullptr;
/* wl4 - Realtek NCP Access Point */
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
        std::unique_ptr<WifiNcpClient> ncpClient(new(std::nothrow) RealtekNcpClient);
        CHECK_TRUE(ncpClient, SYSTEM_ERROR_NO_MEMORY);
        auto conf = NcpClientConfig()
                .eventHandler(RealtekNcpNetif::ncpEventHandlerCb, wl3)
                .dataHandler(RealtekNcpNetif::ncpDataHandlerCb, wl3);
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
    /* lo0 (created by LwIP) */

    /* th1 - OpenThread (Deprecated) */
    reserve_netif_index();

    /* en2 - Ethernet FeatherWing (optional) */
    uint8_t mac[HAL_DEVICE_MAC_ADDR_SIZE] = {};
    CHECK(hal_get_mac_address(HAL_DEVICE_MAC_ETHERNET, mac, HAL_DEVICE_MAC_ADDR_SIZE, nullptr));

    if (HAL_Feature_Get(FEATURE_ETHERNET_DETECTION)) {
        WizNetifConfigData wizNetifConfigData;
        wizNetifConfigData.size = sizeof(WizNetifConfigData);
        wizNetifConfigData.version = WIZNETIF_CONFIG_DATA_VERSION;
        WizNetifConfig::instance()->getConfigData(&wizNetifConfigData);
        en2 = new WizNetif(HAL_SPI_INTERFACE1, wizNetifConfigData.cs_pin, wizNetifConfigData.reset_pin, wizNetifConfigData.int_pin, mac);
    }

    uint8_t dummy;
    if (!en2 || if_get_index(en2->interface(), &dummy)) {
        /* No en2 present */
        delete en2;
        en2 = nullptr;
        reserve_netif_index();
    }

    /* wl3 - Realtek NCP Station */
    wl3 = new RealtekNcpNetif();
    if (wl3) {
        ((RealtekNcpNetif*)wl3)->setWifiManager(wifiNetworkManager());
        ((RealtekNcpNetif*)wl3)->init();
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

unsigned char* rltk_wlan_get_ip(int idx) {
    return (uint8_t *) &(wl3->interface()->ip_addr);
}

unsigned char* rltk_wlan_get_gw(int idx) {
    return (uint8_t *) &(wl3->interface()->gw);
}

unsigned char* rltk_wlan_get_gwmask(int idx) {
    return (uint8_t *) &(wl3->interface()->netmask);
}

void rltk_wlan_set_netif_info(int idx_wlan, void* dev, unsigned char* dev_addr) {
    LOG(INFO, "rltk_wlan_set_netif_info: %d, %02x:%02x:%02x:%02x:%02x:%02x", idx_wlan,
        dev_addr[0], dev_addr[1], dev_addr[2], dev_addr[3], dev_addr[4], dev_addr[5]);
    if (wl3) {
        memcpy(wl3->interface()->hwaddr, dev_addr, sizeof(wl3->interface()->hwaddr));
    }
}

void netif_rx(int idx, unsigned int len) {
    // LOG(INFO, "netif_rx %d %u", idx, len);
    RealtekNcpNetif::ncpDataHandlerCb(0, nullptr, len, wl3);
}

int netif_is_valid_IP(int idx, unsigned char *ip_dest) {
    // Let LwIP stack handle this
	return 1;
}

}
