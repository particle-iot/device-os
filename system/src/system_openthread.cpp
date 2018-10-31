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

#include "spark_protocol_functions.h"

#include "service_debug.h"
#include "logging.h"
#include "check.h"

#include <openthread-core-config.h>

#include <openthread/openthread.h>
#include <openthread/thread.h>
#include <openthread/thread_ftd.h>
#include <openthread/instance.h>
#include <openthread/commissioner.h>
#include <openthread/joiner.h>
#include <openthread/platform.h>
#include <openthread/platform/settings.h>
#include <mutex>

#define CHECK_THREAD(_expr) \
        do { \
            const otError ret = _expr; \
            if (ret != OT_ERROR_NONE) { \
                LOG_DEBUG(ERROR, #_expr " failed: %d", (int)ret); \
                return ::particle::system::threadToSystemError(ret); \
            } \
        } while (false)

LOG_SOURCE_CATEGORY("system.ot");

namespace {

using namespace particle;
using namespace particle::system;

// Persistent storage keys
const uint16_t NETWORK_ID_KEY = 0x4000;
const uint16_t DEVICE_CONFIG_KEY = 0x4001;

// Version of the device settings format
const uint8_t DEVICE_CONFIG_VERSION = 1;

// Device settings
struct DeviceConfig {
    uint8_t version; // Format version
    uint8_t role; // Device role
    uint8_t type; // Device type
} __attribute__((packed));

int setDeviceMode(otInstance* thread, int role, int type) {
    otLinkModeConfig conf = {};
    if (role != MESH_DEVICE_ROLE_ENDPOINT || type != MESH_DEVICE_TYPE_SLEEPY) {
        conf.mRxOnWhenIdle = true;
    }
    if (role != MESH_DEVICE_ROLE_ENDPOINT || type == MESH_DEVICE_TYPE_FULL || type == MESH_DEVICE_TYPE_DEFAULT) {
        conf.mDeviceType = true; // FTD
    }
    conf.mSecureDataRequests = true;
    conf.mNetworkData = true;
    CHECK_THREAD(otThreadSetLinkMode(thread, conf));
    const bool routerEnabled = (role != MESH_DEVICE_ROLE_ENDPOINT);
    otThreadSetRouterRoleEnabled(thread, routerEnabled);
    // TODO: Enable/disable border router
    return 0;
}

int loadDeviceConfig(otInstance* thread, DeviceConfig* conf) {
    uint16_t size = sizeof(DeviceConfig);
    CHECK_THREAD(otPlatSettingsGet(thread, DEVICE_CONFIG_KEY, 0, (uint8_t*)conf, &size));
    CHECK_TRUE(size == sizeof(DeviceConfig), SYSTEM_ERROR_BAD_DATA);
    return 0;
}

int saveDeviceConfig(otInstance* thread, const DeviceConfig& conf) {
    CHECK_THREAD(otPlatSettingsSet(thread, DEVICE_CONFIG_KEY, (const uint8_t*)&conf, sizeof(DeviceConfig)));
    return 0;
}

int threadToSystemRole(otDeviceRole role) {
    switch (role) {
    case OT_DEVICE_ROLE_DETACHED:
        return MESH_DEVICE_ROLE_DETACHED;
    case OT_DEVICE_ROLE_CHILD:
        return MESH_DEVICE_ROLE_ENDPOINT;
    case OT_DEVICE_ROLE_ROUTER:
    case OT_DEVICE_ROLE_LEADER:
        return MESH_DEVICE_ROLE_REPEATER;
    default:
        return MESH_DEVICE_ROLE_DISABLED;
    }
}

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

static_assert(MESH_MAX_NETWORK_NAME_SIZE == OT_NETWORK_NAME_MAX_SIZE,
        "Invalid maximum size of a network name");
static_assert(MESH_EXT_PAN_ID_SIZE == OT_EXT_PAN_ID_SIZE,
        "Invalid size of an extended PAN ID");
static_assert(MESH_NETWORK_ID_SIZE == MeshCommand::MESH_NETWORK_ID_LENGTH,
        "Invalid size of a network ID");

} // unnamed

namespace particle {

namespace system {

int threadInit() {
    return ot_init([](otInstance* thread) -> int {
        int role = MESH_DEVICE_ROLE_DEFAULT;
        int type = MESH_DEVICE_TYPE_DEFAULT;
        DeviceConfig conf = {};
        const int r = loadDeviceConfig(thread, &conf);
        if (r == 0) {
            LOG(INFO, "Loaded mesh device settings: role: %d, type: %d", role, type);
            role = conf.role;
            type = conf.type;
        }

        CHECK_THREAD(otSetStateChangedCallback(thread, threadStateChanged, thread));
        CHECK(setDeviceMode(thread, role, type));

        if (otDatasetIsCommissioned(thread)) {
            LOG(INFO, "Network name: %s", otThreadGetNetworkName(thread));
            LOG(INFO, "802.15.4 channel: %d", (int)otLinkGetChannel(thread));
            LOG(INFO, "802.15.4 PAN ID: 0x%04x", (unsigned)otLinkGetPanId(thread));
        }

        return 0;
    }, nullptr);
}

int threadGetNetworkId(otInstance* ot, char* buf, uint16_t* buflen) {
    std::lock_guard<ThreadLock> lk(ThreadLock());
    buf[0] = 0;
    auto result = otPlatSettingsGet(ot, NETWORK_ID_KEY, 0, (uint8_t*)buf, buflen);
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
    std::lock_guard<ThreadLock> lk(ThreadLock());
    return threadToSystemError(otPlatSettingsSet(ot, NETWORK_ID_KEY, (const uint8_t*)buf, strlen(buf) + 1));
}

} // particle::system

} // particle

// System API
int mesh_set_device_mode(int role, int type, unsigned flags, void* reserved) {
    // TODO: Make it possible to disable Thread persistently
    if (role == MESH_DEVICE_ROLE_DISABLED || role == MESH_DEVICE_ROLE_DETACHED) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    const std::lock_guard<ThreadLock> lock(ThreadLock());
    const auto thread = threadInstance();
    CHECK_TRUE(thread, SYSTEM_ERROR_INVALID_STATE);
    CHECK(setDeviceMode(thread, role, type));
    DeviceConfig conf = {};
    conf.version = DEVICE_CONFIG_VERSION;
    conf.role = role;
    conf.type = type;
    CHECK(saveDeviceConfig(thread, conf));
    return 0;
}

int mesh_get_device_mode(int* current_role, int* configured_role, int* type, void* reserved) {
    const std::lock_guard<ThreadLock> lock(ThreadLock());
    const auto thread = threadInstance();
    CHECK_TRUE(thread, SYSTEM_ERROR_INVALID_STATE);
    if (current_role) {
        // TODO: Gateway role
        const auto role = otThreadGetDeviceRole(thread);
        *current_role = threadToSystemRole(role);
    }
    if (configured_role || type) {
        DeviceConfig conf = {};
        CHECK(loadDeviceConfig(thread, &conf));
        if (configured_role) {
            *configured_role = conf.role;
        }
        if (type) {
            *type = conf.type;
        }
    }
    return 0;
}

int mesh_get_network_info(mesh_network_info* info, void* reserved) {
    const std::lock_guard<ThreadLock> lock(ThreadLock());
    const auto thread = threadInstance();
    CHECK_TRUE(thread, SYSTEM_ERROR_INVALID_STATE);
    CHECK_TRUE(otDatasetIsCommissioned(thread), SYSTEM_ERROR_NOT_FOUND);
    // Network ID
    uint16_t size = sizeof(info->id);
    CHECK(threadGetNetworkId(thread, info->id, &size));
    // Network name
    const auto name = otThreadGetNetworkName(thread);
    CHECK_TRUE(name, SYSTEM_ERROR_UNKNOWN);
    strlcpy(info->name, name, sizeof(info->name));
    // Channel
    info->channel = otLinkGetChannel(thread);
    // PAN ID
    info->pan_id = otLinkGetPanId(thread);
    // Extended PAN ID
    const auto extPanId = otThreadGetExtendedPanId(thread);
    memcpy(info->ext_pan_id, extPanId, OT_EXT_PAN_ID_SIZE);
    return 0;
}

#endif /* HAL_PLATFORM_OPENTHREAD */
