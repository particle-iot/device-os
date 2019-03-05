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

#include "basenetif.h"
#include "lwiplock.h"

using namespace particle::net;

uint8_t BaseNetif::clientDataId_;
std::once_flag BaseNetif::once_;

BaseNetif::BaseNetif() {
}

BaseNetif::~BaseNetif() {
    LwipTcpIpCoreLock lk;
    netif_remove_ext_callback(&netifEventHandlerCookie_);
    if_event_handler_del(eventHandlerCookie_);
}

int BaseNetif::getClientDataId() {
    return clientDataId_;
}

void BaseNetif::registerHandlers() {
    LwipTcpIpCoreLock lk;
    std::call_once(once_, []() {
        clientDataId_ = netif_alloc_client_data_id();
    });
    netif_set_client_data(interface(), clientDataId_, (void*)this);
    netif_add_ext_callback(&netifEventHandlerCookie_, &BaseNetif::netifEventCb);
    eventHandlerCookie_ = if_event_handler_self(interface(), &BaseNetif::ifEventCb, this);
}

if_t BaseNetif::interface() {
    return (if_t)&netif_;
}

void BaseNetif::ifEventCb(void* arg, if_t iface, const if_event* ev) {
    BaseNetif* self = static_cast<BaseNetif*>(arg);
    if (self && self->interface() == iface) {
        self->ifEventHandler(ev);
    }
}

void BaseNetif::netifEventCb(netif* iface, netif_nsc_reason_t reason, const netif_ext_callback_args_t* args) {
    BaseNetif* self = static_cast<BaseNetif*>(netif_get_client_data(iface, clientDataId_));
    if (self && self->interface() == iface) {
        self->netifEventHandler(reason, args);
    }
}
