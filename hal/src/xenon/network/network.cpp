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
#include <nrf52840.h>

using namespace particle::net;

namespace {

/* th2 - OpenThread */
BaseNetif* th2 = nullptr;
/* en3 - Ethernet FeatherWing */
BaseNetif* en3 = nullptr;

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
    return 0;
}
