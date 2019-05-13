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

#include "ifapi.h"
#include "ot_api.h"
#include "openthread/lwip_openthreadif.h"
#include "wiznet/wiznetif.h"
#include "nat64.h"
#include <mutex>
#include <nrf52840.h>
#include "random.h"
#include "border_router_manager.h"
#include <malloc.h>
#include "lwip_util.h"
#include "core_hal.h"
#include "check.h"

using namespace particle;
using namespace particle::net;
using namespace particle::net::nat;

namespace {

/* th1 - OpenThread */
BaseNetif* th1 = nullptr;
/* en2 - Ethernet FeatherWing */
BaseNetif* en2 = nullptr;

} /* anonymous */

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
#if PLATFORM_ID == PLATFORM_XENON
        en2 = new WizNetif(HAL_SPI_INTERFACE1, D5, D3, D4, mac);
#else // X SoM
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

    auto m = mallinfo();
    const size_t total = m.uordblks + m.fordblks;
    LOG(TRACE, "Heap: %lu/%lu Kbytes used", m.uordblks / 1000, total / 1000);

    return 0;
}

extern "C" {

struct netif* lwip_hook_ip4_route_src(const ip4_addr_t* src, const ip4_addr_t* dst) {
    if (en2) {
        return en2->interface();
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
