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

#include "logging.h"
LOG_SOURCE_CATEGORY("net.br")

#include "border_router_manager.h"
#include "ot_api.h"
#include "system_error.h"
#include "nat64.h"
#include "dns64.h"
#include "thread_runner.h"
#include "random.h"
#include <mutex>
#include <openthread/border_router.h>
#include <openthread/thread_ftd.h>
#include <openthread/thread.h>
#include <openthread/instance.h>
#include <openthread/ip6.h>
#include <openthread/netdata.h>

using namespace particle;
using namespace particle::net;
using namespace particle::net::nat;

BorderRouterManager::BorderRouterManager()
        : started_(false),
          enabled_(false) {
}

BorderRouterManager::~BorderRouterManager() {
    stop();
}

BorderRouterManager* BorderRouterManager::instance() {
    static BorderRouterManager m;
    return &m;
}

int BorderRouterManager::start() {
    if (started_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    started_ = true;

    LOG(INFO, "Starting");

    if (!threadIface_) {
        threadIface_ = findThreadIface();
    }

    int r = SYSTEM_ERROR_NO_MEMORY;

    if (!threadIface_) {
        r = SYSTEM_ERROR_NOT_FOUND;
        goto cleanup;
    }

    nat64_.reset(new Nat64());
    if (!nat64_) {
        goto cleanup;
    }

    dns64_.reset(new Dns64());
    if (!dns64_) {
        goto cleanup;
    }

    dns64Runner_.reset(new ThreadRunner());
    if (!dns64Runner_) {
        goto cleanup;
    }

    r = startServices();
    if (r) {
        goto cleanup;
    }

    return r;

cleanup:
    LOG(ERROR, "Error %d", r);
    stop();
    return r;
}

int BorderRouterManager::stop() {
    if (!started_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    LOG(INFO, "Stopping");

    stopServices();

    nat64_.reset();
    dns64Runner_.reset();
    dns64_.reset();

    started_ = false;
    return 0;
}

std::shared_ptr<Nat64> BorderRouterManager::getNat64() const {
    return nat64_;
}

if_t BorderRouterManager::findThreadIface() const {
    if_t iface = nullptr;
    /* FIXME: hardcoded name for now */
    if_get_by_name("th1", &iface);

    return iface;
}

int BorderRouterManager::startServices() {
    ip6_addr_t pref64 = nat64_->getPref64();
    LOG(TRACE, "Starting DNS64");
    CHECK(dns64_->init(threadIface_, pref64));
    CHECK(dns64Runner_->init(dns64_.get()));

    {
        particle::net::ot::ThreadLock lk;
        /* Register OpenThread state changed callback */
        otSetStateChangedCallback(ot_get_instance(), &otStateChangedCb, this);
    }

    ifEventCookie_ = if_event_handler_add_if(threadIface_, &ifEventHandlerCb, this);

    LOG(TRACE, "Enabling NAT64");
    Rule r(threadIface_, nullptr);
    nat64_->enable(r);

    /* Enable if possible */
    enable();

    return 0;
}

int BorderRouterManager::stopServices() {
    {
        particle::net::ot::ThreadLock lk;
        /* Register OpenThread state changed callback */
        otRemoveStateChangeCallback(ot_get_instance(), &otStateChangedCb, this);
    }

    if (ifEventCookie_) {
        if_event_handler_del(ifEventCookie_);
        ifEventCookie_ = nullptr;
    }

    LOG(TRACE, "Disabling NAT64");
    nat64_->disable(nullptr);

    LOG(TRACE, "Stopping DNS64");
    if (dns64Runner_) {
        dns64Runner_->destroy();
    }
    if (dns64_) {
        dns64_->destroy();
    }

    /* Disable if needed */
    disable();

    return 0;
}

int BorderRouterManager::enable() {
    particle::net::ot::ThreadLock lk;

    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otBorderRouterConfig config = {};
    bool presentLocal = false;
    while (otBorderRouterGetNextOnMeshPrefix(ot_get_instance(), &iterator, &config) == OT_ERROR_NONE) {
        presentLocal = true;
        break;
    }

    if (!presentLocal) {
        LOG(TRACE, "No prefix in local network data");
        if (!config_.mOnMesh) {
            LOG(TRACE, "Generating border router configuration");
            // FIXME: for now HIGH for Xenon with ethernet shield, medium for Argon,
            // and low for Boron
#if PLATFORM_ID == PLATFORM_XENON || PLATFORM_ID == PLATFORM_XENON_SOM
            config_.mPreference = OT_ROUTE_PREFERENCE_HIGH;
#elif PLATFORM_ID == PLATFORM_ARGON || PLATFORM_ID == PLATFORM_ARGON_SOM
            config_.mPreference = OT_ROUTE_PREFERENCE_MED;
#elif PLATFORM_ID == PLATFORM_BORON || PLATFORM_ID == PLATFORM_BORON_SOM
            config_.mPreference = OT_ROUTE_PREFERENCE_LOW;
#endif
            config_.mPreferred = 1;
            config_.mSlaac = 1;
            config_.mDefaultRoute = 1;
            config_.mOnMesh = 1;
            config_.mStable = 1;

            if (ot_get_border_router_prefix(ot_get_instance(), 0, &config_.mPrefix) ||
                    config_.mPrefix.mLength != 64) {
                // Failed to retrieve from persistent storage
                // Generate new prefix
                config_.mPrefix.mPrefix.mFields.m8[0] = 0xfd;
                Random rand;
                rand.gen((char*)config_.mPrefix.mPrefix.mFields.m8 + 1, 5); // Generate global ID
                config_.mPrefix.mLength = 64;
                ot_set_border_router_prefix(ot_get_instance(), 0, &config_.mPrefix);
            }
        }

        LOG(TRACE, "Adding prefix into local network data");
        CHECK(otBorderRouterAddOnMeshPrefix(ot_get_instance(), &config_));
    }

    iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    bool presentLeader = false;
    while (otNetDataGetNextOnMeshPrefix(ot_get_instance(), &iterator, &config) == OT_ERROR_NONE) {
        if (config.mRloc16 == otThreadGetRloc16(ot_get_instance())) {
            if (config_.mPrefix.mLength == config.mPrefix.mLength && otIp6PrefixMatch(&config.mPrefix.mPrefix, &config_.mPrefix.mPrefix) >= config_.mPrefix.mLength) {
                presentLeader = true;
                break;
            }
        }
    }

    if (isIfUp() && !presentLeader) {
        LOG(TRACE, "Trying to synchronize local netdata with leader");
        if (otThreadGetDeviceRole(ot_get_instance()) == OT_DEVICE_ROLE_CHILD) {
            otThreadBecomeRouter(ot_get_instance());
        }
        otBorderRouterRegister(ot_get_instance());
    }

    enabled_ = true;

    return 0;
}

int BorderRouterManager::disable() {
    particle::net::ot::ThreadLock lk;

    if (config_.mOnMesh) {
        LOG(TRACE, "Disabling border routing");
        otBorderRouterRemoveOnMeshPrefix(ot_get_instance(), &config_.mPrefix);
        otBorderRouterRegister(ot_get_instance());
    }

    enabled_ = false;

    return 0;
}

bool BorderRouterManager::isIfUp() const {
    unsigned int flags = 0;
    if_get_flags(threadIface_, &flags);

    if (flags & IFF_UP) {
        return true;
    }

    return false;
}

void BorderRouterManager::otStateChangedCb(uint32_t flags, void* ctx) {
    auto self = static_cast<BorderRouterManager*>(ctx);
    self->otStateChanged(flags);
}

void BorderRouterManager::otStateChanged(uint32_t flags) {
    if (!isIfUp() || !enabled_) {
        /* Ignore */
        return;
    }

    /* link state */
    if (flags & (OT_CHANGED_THREAD_ROLE | OT_CHANGED_THREAD_NETDATA)) {
        switch (otThreadGetDeviceRole(ot_get_instance())) {
            /* We will only disable border routing when Thread interface is downed */
            case OT_DEVICE_ROLE_DISABLED:
            case OT_DEVICE_ROLE_DETACHED: {
                break;
            }
            default: {
                enable();
                break;
            }
        }
    }
}

void BorderRouterManager::ifEventHandlerCb(void* arg, if_t iface, const struct if_event* ev) {
    auto self = static_cast<BorderRouterManager*>(arg);
    self->ifEventHandler(iface, ev);
}

void BorderRouterManager::ifEventHandler(if_t iface, const struct if_event* ev) {
    if (ev->ev_type == IF_EVENT_STATE) {
        if (ev->ev_if_state->state) {
            enable();
        } else {
            disable();
        }
    }
}
