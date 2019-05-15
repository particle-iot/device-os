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

#ifndef BORDER_ROUTER_MANAGER_H
#define BORDER_ROUTER_MANAGER_H

#include "ifapi.h"
#include <openthread/instance.h>
#include <openthread/netdata.h>
#include <memory>
#include <atomic>

namespace particle {

/* Forward declaration */
class ThreadRunner;

namespace net {

namespace nat {
/* Forward declaration */
class Nat64;
} /* nat */

/* Forward declaration */
class Dns64;

class BorderRouterManager {
public:
    ~BorderRouterManager();

    static BorderRouterManager* instance();

    int start();
    int stop();

    std::shared_ptr<nat::Nat64> getNat64() const;

protected:
    BorderRouterManager();

private:
    if_t findThreadIface() const;

    int startServices();
    int stopServices();

    int enable();
    int disable();
    int syncWithLeader();

    bool isIfUp() const;

    /* OpenThread state changed callback */
    static void otStateChangedCb(uint32_t flags, void* ctx);
    void otStateChanged(uint32_t flags);

    static void ifEventHandlerCb(void* arg, if_t iface, const struct if_event* ev);
    void ifEventHandler(if_t iface, const struct if_event* ev);

private:
    std::atomic_bool started_;
    std::atomic_bool enabled_;
    if_t threadIface_ = nullptr;

    /* TODO: multiple prefixes */
    otBorderRouterConfig config_ = {};

    if_event_handler_cookie_t ifEventCookie_ = nullptr;

    std::shared_ptr<nat::Nat64> nat64_;
    std::unique_ptr<Dns64> dns64_;
    std::unique_ptr<::particle::ThreadRunner> dns64Runner_;
};

} } /* particle::net */

#endif /* BORDER_ROUTER_MANAGER_H */
