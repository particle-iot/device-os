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
#include <lwip/timeouts.h>
#include <mutex>
#include <openthread/border_router.h>
#include <openthread/thread_ftd.h>
#include <openthread/ip6.h>
#include <nrf52840.h>

using namespace particle::net;
using namespace particle::net::nat;

namespace {

/* th2 - OpenThread */
BaseNetif* th2 = nullptr;
/* en3 - Ethernet FeatherWing */
BaseNetif* en3 = nullptr;

Nat64* nat64 = nullptr;

void nat64_tick_do() {
    sys_timeout(1000, [](void* arg) -> void {
        nat64->timeout(1000);
        nat64_tick_do();
    }, nullptr);
}

} /* anonymous */

int if_init_platform(void*) {
    /* lo1 (created by LwIP) */

    /* th2 - OpenThread */
    th2 = new OpenThreadNetif(ot_get_instance());

    /* en3 - Ethernet FeatherWing (optional) */
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
    en3 = new WizNetif(HAL_SPI_INTERFACE1, D5, D3, D4, mac);
    uint8_t dummy;
    if (if_get_index(en3->interface(), &dummy)) {
        /* No en3 present */
        delete en3;
        en3 = nullptr;
    } else {
        unsigned int flags = 0;
        if_get_flags(en3->interface(), &flags);
        flags |= (IFF_UP);
        if_set_flags(en3->interface(), flags);

        flags = 0;
        if_get_xflags(en3->interface(), &flags);
        flags |= IFXF_DHCP;
        if_set_xflags(en3->interface(), flags);

        nat64 = new Nat64();
        ip_addr_t prefix;
        ipaddr_aton("64:ff9b::", &prefix);
        Rule r(th2->interface(), en3->interface());
        nat64->enable(r);

        nat64_tick_do();

        auto thread = ot_get_instance();
        otBorderRouterConfig config = {};
        config.mPreference = 0x01;
        config.mPreferred = 1;
        config.mSlaac = 1;
        config.mDefaultRoute = 1;
        config.mOnMesh = 1;
        config.mStable = 1;

        std::lock_guard<particle::net::ot::ThreadLock> lk(particle::net::ot::ThreadLock());
        otIp6AddressFromString("fd11:23::", &config.mPrefix.mPrefix);
        config.mPrefix.mLength = 64;

        otBorderRouterAddOnMeshPrefix(thread, &config);
        otThreadBecomeRouter(thread);
        otBorderRouterRegister(thread);
    }
    return 0;
}

extern "C" {

struct netif* lwip_hook_ip4_route_src(const ip4_addr_t* src, const ip4_addr_t* dst) {
    if (en3) {
        return en3->interface();
    }

    return nullptr;
}

int lwip_hook_ip6_forward_pre_routing(struct pbuf* p, struct ip6_hdr* ip6hdr, struct netif* inp, u32_t* flags) {
    if (nat64) {
        return nat64->ip6Input(p, ip6hdr, inp);
    }

    /* Do not forward */
    return 1;
}

int lwip_hook_ip4_input_pre_upper_layers(struct pbuf* p, const struct ip_hdr* iphdr, struct netif* inp) {
    if (nat64) {
        nat64->ip4Input(p, (ip_hdr*)iphdr, inp);
    }

    /* Try to handle locally if not consumed by NAT64 */
    return 0;
}

}
