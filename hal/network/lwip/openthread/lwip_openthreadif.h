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

#ifndef LWIP_OPENTHREAD_IF_H
#define LWIP_OPENTHREAD_IF_H

#include <openthread-core-config.h>
#include <lwip/netif.h>
#include <lwip/pbuf.h>
#include <openthread/message.h>
#include <openthread/ip6.h>
#include <openthread/netdata.h>
#include "basenetif.h"

#ifdef __cplusplus

namespace particle { namespace net {

class OpenThreadNetif : public BaseNetif {
public:
    OpenThreadNetif(otInstance* ot = nullptr);
    virtual ~OpenThreadNetif();

    otInstance* getOtInstance();

    virtual int powerUp() override;
    virtual int powerDown() override;

protected:
    virtual void ifEventHandler(const if_event* ev) override;
    virtual void netifEventHandler(netif_nsc_reason_t reason, const netif_ext_callback_args_t* args) override;

private:
    /* LwIP netif init callback */
    static err_t initCb(netif *netif);
    /* LwIP netif output_ip6 callback */
    static err_t outputIp6Cb(netif* netif, pbuf* p, const ip6_addr_t* addr);
    /* LwIP netif mld_mac_filter callback */
    static err_t mldMacFilterCb(netif* netif, const ip6_addr_t *group,
            netif_mac_filter_action action);

    /* OpenThread receive callback */
    static void otReceiveCb(otMessage* msg, void* ctx);
    /* OpenThread state changed callback */
    static void otStateChangedCb(uint32_t flags, void* ctx);

    void input(otMessage* message);
    void stateChanged(uint32_t flags);
    static void debugLogOtStateChange(uint32_t flags);

    void refreshIpAddresses();
    void syncIpState();

    int up();
    int down();

    void setDns(const ip6_addr_t* addr);

private:
    otInstance* ot_ = nullptr;
    otNetifAddress addresses_[OPENTHREAD_CONFIG_MAX_EXT_IP_ADDRS] = {};
    otBorderRouterConfig abr_ = {};
};

} } // namespace particle::net

#endif /* __cplusplus */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWIP_OPENTHREAD_IF_H */
