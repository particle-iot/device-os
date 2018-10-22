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

#include "system_openthread.h"

#if HAL_PLATFORM_OPENTHREAD

#include "system_error.h"
#include "service_debug.h"
#include "logging.h"

#include <openthread-core-config.h>

#include <openthread/openthread.h>
#include <openthread/thread.h>
#include <openthread/instance.h>
#include <openthread/commissioner.h>
#include <openthread/joiner.h>
#include <openthread/platform.h>
#include <openthread/platform/settings.h>

#define CHECK_THREAD(_expr) \
        do { \
            const otError ret = _expr; \
            if (ret != OT_ERROR_NONE) { \
                LOG_DEBUG(ERROR, #_expr " failed: %d", (int)ret); \
                return ::particle::system::threadToSystemError(ret); \
            } \
        } while (false)

LOG_SOURCE_CATEGORY("system.ot");

namespace particle {

namespace system {

namespace {

const char* deviceRoleStr(otDeviceRole role) {
    switch (role) {
    case OT_DEVICE_ROLE_DISABLED:
        return "disabled";
    case OT_DEVICE_ROLE_DETACHED:
        return "detached";
    case OT_DEVICE_ROLE_CHILD:
        return "child";
    case OT_DEVICE_ROLE_ROUTER:
        return "router";
    case OT_DEVICE_ROLE_LEADER:
        return "leader";
    default:
        return "unknown";
    }
}

const char* commissionerStateStr(otCommissionerState state) {
    switch (state) {
    case OT_COMMISSIONER_STATE_DISABLED:
        return "disabled";
    case OT_COMMISSIONER_STATE_PETITION:
        return "petition";
    case OT_COMMISSIONER_STATE_ACTIVE:
        return "active";
    default:
        return "unknown";
    }
}

const char* joinerStateStr(otJoinerState state) {
    switch (state) {
    case OT_JOINER_STATE_IDLE:
        return "idle";
    case OT_JOINER_STATE_DISCOVER:
        return "discover";
    case OT_JOINER_STATE_CONNECT:
        return "connect";
    case OT_JOINER_STATE_CONNECTED:
        return "connected";
    case OT_JOINER_STATE_ENTRUST:
        return "entrust";
    case OT_JOINER_STATE_JOINED:
        return "joined";
    default:
        return "unknown";
    }
}

void threadStateChanged(uint32_t flags, void* data) {
    const auto thread = (otInstance*)data;
    if (flags & OT_CHANGED_IP6_ADDRESS_ADDED) {
        LOG(TRACE, "IPv6 address was added");
    }
    if (flags & OT_CHANGED_IP6_ADDRESS_REMOVED) {
        LOG(TRACE, "IPv6 address was removed");
    }
    if (flags & OT_CHANGED_THREAD_ROLE) {
        const otDeviceRole role = otThreadGetDeviceRole(thread);
        LOG(INFO, "Role changed: %s", deviceRoleStr(role));
    }
    if (flags & OT_CHANGED_THREAD_LL_ADDR) {
        LOG(TRACE, "Link-local address changed");
    }
    if (flags & OT_CHANGED_THREAD_ML_ADDR) {
        LOG(TRACE, "Mesh-local address changed");
    }
    if (flags & OT_CHANGED_THREAD_RLOC_ADDED) {
        LOG(TRACE, "RLOC was added");
    }
    if (flags & OT_CHANGED_THREAD_RLOC_REMOVED) {
        LOG(TRACE, "RLOC was removed");
    }
    if (flags & OT_CHANGED_THREAD_PARTITION_ID) {
        LOG(TRACE, "Partition ID changed");
    }
    if (flags & OT_CHANGED_THREAD_KEY_SEQUENCE_COUNTER) {
        LOG(TRACE, "Thread key sequence changed");
    }
    if (flags & OT_CHANGED_THREAD_NETDATA) {
        LOG(TRACE, "Thread network data changed");
    }
    if (flags & OT_CHANGED_THREAD_CHILD_ADDED) {
        LOG(TRACE, "Child was added");
    }
    if (flags & OT_CHANGED_THREAD_CHILD_REMOVED) {
        LOG(TRACE, "Child was removed");
    }
    if (flags & OT_CHANGED_IP6_MULTICAST_SUBSRCRIBED) {
        LOG(TRACE, "Subscribed to IPv6 multicast address");
    }
    if (flags & OT_CHANGED_IP6_MULTICAST_UNSUBSRCRIBED) {
        LOG(TRACE, "Unsubscribed from IPv6 multicast address");
    }
    if (flags & OT_CHANGED_COMMISSIONER_STATE) {
        const otCommissionerState state = otCommissionerGetState(thread);
        LOG(INFO, "Commissioner state changed: %s", commissionerStateStr(state));
    }
    if (flags & OT_CHANGED_JOINER_STATE) {
        const otJoinerState state = otJoinerGetState(thread);
        LOG(INFO, "Joiner state changed: %s", joinerStateStr(state));
    }
    if (flags & OT_CHANGED_THREAD_CHANNEL) {
        LOG(TRACE, "Thread network channel changed");
    }
    if (flags & OT_CHANGED_THREAD_PANID) {
        LOG(TRACE, "Thread network PAN ID changed");
    }
    if (flags & OT_CHANGED_THREAD_NETWORK_NAME) {
        LOG(TRACE, "Thread network name changed");
    }
    if (flags & OT_CHANGED_THREAD_EXT_PANID) {
        LOG(TRACE, "Thread network extended PAN ID changed");
    }
    if (flags & OT_CHANGED_MASTER_KEY) {
        LOG(TRACE, "Master key changed");
    }
    if (flags & OT_CHANGED_PSKC) {
        LOG(TRACE, "PSKc changed");
    }
    if (flags & OT_CHANGED_SECURITY_POLICY) {
        LOG(TRACE, "Security policy changed");
    }
}

/**
 * Thread persistent storage key index.
 */
const uint16_t kKeyNetworkId = 0x4000;

} // particle::system::

int threadInit() {
    return ot_init([](otInstance* thread) -> int {
        CHECK_THREAD(otSetStateChangedCallback(thread, threadStateChanged, thread));

        otLinkModeConfig mode = {};
        mode.mRxOnWhenIdle = true;
        mode.mSecureDataRequests = true;
        mode.mDeviceType = true;
        mode.mNetworkData = true;
        CHECK_THREAD(otThreadSetLinkMode(thread, mode));

        if (otDatasetIsCommissioned(thread)) {
            LOG(INFO, "Network name: %s", otThreadGetNetworkName(thread));
            LOG(INFO, "802.15.4 channel: %d", (int)otLinkGetChannel(thread));
            LOG(INFO, "802.15.4 PAN ID: 0x%04x", (unsigned)otLinkGetPanId(thread));
        }

        return 0;
    }, nullptr);
}

int threadGetNetworkId(otInstance* ot, char* buf, uint16_t* buflen) {
    buf[0] = 0;
    auto result = otPlatSettingsGet(ot, kKeyNetworkId, 0, (uint8_t*)buf, buflen);
    if (result == OT_ERROR_NOT_FOUND) {
        result = OT_ERROR_NONE;
    }
    return threadToSystemError(result);
}

int threadToSystemError(otError error) {
    switch (error) {
    case OT_ERROR_NONE:
        return SYSTEM_ERROR_NONE;
    case OT_ERROR_SECURITY:
        return SYSTEM_ERROR_NOT_ALLOWED;
    case OT_ERROR_NOT_FOUND:
        return SYSTEM_ERROR_NOT_FOUND;
    case OT_ERROR_RESPONSE_TIMEOUT:
        return SYSTEM_ERROR_TIMEOUT;
    case OT_ERROR_NO_BUFS:
        return SYSTEM_ERROR_NO_MEMORY;
    case OT_ERROR_BUSY:
        return SYSTEM_ERROR_BUSY;
    case OT_ERROR_ABORT:
        return SYSTEM_ERROR_ABORTED;
    case OT_ERROR_INVALID_STATE:
        return SYSTEM_ERROR_INVALID_STATE;
    default:
        return SYSTEM_ERROR_UNKNOWN;
    }
}

int threadSetNetworkId(otInstance* ot, const char* buf) {
    return threadToSystemError(otPlatSettingsSet(ot, kKeyNetworkId, (const uint8_t*)buf, strlen(buf) + 1));
}

} // particle::system

} // particle

#endif /* HAL_PLATFORM_OPENTHREAD */
