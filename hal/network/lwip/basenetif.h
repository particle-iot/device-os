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

#ifndef HAL_NETWORK_LWIP_BASENETIF_H
#define HAL_NETWORK_LWIP_BASENETIF_H

#include <mutex>
#include <lwip/netif.h>
#include "ifapi.h"

namespace particle { namespace net {

class BaseNetif {
public:
    BaseNetif();
    virtual ~BaseNetif();

    virtual if_t interface();

    static int getClientDataId();

    virtual int powerUp() = 0;
    virtual int powerDown() = 0;

protected:
    void registerHandlers();

    virtual void ifEventHandler(const if_event* ev) = 0;
    virtual void netifEventHandler(netif_nsc_reason_t reason, const netif_ext_callback_args_t* args) = 0;

private:
    static void ifEventCb(void* arg, if_t iface, const if_event* ev);
    static void netifEventCb(netif* iface, netif_nsc_reason_t reason, const netif_ext_callback_args_t* args);

protected:
    netif netif_ = {};

private:
    netif_ext_callback_t netifEventHandlerCookie_;
    if_event_handler_cookie_t eventHandlerCookie_ = nullptr;
    static uint8_t clientDataId_;
    static std::once_flag once_;
};

} } /* particle::net */

#endif /* HAL_NETWORK_LWIP_BASENETIF_H */
