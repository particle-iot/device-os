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
LOG_SOURCE_CATEGORY("system.ctrl.mesh");

#include "mesh.h"

#if SYSTEM_CONTROL_ENABLED && HAL_PLATFORM_MESH

#include "system_openthread.h"
#include "system_network_manager.h"
#include "system_cloud.h"
#include "system_threading.h"

#include "common.h"

#include "concurrent_hal.h"
#include "timer_hal.h"
#include "delay_hal.h"
#include "core_hal.h"

#include "bytes2hexbuf.h"
#include "hex_to_bytes.h"
#include "random.h"
#include "preprocessor.h"
#include "logging.h"
#include "check.h"
#include "scope_guard.h"
#include <arpa/inet.h>
#include "inet_hal_posix.h"

#include "spark_wiring_vector.h"

#include "openthread/thread.h"
#include "openthread/thread_ftd.h"
#include "openthread/instance.h"
#include "openthread/commissioner.h"
#include "openthread/joiner.h"
#include "openthread/ip6.h"
#include "openthread/link.h"
#include "openthread/dataset.h"
#include "openthread/dataset_ftd.h"
#include "openthread/platform/settings.h"
#include "openthread/netdata.h"
#include "thread/network_diagnostic_tlvs.hpp"
#include "thread/network_data_tlvs.hpp"
#include "thread/mle_constants.hpp"

#include "deviceid_hal.h"
#include "openthread/particle_extension.h"

#include "proto/mesh.pb.h"

#include "system_commands.h"
#include <mutex>
#include <cstdlib>

// FIXME: OT_RADIO_2P4GHZ_OQPSK_CHANNEL_MASK should be used instead
// once https://github.com/openthread/openthread/pull/3381 is merged
#ifndef OT_CHANNEL_ALL
#define OT_CHANNEL_ALL (0xffffffff)
#endif // OT_CHANNEL_ALL

#define THREAD_LOCK(_name) \
        ThreadLock _name;

// TODO: Make mesh-related request handlers asynchronous
#define WAIT_UNTIL(_lock, _expr) \
        for (const auto _t1 = HAL_Timer_Get_Milli_Seconds();;) { \
            if (_expr) { \
                break; \
            } \
            _lock.unlock(); \
            const auto _t2 = HAL_Timer_Get_Milli_Seconds(); \
            if (_t2 - _t1 >= WAIT_UNTIL_TIMEOUT) { \
                LOG(ERROR, "WAIT_UNTIL() timeout"); \
                HAL_Core_System_Reset_Ex(RESET_REASON_WATCHDOG, 0, nullptr); \
            } \
            HAL_Delay_Milliseconds(500); \
            _lock.lock(); \
        }

// FIXME: provide a single WAIT_UNTIL with variable number of arguments
#define WAIT_UNTIL_EX(_lock, _pollPeriod, _maxWait, _expr) \
        for (const auto _t1 = HAL_Timer_Get_Milli_Seconds();;) { \
            if (_expr) { \
                break; \
            } \
            const auto _t2 = HAL_Timer_Get_Milli_Seconds(); \
            if (_t2 - _t1 >= _maxWait) { \
                break; \
            } \
            _lock.unlock(); \
            HAL_Delay_Milliseconds(_pollPeriod); \
            _lock.lock(); \
        }

#define PB(_name) particle_ctrl_mesh_##_name

using spark::Vector;

namespace particle {

using namespace control::common;
using namespace system;

using namespace MeshCommand;

namespace ctrl {

namespace mesh {

namespace {

// Default IEEE 802.15.4 channel
const unsigned DEFAULT_CHANNEL = 11;

// Time is milliseconds after which the commissioner role is automatically stopped
const unsigned DEFAULT_COMMISSIONER_TIMEOUT = 600000;

// Time in seconds after which a joiner entry is automatically removed from the commissioner dataset
const unsigned DEFAULT_JOINER_ENTRY_TIMEOUT = 600;

// Maximum time in milliseconds to spend trying to join a network
const unsigned DEFAULT_JOINER_TIMEOUT = 120000;

// Maximum time in milliseconds to spend trying to discover a network
const unsigned DISCOVERY_TIMEOUT = 45000;

// Minimum size of the joining device credential
const size_t JOINER_PASSWORD_MIN_SIZE = 6;

// Maximum size of the joining device credential
const size_t JOINER_PASSWORD_MAX_SIZE = 32;

// Time in milliseconds to spend scanning each channel during an active scan
const unsigned DEFAULT_ACTIVE_SCAN_DURATION = 0; // Use OpenThread's default timeout

// Time in milliseconds to spend scanning each channel during an energy scan
const unsigned ENERGY_SCAN_DURATION = 200;

// Time in milliseconds after which WAIT_UNTIL() resets the device
const unsigned WAIT_UNTIL_TIMEOUT = 600000;

// Time in milliseconds to wait for diagnostic reply from the nodes
const unsigned DIAGNOSTIC_REPLY_TIMEOUT = 10000;

// Vendor data
const char* const VENDOR_NAME = "Particle";
const char* const VENDOR_MODEL = PP_STR(PLATFORM_NAME);
const char* const VENDOR_SW_VERSION = PP_STR(SYSTEM_VERSION_STRING);
const char* const VENDOR_DATA = "";

// Current joining device credential
char g_joinPwd[JOINER_PASSWORD_MAX_SIZE + 1] = {}; // +1 character for term. null

// Joiner's network ID
char g_joinNetworkId[MESH_NETWORK_ID_LENGTH + 1] = {}; // +1 character for term. null

// Commissioner role timer
os_timer_t g_commTimer = nullptr;

// Commissioner role timeout
unsigned g_commTimeout = DEFAULT_COMMISSIONER_TIMEOUT;

class Random: public particle::Random {
public:
    void genBase32Thread(char* data, size_t size) {
        // base32-thread isn't really defined anywhere, but otbr-commissioner explicitly forbids using
        // I, O, Q and Z in the joiner passphrase
        static const char alpha[32] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K', 'L', 'M', 'N', 'P', 'R', 'S',
                'T', 'U', 'V', 'W', 'X', 'Y', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
        genAlpha(data, size, alpha, sizeof(alpha));
    }
};

void setIp6RlocAddress(otIp6Address* addr, const otMeshLocalPrefix* prefix, uint16_t rloc) {
    memcpy(addr->mFields.m8, prefix->m8, OT_MESH_LOCAL_PREFIX_SIZE);
    // Generate RLOC IPv6 address. Is there a better way to do this?
    addr->mFields.m16[(sizeof(addr->mFields.m16) / sizeof(addr->mFields.m16[0])) - 4] = 0x0000;
    addr->mFields.m16[(sizeof(addr->mFields.m16) / sizeof(addr->mFields.m16[0])) - 3] = htons(0x00ff);
    addr->mFields.m16[(sizeof(addr->mFields.m16) / sizeof(addr->mFields.m16[0])) - 2] = htons(0xfe00);
    addr->mFields.m16[(sizeof(addr->mFields.m16) / sizeof(addr->mFields.m16[0])) - 1] = htons(rloc);
}

/**
 * Fetch the corresponding network attributes given by the flags into a network update command
 * and apply the update.
 */
int notifyNetworkUpdated(int flags) {
    NotifyMeshNetworkUpdated cmd;
    NetworkInfo& ni = cmd.ni;
    // todo - consolidate with getNetworkInfo - decouple fetching the network info from the
    // control request decoding/result encoding.
    THREAD_LOCK(lock);
    const auto thread = threadInstance();
    if (!thread) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Network ID
    size_t netIdSize = sizeof(ni.update.id);
    CHECK(ot_get_network_id(thread, ni.update.id, &netIdSize));
    if (flags & NetworkInfo::NETWORK_ID_VALID) {
        memcpy(ni.id, ni.update.id, sizeof(ni.update.id));
    }
    // Network name
    if (flags & NetworkInfo::NAME_VALID) {
        const char* name = otThreadGetNetworkName(thread);
        if (!name) {
            LOG(ERROR, "Unable to retrieve thread network name");
            return SYSTEM_ERROR_UNKNOWN;
        }
        size_t length = strlen(name);
        strncpy(ni.name, name, MAX_NETWORK_NAME_LENGTH);
        ni.name_length = length;
    }
    // Channel
    if (flags & NetworkInfo::CHANNEL_VALID) {
        ni.channel = otLinkGetChannel(thread);
    }
    // PAN ID
    if (flags & NetworkInfo::PANID_VALID) {
        const otPanId panId = otLinkGetPanId(thread);
        ni.panid[0] = panId >> 8;
        ni.panid[1] = panId & 0xff;
    }
    // Extended PAN ID
    if (flags & NetworkInfo::XPANID_VALID) {
        const auto extPanId = otThreadGetExtendedPanId(thread);
        if (!extPanId) {
            LOG(ERROR, "Unable to retrieve thread XPAN ID");
            return SYSTEM_ERROR_UNKNOWN;
        }
        memcpy(ni.xpanid, extPanId->m8, sizeof(ni.xpanid));
    }
    // Mesh-local prefix
    if (flags & NetworkInfo::ON_MESH_PREFIX_VALID) {
        const auto prefix = otThreadGetMeshLocalPrefix(thread);
        if (!prefix) {
            LOG(ERROR, "Unable to retrieve thread network local prefix");
            return SYSTEM_ERROR_UNKNOWN;
        }
        memcpy(ni.on_mesh_prefix, prefix->m8, 8);
    }
    ni.update.size = sizeof(ni);
    ni.flags = flags;
    int result = system_command_enqueue(cmd, sizeof(cmd));
    if (result) {
        LOG(ERROR, "Unable to add notification to system command queue %d", result);
    }
    return result;
}

int notifyJoined(bool joined) {
    THREAD_LOCK(lock);
    const auto thread = threadInstance();
    CHECK_TRUE(thread, SYSTEM_ERROR_INVALID_STATE);
    NotifyMeshNetworkJoined cmd;
    size_t netIdSize = sizeof(cmd.nu.id);
    CHECK(ot_get_network_id(thread, cmd.nu.id, &netIdSize));
    cmd.joined = joined;
    return system_command_enqueue(cmd, sizeof(cmd));
}

int resetThread() {
    THREAD_LOCK(lock);
    const auto thread = threadInstance();
    if (!thread) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // TODO: Disable only the Thread interface
    NetworkManager::instance()->deactivateConnections();
    // Enqueue a network change event
    system_command_clear();
    if (otDatasetIsCommissioned(thread)) {
        notifyJoined(false);
    }
    // Disable Thread
    CHECK_THREAD(otThreadSetEnabled(thread, false));
    CHECK_THREAD(otIp6SetEnabled(thread, false));
    // Clear master key (invalidates active and pending datasets)
    otMasterKey key = {};
    CHECK_THREAD(otThreadSetMasterKey(thread, &key));
    // Erase persistent data
    CHECK_THREAD(otInstanceErasePersistentInfo(thread));
    return 0;
}

void commissionerTimeout(os_timer_t timer);

void stopCommissionerTimer() {
    THREAD_LOCK(lock);
    if (g_commTimer) {
        os_timer_destroy(g_commTimer, nullptr);
        g_commTimer = nullptr;
        g_commTimeout = DEFAULT_COMMISSIONER_TIMEOUT;
        LOG_DEBUG(TRACE, "Commissioner timer stopped");
    }
}

int startCommissionerTimer() {
    THREAD_LOCK(lock);
    os_timer_change_t change = OS_TIMER_CHANGE_PERIOD;
    if (!g_commTimer) {
        const int ret = os_timer_create(&g_commTimer, g_commTimeout, commissionerTimeout, nullptr, true, nullptr);
        if (ret != 0) {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        change = OS_TIMER_CHANGE_START;
    }
    const int ret = os_timer_change(g_commTimer, change, false, g_commTimeout, 0xffffffff, nullptr);
    if (ret != 0) {
        stopCommissionerTimer();
        return SYSTEM_ERROR_UNKNOWN;
    }
    LOG_DEBUG(TRACE, "Commissioner timer %s; timeout: %u", (change == OS_TIMER_CHANGE_START) ? "started" : "restarted",
            g_commTimeout);
    return 0;
}

void commissionerTimeout(os_timer_t timer) {
    LOG_DEBUG(TRACE, "Commissioner timeout");
    stopCommissionerTimer();
    // Stopping the commissioner in the timer thread may cause a stack overflow
    const auto task = new(std::nothrow) ISRTaskQueue::Task;
    if (!task) {
        return;
    }
    task->func = [](ISRTaskQueue::Task* task) {
        delete task;
        THREAD_LOCK(lock);
        const auto thread = threadInstance();
        if (!thread) {
            return;
        }
        const auto state = otCommissionerGetState(thread);
        if (state != OT_COMMISSIONER_STATE_DISABLED) {
            LOG_DEBUG(TRACE, "Stopping commissioner");
            otCommissionerStop(thread);
        }
    };
    SystemISRTaskQueue.enqueue(task);
}

} // particle::ctrl::mesh::

int notifyBorderRouter(bool active) {
    THREAD_LOCK(lock);
    const auto thread = threadInstance();
    CHECK_TRUE(thread, SYSTEM_ERROR_INVALID_STATE);
    NotifyMeshNetworkGateway cmd;
    size_t netIdSize = sizeof(cmd.nu.id);
    CHECK(ot_get_network_id(thread, cmd.nu.id, &netIdSize));
    cmd.active = active;
    return system_command_enqueue(cmd, sizeof(cmd));
}

int auth(ctrl_request* req) {
    THREAD_LOCK(lock);
    const auto thread = threadInstance();
    if (!thread) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Parse request
    PB(AuthRequest) pbReq = {};
    DecodedCString dPwd(&pbReq.password); // Commissioning credential
    int ret = decodeRequestMessage(req, PB(AuthRequest_fields), &pbReq);
    if (ret != 0) {
        return ret;
    }
    // Get network name, extended PAN ID and current PSKc
    const char* name = otThreadGetNetworkName(thread);
    const auto extPanId = otThreadGetExtendedPanId(thread);
    const uint8_t* curPskc = otThreadGetPSKc(thread);
    if (!name || !extPanId || !curPskc) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Generate PSKc for the provided commissioning credential
    uint8_t pskc[OT_PSKC_MAX_SIZE] = {};
    CHECK_THREAD(otCommissionerGeneratePSKc(thread, dPwd.data, name, extPanId, pskc));
    if (memcmp(pskc, curPskc, OT_PSKC_MAX_SIZE) != 0) {
        return SYSTEM_ERROR_NOT_ALLOWED;
    }
    return 0;
}

int createNetwork(ctrl_request* req) {
    THREAD_LOCK(lock);
    const auto thread = threadInstance();
    if (!thread) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Parse request
    PB(CreateNetworkRequest) pbReq = {};
    DecodedCString dName(&pbReq.name); // Network name
    DecodedCString dPwd(&pbReq.password); // Commissioning credential
    DecodedCString dId(&pbReq.network_id); // network id credential
    int ret = decodeRequestMessage(req, PB(CreateNetworkRequest_fields), &pbReq);
    if (ret != 0) {
        return ret;
    }
    // Network name: up to 16 characters, UTF-8 encoded
    // Commissioning credential: 6 to 255 characters, UTF-8 encoded
    if (dName.size == 0 || dName.size > OT_NETWORK_NAME_MAX_SIZE || dPwd.size < OT_COMMISSIONING_PASSPHRASE_MIN_SIZE ||
            dPwd.size > OT_COMMISSIONING_PASSPHRASE_MAX_SIZE || dId.size != MESH_NETWORK_ID_LENGTH) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    // Disable Thread and clear network credentials
    CHECK(resetThread());
    unsigned channel = pbReq.channel; // IEEE 802.15.4 channel
    if (channel == 0) {
        // Perform an energy scan
        struct EnergyScanResult {
            unsigned channel;
            int minRssi;
            volatile bool done;
        };
        EnergyScanResult enScan = {};
        // For now, excluding channels 25 and 26 which are not allowed for use in most countries which
        // have radio frequency regulations
        const unsigned channelMask = OT_CHANNEL_ALL & ~OT_CHANNEL_25_MASK & ~OT_CHANNEL_26_MASK;
        LOG_DEBUG(TRACE, "Performing energy scan");
        CHECK_THREAD(otLinkEnergyScan(thread, channelMask, ENERGY_SCAN_DURATION,
                [](otEnergyScanResult* result, void* data) {
            const auto scan = (EnergyScanResult*)data;
            if (!result) {
                LOG_DEBUG(TRACE, "Energy scan done");
                scan->done = true;
                return;
            }
            LOG_DEBUG(TRACE, "Channel: %u; RSSI: %d", (unsigned)result->mChannel, (int)result->mMaxRssi);
            if (result->mMaxRssi < scan->minRssi) {
                scan->minRssi = result->mMaxRssi;
                scan->channel = result->mChannel;
            }
        }, &enScan));
        WAIT_UNTIL(lock, enScan.done);
        channel = (enScan.channel != 0) ? enScan.channel : DEFAULT_CHANNEL; // Just in case
    }
    LOG(TRACE, "Using channel %u", channel);
    CHECK_THREAD(otLinkSetChannel(thread, channel));
    // Perform an IEEE 802.15.4 active scan
    struct ActiveScanResult {
        Vector<uint64_t> extPanIds;
        Vector<uint16_t> panIds;
        int result;
        volatile bool done;
    };
    ActiveScanResult actScan = {};
    if (!actScan.extPanIds.reserve(4) || !actScan.panIds.reserve(4)) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    LOG_DEBUG(TRACE, "Performing active scan");
    CHECK_THREAD(otLinkActiveScan(thread, (uint32_t)1 << channel, DEFAULT_ACTIVE_SCAN_DURATION,
            [](otActiveScanResult* result, void* data) {
        const auto scan = (ActiveScanResult*)data;
        if (!result) {
            LOG_DEBUG(TRACE, "Active scan done");
            scan->done = true;
            return;
        }
        if (scan->result != 0) {
            return;
        }
        uint64_t extPanId = 0;
        static_assert(sizeof(result->mExtendedPanId) == sizeof(extPanId), "");
        memcpy(&extPanId, &result->mExtendedPanId, sizeof(extPanId));
        if (!scan->extPanIds.contains(extPanId)) {
            if (!scan->extPanIds.append(extPanId)) {
                scan->result = SYSTEM_ERROR_NO_MEMORY;
            }
        }
        const uint16_t panId = result->mPanId;
        if (!scan->panIds.contains(panId)) {
            if (!scan->panIds.append(panId)) {
                scan->result = SYSTEM_ERROR_NO_MEMORY;
            }
        }
#ifdef DEBUG_BUILD
        char extPanIdStr[sizeof(extPanId) * 2 + 1] = {};
        bytes2hexbuf_lower_case((const uint8_t*)&extPanId, sizeof(extPanId), extPanIdStr);
        LOG_DEBUG(TRACE, "Name: %s; PAN ID: 0x%04x; Extended PAN ID: 0x%s", (const char*)&result->mNetworkName,
                (unsigned)panId, extPanIdStr);
#endif
    }, &actScan));
    WAIT_UNTIL(lock, actScan.done);
    if (actScan.result != 0) {
        return actScan.result;
    }
    // Generate PAN ID
    Random rand;
    uint16_t panId = 0;
    do {
        panId = rand.gen<uint16_t>();
    } while (panId == 0xffff || actScan.panIds.contains(panId));
    CHECK_THREAD(otLinkSetPanId(thread, panId));
    // Generate extended PAN ID
    otExtendedPanId extPanId = {};
    do {
        rand.gen((char*)extPanId.m8, OT_EXT_PAN_ID_SIZE);
    } while (actScan.extPanIds.contains(*((uint64_t*)extPanId.m8)));
    static_assert(sizeof(extPanId) == OT_EXT_PAN_ID_SIZE, "");
    CHECK_THREAD(otThreadSetExtendedPanId(thread, &extPanId));
    // Set network name
    CHECK_THREAD(otThreadSetNetworkName(thread, dName.data));
    // Generate mesh-local prefix (see section 3 of RFC 4193)
    otMeshLocalPrefix prefix = {
        .m8 = {
            0xfd, // Prefix, L
            0x00, 0x00, 0x00, 0x00, 0x00, // Global ID
            0x00, 0x00 // Subnet ID
        }
    };
    rand.gen((char*)prefix.m8 + 1, 5); // Generate global ID
    CHECK_THREAD(otThreadSetMeshLocalPrefix(thread, &prefix));
    // Generate master key
    otMasterKey key = {};
    rand.genSecure((char*)&key, sizeof(key));
    CHECK_THREAD(otThreadSetMasterKey(thread, &key));
    // Set PSKc
    uint8_t pskc[OT_PSKC_MAX_SIZE] = {};
    CHECK_THREAD(otCommissionerGeneratePSKc(thread, dPwd.data, dName.data, &extPanId, pskc));
    CHECK_THREAD(otThreadSetPSKc(thread, pskc));
    CHECK(ot_set_network_id(thread, dId.data ? dId.data : "", dId.data ? (strlen(dId.data) + 1) : 1));
    // Enable Thread
    CHECK_THREAD(otIp6SetEnabled(thread, true));
    CHECK_THREAD(otThreadSetEnabled(thread, true));
    WAIT_UNTIL(lock, otThreadGetDeviceRole(thread) != OT_DEVICE_ROLE_DETACHED);
    const int notifyResult = notifyNetworkUpdated(NetworkInfo::NETWORK_CREATED | NetworkInfo::PANID_VALID |
            NetworkInfo::XPANID_VALID | NetworkInfo::CHANNEL_VALID | NetworkInfo::ON_MESH_PREFIX_VALID |
            NetworkInfo::NAME_VALID | NetworkInfo::NETWORK_ID_VALID);
    if (notifyResult < 0) {
        LOG(ERROR, "Unable to notify network change %d", notifyResult);
    }
    // Encode a reply
    char extPanIdStr[OT_EXT_PAN_ID_SIZE * 2] = {};
    bytes2hexbuf_lower_case(extPanId.m8, OT_EXT_PAN_ID_SIZE, extPanIdStr);
    PB(CreateNetworkReply) pbRep = {};
    EncodedString eName(&pbRep.network.name, dName.data, dName.size);
    EncodedString eExtPanId(&pbRep.network.ext_pan_id, extPanIdStr, sizeof(extPanIdStr));
    EncodedString eNetworkId(&pbRep.network.network_id, dId.data, dId.size);
    pbRep.network.pan_id = panId;
    pbRep.network.channel = channel;
    ret = encodeReplyMessage(req, PB(CreateNetworkReply_fields), &pbRep);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int startCommissioner(ctrl_request* req) {
    THREAD_LOCK(lock);
    const auto thread = threadInstance();
    if (!thread) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Parse request
    PB(StartCommissionerRequest) pbReq = {};
    int ret = decodeRequestMessage(req, PB(StartCommissionerRequest_fields), &pbReq);
    if (ret != 0) {
        return ret;
    }
    // Enable Thread
    if (!otDatasetIsCommissioned(thread)) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!otIp6IsEnabled(thread)) {
        CHECK_THREAD(otIp6SetEnabled(thread, true));
    }
    if (otThreadGetDeviceRole(thread) == OT_DEVICE_ROLE_DISABLED) {
        CHECK_THREAD(otThreadSetEnabled(thread, true));
    }
    WAIT_UNTIL(lock, otThreadGetDeviceRole(thread) != OT_DEVICE_ROLE_DETACHED);
    auto state = otCommissionerGetState(thread);
    if (state == OT_COMMISSIONER_STATE_ACTIVE) {
        LOG_DEBUG(TRACE, "Commissioner is already active");
    } else {
        if (state == OT_COMMISSIONER_STATE_DISABLED) {
            LOG_DEBUG(TRACE, "Starting commissioner");
            CHECK_THREAD(otCommissionerStart(thread));
        }
        WAIT_UNTIL(lock, otCommissionerGetState(thread) != OT_COMMISSIONER_STATE_PETITION);
        state = otCommissionerGetState(thread);
        if (state != OT_COMMISSIONER_STATE_ACTIVE) {
            return SYSTEM_ERROR_TIMEOUT;
        }
        LOG_DEBUG(TRACE, "Commissioner started");
    }
    g_commTimeout = DEFAULT_COMMISSIONER_TIMEOUT;
    if (pbReq.timeout > 0) {
        g_commTimeout = pbReq.timeout * 1000;
    }
    startCommissionerTimer();
    return 0;
}

int stopCommissioner(ctrl_request* req) {
    THREAD_LOCK(lock);
    const auto thread = threadInstance();
    if (!thread) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    stopCommissionerTimer();
    const auto state = otCommissionerGetState(thread);
    if (state != OT_COMMISSIONER_STATE_DISABLED) {
        LOG_DEBUG(TRACE, "Stopping commissioner");
        otCommissionerStop(thread);
        WAIT_UNTIL(lock, otCommissionerGetState(thread) == OT_COMMISSIONER_STATE_DISABLED);
        LOG_DEBUG(TRACE, "Commissioner stopped");
    } else {
        LOG_DEBUG(TRACE, "Commissioner is not active");
    }
    return 0;
}

int prepareJoiner(ctrl_request* req) {
    THREAD_LOCK(lock);
    const auto thread = threadInstance();
    if (!thread) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Parse request
    PB(PrepareJoinerRequest) pbReq = {};
    DecodedCString dNetworkId(&pbReq.network.network_id);
    int ret = decodeRequestMessage(req, PB(PrepareJoinerRequest_fields), &pbReq);
    if (ret != 0) {
        return ret;
    }
    if (dNetworkId.size != MESH_NETWORK_ID_LENGTH) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    // Disable Thread and clear network credentials
    CHECK(resetThread());
    // Set PAN ID
    // https://github.com/openthread/openthread/pull/613
    CHECK_THREAD(otLinkSetPanId(thread, pbReq.network.pan_id));
    // Get factory-assigned EUI-64
    otExtAddress eui64 = {}; // OT_EXT_ADDRESS_SIZE
    otLinkGetFactoryAssignedIeeeEui64(thread, &eui64);
    char eui64Str[sizeof(eui64) * 2 + 1] = {}; // +1 character for term. null
    bytes2hexbuf_lower_case((const uint8_t*)&eui64, sizeof(eui64), eui64Str);
    // Generate joining device credential
    Random rand;
    rand.genBase32Thread(g_joinPwd, JOINER_PASSWORD_MAX_SIZE);
    LOG_DEBUG(TRACE, "Joiner initialized: PAN ID: 0x%04x, EUI-64: %s, password: %s", (unsigned)pbReq.network.pan_id,
            eui64Str, g_joinPwd);
    memcpy(g_joinNetworkId, dNetworkId.data, dNetworkId.size);
    g_joinNetworkId[dNetworkId.size] = '\0';
    // Encode a reply
    PB(PrepareJoinerReply) pbRep = {};
    EncodedString eEuiStr(&pbRep.eui64, eui64Str, strlen(eui64Str));
    EncodedString eJoinPwd(&pbRep.password, g_joinPwd, JOINER_PASSWORD_MAX_SIZE);
    ret = encodeReplyMessage(req, PB(PrepareJoinerReply_fields), &pbRep);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int addJoiner(ctrl_request* req) {
    THREAD_LOCK(lock);
    const auto thread = threadInstance();
    if (!thread) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Parse request
    PB(AddJoinerRequest) pbReq = {};
    DecodedCString dEui64Str(&pbReq.eui64);
    DecodedCString dJoinPwd(&pbReq.password);
    int ret = decodeRequestMessage(req, PB(AddJoinerRequest_fields), &pbReq);
    if (ret != 0) {
        return ret;
    }
    if (dEui64Str.size != sizeof(otExtAddress) * 2 || dJoinPwd.size < JOINER_PASSWORD_MIN_SIZE ||
            dJoinPwd.size > JOINER_PASSWORD_MAX_SIZE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    // Add joiner
    otExtAddress eui64 = {};
    hexToBytes(dEui64Str.data, (char*)&eui64, sizeof(otExtAddress));
    unsigned timeout = DEFAULT_JOINER_ENTRY_TIMEOUT;
    if (pbReq.timeout > 0) {
        timeout = pbReq.timeout;
    }
    LOG_DEBUG(TRACE, "Adding joiner: EUI-64: %s, password: %s", dEui64Str.data, dJoinPwd.data);
    CHECK_THREAD(otCommissionerAddJoiner(thread, &eui64, dJoinPwd.data, timeout));
    startCommissionerTimer();
    return 0;
}

int removeJoiner(ctrl_request* req) {
    THREAD_LOCK(lock);
    const auto thread = threadInstance();
    if (!thread) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Parse request
    PB(RemoveJoinerRequest) pbReq = {};
    DecodedCString dEui64Str(&pbReq.eui64);
    int ret = decodeRequestMessage(req, PB(RemoveJoinerRequest_fields), &pbReq);
    if (ret != 0) {
        return ret;
    }
    if (dEui64Str.size != sizeof(otExtAddress) * 2) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    // Remove joiner
    otExtAddress eui64 = {};
    hexToBytes(dEui64Str.data, (char*)&eui64, sizeof(otExtAddress));
    LOG_DEBUG(TRACE, "Removing joiner: EUI-64: %s", dEui64Str.data);
    CHECK_THREAD(otCommissionerRemoveJoiner(thread, &eui64));
    startCommissionerTimer();
    return 0;
}

int joinNetwork(ctrl_request* req) {
    THREAD_LOCK(lock);
    const auto thread = threadInstance();
    if (!thread) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Parse request
    PB(JoinNetworkRequest) pbReq = {};
    int ret = decodeRequestMessage(req, PB(JoinNetworkRequest_fields), &pbReq);
    if (ret != 0) {
        return ret;
    }
    unsigned timeout = DEFAULT_JOINER_TIMEOUT;
    if (pbReq.timeout > 0) {
        timeout = pbReq.timeout * 1000;
    }
    CHECK_THREAD(otIp6SetEnabled(thread, true));
    struct JoinStatus {
        otError result;
        volatile bool done;
    };
    LOG(INFO, "Joining the network; timeout: %u", timeout);
    JoinStatus stat = {};
    bool cancel = false;
    const auto t1 = HAL_Timer_Get_Milli_Seconds();
    for (;;) {
        stat.result = OT_ERROR_FAILED;
        stat.done = false;
        CHECK_THREAD(otJoinerStart(thread, g_joinPwd, nullptr, VENDOR_NAME, VENDOR_MODEL, VENDOR_SW_VERSION,
                VENDOR_DATA, [](otError result, void* data) {
            const auto stat = (JoinStatus*)data;
            if (result == OT_ERROR_NONE) {
                LOG(TRACE, "Joiner succeeded");
            } else {
                LOG(ERROR, "Joiner failed: %u", (unsigned)result);
            }
            stat->result = result;
            stat->done = true;
        }, &stat));
        lock.unlock();
        for (;;) {
            HAL_Delay_Milliseconds(500);
            if (stat.done) {
                break;
            }
            const auto t2 = HAL_Timer_Get_Milli_Seconds();
            if (t2 - t1 >= timeout) {
                LOG(WARN, "Joiner timeout");
                cancel = true;
                break;
            }
        }
        lock.lock();
        if (cancel || stat.result != OT_ERROR_NOT_FOUND) {
            break;
        }
        const auto t2 = HAL_Timer_Get_Milli_Seconds();
        if (t2 - t1 >= DISCOVERY_TIMEOUT) {
            break;
        }
        LOG_DEBUG(TRACE, "Restarting joiner");
        HAL_Delay_Milliseconds(500);
    }
    if (cancel) {
        LOG_DEBUG(TRACE, "Stopping joiner");
        otJoinerStop(thread);
        WAIT_UNTIL(lock, otJoinerGetState(thread) == OT_JOINER_STATE_IDLE);
        LOG_DEBUG(TRACE, "Joiner stopped");
        CHECK(resetThread());
        return SYSTEM_ERROR_TIMEOUT;
    }
    if (stat.result != OT_ERROR_NONE) {
        CHECK(resetThread());
        return ot_system_error(stat.result);
    }
    CHECK(ot_set_network_id(thread, g_joinNetworkId, strlen(g_joinNetworkId) + 1));
    CHECK_THREAD(otThreadSetEnabled(thread, true));
    WAIT_UNTIL(lock, otThreadGetDeviceRole(thread) != OT_DEVICE_ROLE_DETACHED);
    LOG(INFO, "Successfully joined the network");
    notifyJoined(true);
    return 0;
}

int leaveNetwork(ctrl_request* req) {
    // Disable Thread and clear network credentials
    CHECK(resetThread());
    return 0;
}

int getNetworkInfo(ctrl_request* req) {
    THREAD_LOCK(lock);
    const auto thread = threadInstance();
    if (!thread) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!otDatasetIsCommissioned(thread)) {
        return SYSTEM_ERROR_NOT_FOUND;
    }
    // Network name
    const char* name = otThreadGetNetworkName(thread);
    if (!name) {
        return SYSTEM_ERROR_UNKNOWN;
    }
    // Network Id
    char networkId[MESH_NETWORK_ID_LENGTH + 1] = {};
    size_t networkIdSize = sizeof(networkId);
    ot_get_network_id(thread, networkId, &networkIdSize);
    // Channel
    const uint8_t channel = otLinkGetChannel(thread);
    // PAN ID
    const otPanId panId = otLinkGetPanId(thread);
    // Extended PAN ID
    const auto extPanId = otThreadGetExtendedPanId(thread);
    if (!extPanId) {
        return SYSTEM_ERROR_UNKNOWN;
    }
    char extPanIdStr[OT_EXT_PAN_ID_SIZE * 2] = {};
    bytes2hexbuf_lower_case(extPanId->m8, OT_EXT_PAN_ID_SIZE, extPanIdStr);
    // Encode a reply
    PB(GetNetworkInfoReply) pbRep = {};
    EncodedString eName(&pbRep.network.name, name, strlen(name));
    EncodedString eExtPanIdStr(&pbRep.network.ext_pan_id, extPanIdStr, sizeof(extPanIdStr));
    EncodedString eNetworkId(&pbRep.network.network_id, networkId, strlen(networkId));
    pbRep.network.channel = channel;
    pbRep.network.pan_id = panId;
    const int ret = encodeReplyMessage(req, PB(GetNetworkInfoReply_fields), &pbRep);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int scanNetworks(ctrl_request* req) {
    THREAD_LOCK(lock);
    const auto thread = threadInstance();
    if (!thread) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Parse request
    PB(ScanNetworksRequest) pbReq = {};
    CHECK(decodeRequestMessage(req, PB(ScanNetworksRequest_fields), &pbReq));
    struct Network {
        char name[OT_NETWORK_NAME_MAX_SIZE + 1]; // Network name (null-terminated)
        char extPanId[OT_EXT_PAN_ID_SIZE * 2]; // Extended PAN ID in hex
        uint16_t panId; // PAN ID
        uint8_t channel; // Channel number
    };
    struct ScanResult {
        Vector<Network> networks;
        int result;
        volatile bool done;
    };
    ScanResult scan = {};
    const unsigned duration = pbReq.duration ? pbReq.duration : DEFAULT_ACTIVE_SCAN_DURATION;
    CHECK_THREAD(otLinkActiveScan(thread, OT_CHANNEL_ALL, duration,
            [](otActiveScanResult* result, void* data) {
        const auto scan = (ScanResult*)data;
        if (!result) {
            scan->done = true;
            return;
        }
        if (scan->result != 0) {
            return;
        }
        Network network = {};
        // Network name
        static_assert(sizeof(result->mNetworkName) <= sizeof(Network::name), "");
        strncpy(network.name, result->mNetworkName.m8, sizeof(Network::name));
        // Extended PAN ID
        static_assert(sizeof(result->mExtendedPanId) * 2 == sizeof(Network::extPanId), "");
        bytes2hexbuf_lower_case((const uint8_t*)&result->mExtendedPanId, sizeof(result->mExtendedPanId), network.extPanId);
        // PAN ID
        network.panId = result->mPanId;
        // Channel number
        network.channel = result->mChannel;
        // Ignore duplicate entries. Note: For some reason, OT may report the same network multiple
        // times with different channel numbers. As a workaround, we're excluding the channel number
        // from the comparison
        for (const auto& n: scan->networks) {
            if (strcmp(network.name, n.name) == 0 && memcmp(network.extPanId, n.extPanId, sizeof(Network::extPanId)) == 0 &&
                    network.panId == n.panId) {
                return;
            }
        }
        if (!scan->networks.append(std::move(network))) {
            scan->result = SYSTEM_ERROR_NO_MEMORY;
        }
    }, &scan));
    WAIT_UNTIL(lock, scan.done);
    if (scan.result != 0) {
        return scan.result;
    }
    // Encode a reply
    PB(ScanNetworksReply) pbRep = {};
    pbRep.networks.arg = &scan;
    pbRep.networks.funcs.encode = [](pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
        const auto scan = (const ScanResult*)*arg;
        for (int i = 0; i < scan->networks.size(); ++i) {
            const Network& network = scan->networks.at(i);
            PB(NetworkInfo) pbNetwork = {};
            EncodedString eName(&pbNetwork.name, network.name, strlen(network.name));
            EncodedString eExtPanId(&pbNetwork.ext_pan_id, network.extPanId, sizeof(network.extPanId));
            pbNetwork.pan_id = network.panId;
            pbNetwork.channel = network.channel;
            if (!pb_encode_tag_for_field(strm, field)) {
                return false;
            }
            if (!pb_encode_submessage(strm, PB(NetworkInfo_fields), &pbNetwork)) {
                return false;
            }
        }
        return true;
    };
    const int ret = encodeReplyMessage(req, PB(ScanNetworksReply_fields), &pbRep);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

struct DiagnosticResult {
    ctrl_request* req;

    size_t written;

    const PB(GetNetworkDiagnosticsRequest)* pbReq;

    int routersCount;
    volatile int routersReplied;

    int childCount;
    volatile int childReplied;
    Vector<uint16_t> children;

    int error;

    struct RlocDeviceId {
        bool resolved;
        uint16_t rloc;
        uint8_t deviceId[HAL_DEVICE_ID_SIZE];
    };

    Vector<RlocDeviceId> deviceIdMapping;
    volatile int routersResolved;
    volatile int childrenResolved;
};

struct NetworkDataState {
    ot::NetworkData::NetworkDataTlv* begin;
    ot::NetworkData::NetworkDataTlv* end;
};

void findNetworkDataTlv(NetworkDataState* state, uint8_t* data, size_t len, bool stable) {
    using namespace ot::NetworkData;
    NetworkDataTlv* networkData = reinterpret_cast<NetworkDataTlv*>(data);
    auto end = reinterpret_cast<NetworkDataTlv*>(data + len);
    for (auto netData = networkData; netData && netData < end; netData = netData->GetNext()) {
        if (stable == netData->IsStable()) {
            state->begin = netData;
            state->end = end;
            return;
        }
    }
}

bool encodeContext(pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
    using namespace ot::NetworkData;

    auto prefix = static_cast<PrefixTlv*>(*arg);

    for (auto netData = prefix->GetSubTlvs();
            netData && netData < reinterpret_cast<const NetworkDataTlv*>(
                reinterpret_cast<uint8_t*>(prefix->GetSubTlvs()) + prefix->GetSubTlvsLength());
            netData = netData->GetNext()) {
        if (netData->GetType() != NetworkDataTlv::kTypeContext) {
            continue;
        }

        auto context = static_cast<ContextTlv*>(netData);

        PB(DiagnosticInfo_NetworkData_Context) pbContext = {};
        pbContext.cid = context->GetContextId();
        pbContext.context_length = context->GetContextLength();
        pbContext.compress = context->IsCompress();

        if (!pb_encode_tag_for_field(strm, field)) {
            return false;
        }
        if (!pb_encode_submessage(strm, particle_ctrl_mesh_DiagnosticInfo_NetworkData_Context_fields, &pbContext)) {
            return false;
        }
    }

    return true;
}

bool encodeHasRoute(pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
    using namespace ot::NetworkData;

    auto prefix = static_cast<PrefixTlv*>(*arg);

    for (auto netData = prefix->GetSubTlvs();
            netData && netData < reinterpret_cast<const NetworkDataTlv*>(
                reinterpret_cast<uint8_t*>(prefix->GetSubTlvs()) + prefix->GetSubTlvsLength());
            netData = netData->GetNext()) {
        if (netData->GetType() != NetworkDataTlv::kTypeHasRoute) {
            continue;
        }

        auto hasRoute = static_cast<HasRouteTlv*>(netData);

        PB(DiagnosticInfo_NetworkData_HasRoute) pbHasRoute = {};
        pbHasRoute.entries.arg = hasRoute;
        pbHasRoute.entries.funcs.encode = [](pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
            auto hasRoute = static_cast<HasRouteTlv*>(*arg);

            for (int i = 0; i < hasRoute->GetNumEntries(); i++) {
                auto entry = hasRoute->GetEntry(i);
                PB(DiagnosticInfo_NetworkData_HasRoute_HasRouteEntry) pbEntry = {};
                pbEntry.rloc = entry->GetRloc();
                pbEntry.preference = static_cast<PB(DiagnosticInfo_RoutePreference)>(entry->GetPreference());

                if (!pb_encode_tag_for_field(strm, field)) {
                    return false;
                }
                if (!pb_encode_submessage(strm, PB(DiagnosticInfo_NetworkData_HasRoute_HasRouteEntry_fields), &pbEntry)) {
                    return false;
                }
            }
            return true;
        };

        if (!pb_encode_tag_for_field(strm, field)) {
            return false;
        }
        if (!pb_encode_submessage(strm, PB(DiagnosticInfo_NetworkData_HasRoute_fields), &pbHasRoute)) {
            return false;
        }
    }

    return true;
}

bool encodeBorderRouter(pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
    using namespace ot::NetworkData;

    auto prefix = static_cast<PrefixTlv*>(*arg);

    for (auto netData = prefix->GetSubTlvs();
            netData && netData < reinterpret_cast<const NetworkDataTlv*>(
                reinterpret_cast<uint8_t*>(prefix->GetSubTlvs()) + prefix->GetSubTlvsLength());
            netData = netData->GetNext()) {
        if (netData->GetType() != NetworkDataTlv::kTypeBorderRouter) {
            continue;
        }

        auto borderRouter = static_cast<BorderRouterTlv*>(netData);

        PB(DiagnosticInfo_NetworkData_BorderRouter) pbBorderRouter = {};
        pbBorderRouter.entries.arg = borderRouter;
        pbBorderRouter.entries.funcs.encode = [](pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
            auto borderRouter = static_cast<BorderRouterTlv*>(*arg);

            for (int i = 0; i < borderRouter->GetNumEntries(); i++) {
                auto entry = borderRouter->GetEntry(i);
                PB(DiagnosticInfo_NetworkData_BorderRouter_BorderRouterEntry) pbEntry = {};
                pbEntry.rloc = entry->GetRloc();
                pbEntry.preference = static_cast<PB(DiagnosticInfo_RoutePreference)>(entry->GetPreference());

                if (entry->IsPreferred()) {
                    pbEntry.flags |= PB(DiagnosticInfo_NetworkData_BorderRouter_BorderRouterEntry_Flags_PREFERRED);
                }
                if (entry->IsSlaac()) {
                    pbEntry.flags |= PB(DiagnosticInfo_NetworkData_BorderRouter_BorderRouterEntry_Flags_SLAAC);
                }
                if (entry->IsDhcp()) {
                    pbEntry.flags |= PB(DiagnosticInfo_NetworkData_BorderRouter_BorderRouterEntry_Flags_DHCP);
                }
                if (entry->IsConfigure()) {
                    pbEntry.flags |= PB(DiagnosticInfo_NetworkData_BorderRouter_BorderRouterEntry_Flags_CONFIGURE);
                }
                if (entry->IsDefaultRoute()) {
                    pbEntry.flags |= PB(DiagnosticInfo_NetworkData_BorderRouter_BorderRouterEntry_Flags_DEFAULT_ROUTE);
                }
                if (entry->IsOnMesh()) {
                    pbEntry.flags |= PB(DiagnosticInfo_NetworkData_BorderRouter_BorderRouterEntry_Flags_ON_MESH);
                }
                // P_nd_dns is not supported as of now in OpenThread
                // if (entry->IsNdDns()) {
                //     pbEntry.flags |= PB(DiagnosticInfo_NetworkData_BorderRouter_BorderRouterEntry_Flags_ND_DNS);
                // }

                if (!pb_encode_tag_for_field(strm, field)) {
                    return false;
                }
                if (!pb_encode_submessage(strm, PB(DiagnosticInfo_NetworkData_BorderRouter_BorderRouterEntry_fields), &pbEntry)) {
                    return false;
                }
            }
            return true;
        };

        if (!pb_encode_tag_for_field(strm, field)) {
            return false;
        }
        if (!pb_encode_submessage(strm, PB(DiagnosticInfo_NetworkData_BorderRouter_fields), &pbBorderRouter)) {
            return false;
        }
    }

    return true;
}

bool encodeNetworkDataPrefixes(pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
    using namespace ot::NetworkData;
    auto networkDataState = static_cast<NetworkDataState*>(*arg);

    for (auto netData = networkDataState->begin;
            netData && netData < networkDataState->end;
            netData = netData->GetNext()) {
        if (!(netData->GetType() == NetworkDataTlv::kTypePrefix &&
              netData->IsStable() == networkDataState->begin->IsStable())) {
            continue;
        }

        auto prefix = static_cast<PrefixTlv*>(netData);
        if (!prefix->IsValid()) {
            continue;
        }

        PB(DiagnosticInfo_NetworkData_Prefix) pbPrefix = {};
        pbPrefix.domain_id = prefix->GetDomainId();
        pbPrefix.prefix_length = prefix->GetPrefixLength();
        pbPrefix.prefix.size = BitVectorBytes(prefix->GetPrefixLength());
        memcpy(pbPrefix.prefix.bytes, prefix->GetPrefix(), pbPrefix.prefix.size);

        // Process sub-TLVs: Context, Has Route, Border Router
        if (prefix->GetSubTlvsLength() >= sizeof(NetworkDataTlv)) {
            pbPrefix.context.arg = prefix;
            pbPrefix.context.funcs.encode = encodeContext;
            pbPrefix.has_route.entries.arg = prefix;
            pbPrefix.has_route.entries.funcs.encode = encodeHasRoute;
            pbPrefix.border_router.entries.arg = prefix;
            pbPrefix.border_router.entries.funcs.encode = encodeBorderRouter;
        }

        if (!pb_encode_tag_for_field(strm, field)) {
            return false;
        }
        if (!pb_encode_submessage(strm, particle_ctrl_mesh_DiagnosticInfo_NetworkData_Prefix_fields, &pbPrefix)) {
            return false;
        }
    }

    return true;
}

bool encodeServers(pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
    using namespace ot::NetworkData;

    auto service = static_cast<ServiceTlv*>(*arg);

    for (auto netData = service->GetSubTlvs();
            netData && netData < reinterpret_cast<const NetworkDataTlv*>(
                reinterpret_cast<uint8_t*>(service->GetSubTlvs()) + service->GetSubTlvsLength());
            netData = netData->GetNext()) {
        if (netData->GetType() != NetworkDataTlv::kTypeServer) {
            continue;
        }

        auto server = static_cast<ServerTlv*>(netData);

        PB(DiagnosticInfo_NetworkData_Server) pbServer = {};
        pbServer.rloc = server->GetServer16();
        EncodedString eServerData(&pbServer.data, (const char*)server->GetServerData(), server->GetServerDataLength());

        if (!pb_encode_tag_for_field(strm, field)) {
            return false;
        }
        if (!pb_encode_submessage(strm, PB(DiagnosticInfo_NetworkData_Server_fields), &pbServer)) {
            return false;
        }
    }

    return true;
}

bool encodeNetworkDataServices(pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
    using namespace ot::NetworkData;
    auto networkDataState = static_cast<NetworkDataState*>(*arg);

    for (auto netData = networkDataState->begin;
            netData && netData < networkDataState->end;
            netData = netData->GetNext()) {
        if (!(netData->GetType() == NetworkDataTlv::kTypeService &&
              netData->IsStable() == networkDataState->begin->IsStable())) {
            continue;
        }

        auto service = static_cast<ServiceTlv*>(netData);
        if (!service->IsValid()) {
            continue;
        }

        PB(DiagnosticInfo_NetworkData_Service) pbService = {};
        pbService.sid = service->GetServiceID();
        pbService.enterprise_number = service->GetEnterpriseNumber();
        EncodedString eServiceData(&pbService.data, (const char*)service->GetServiceData(),
                service->GetServiceDataLength());

        // Process sub-TLVs: Server
        if (service->GetSubTlvsLength() >= sizeof(NetworkDataTlv)) {
            pbService.servers.arg = service;
            pbService.servers.funcs.encode = encodeServers;
        }

        if (!pb_encode_tag_for_field(strm, field)) {
            return false;
        }
        if (!pb_encode_submessage(strm, particle_ctrl_mesh_DiagnosticInfo_NetworkData_Service_fields, &pbService)) {
            return false;
        }
    }

    return true;
}

void encodeNetworkData(NetworkDataState* stable, NetworkDataState* temporary, PB(DiagnosticInfo)* pbRep) {
    using namespace ot::NetworkData;

    auto& pbNetData = pbRep->network_data;

    if (stable->begin) {
        pbNetData.stable.prefixes.arg = stable;
        pbNetData.stable.prefixes.funcs.encode = encodeNetworkDataPrefixes;
        pbNetData.stable.services.arg = stable;
        pbNetData.stable.services.funcs.encode = encodeNetworkDataServices;
    }

    if (temporary->begin) {
        pbNetData.temporary.prefixes.arg = temporary;
        pbNetData.temporary.prefixes.funcs.encode = encodeNetworkDataPrefixes;
        pbNetData.temporary.services.arg = temporary;
        pbNetData.temporary.services.funcs.encode = encodeNetworkDataServices;
    }
}

int processNetworkDiagnosticTlvs(DiagnosticResult* diagResult, otMessage* message,
        const otMessageInfo* messageInfo, uint16_t rloc) {
    const int len = otMessageGetLength(message) - otMessageGetOffset(message);
    // Put all the TLVs into a contiguous block
    auto tlvs = std::make_unique<uint8_t[]>(len);
    CHECK_TRUE(tlvs, SYSTEM_ERROR_NO_MEMORY);
    CHECK_TRUE(otMessageRead(message, otMessageGetOffset(message), tlvs.get(), len) == len, SYSTEM_ERROR_BAD_DATA);

    using namespace ot::NetworkDiagnostic;

    PB(DiagnosticInfo) pbRep = {};

    NetworkDataState stableNetData = {};
    NetworkDataState temporaryNetData = {};

    for (const ot::Tlv* tlv = reinterpret_cast<const ot::Tlv*>(tlvs.get());
            reinterpret_cast<const uint8_t*>(tlv) < tlvs.get() + len; tlv = tlv->GetNext()) {

        auto type = tlv->GetType();

        switch (type) {
            case NetworkDiagnosticTlv::kExtMacAddress: {
                auto eMacTlv = static_cast<const ExtMacAddressTlv*>(tlv);
                if (eMacTlv->IsValid()) {
                    auto eMac = eMacTlv->GetMacAddr();
                    memcpy(pbRep.ext_mac_address.bytes, eMac->m8, sizeof(pbRep.ext_mac_address.bytes));
                    pbRep.ext_mac_address.size = sizeof(pbRep.ext_mac_address.bytes);
                }
                break;
            }
            case NetworkDiagnosticTlv::kAddress16: {
                auto a16Tlv = static_cast<const Address16Tlv*>(tlv);
                if (a16Tlv->IsValid()) {
                    pbRep.rloc = a16Tlv->GetRloc16();
                }
                break;
            }
            case NetworkDiagnosticTlv::kMode: {
                auto modeTlv = static_cast<const ModeTlv*>(tlv);
                if (modeTlv->IsValid()) {
                    pbRep.mode = modeTlv->GetMode();
                }
                break;
            }
            case NetworkDiagnosticTlv::kTimeout: {
                auto timeoutTlv = static_cast<const TimeoutTlv*>(tlv);
                if (timeoutTlv->IsValid()) {
                    pbRep.timeout = timeoutTlv->GetTimeout();
                }
                break;
            }
            case NetworkDiagnosticTlv::kConnectivity: {
                auto conTlv = static_cast<const ConnectivityTlv*>(tlv);
                if (conTlv->IsValid()) {
                    auto& con = pbRep.connectivity;
                    con.parent_priority = conTlv->GetParentPriority();
                    con.link_quality_1 = conTlv->GetLinkQuality1();
                    con.link_quality_2 = conTlv->GetLinkQuality2();
                    con.link_quality_3 = conTlv->GetLinkQuality3();
                    con.leader_cost = conTlv->GetLeaderCost();
                    con.id_sequence = conTlv->GetIdSequence();
                    con.active_routers = conTlv->GetActiveRouters();
                    con.sed_buffer_size = conTlv->GetSedBufferSize();
                    con.sed_datagram_count = conTlv->GetSedDatagramCount();
                }
                break;
            }
            case NetworkDiagnosticTlv::kRoute: {
                auto routeTlv = static_cast<const RouteTlv*>(tlv);
                if (routeTlv->IsValid()) {
                    auto& route = pbRep.route64;
                    route.id_sequence = routeTlv->GetRouterIdSequence();
                    route.routes.arg = (void*)routeTlv;
                    route.routes.funcs.encode = [](pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
                        auto routeTlv = reinterpret_cast<const RouteTlv*>(*arg);
                        auto maxRouterId = otThreadGetMaxRouterId(ot_get_instance());
                        for (unsigned i = 0; i < maxRouterId; i++) {
                            if (!routeTlv->IsRouterIdSet(i)) {
                                continue;
                            }
                            PB(DiagnosticInfo_Route64_RouteData) routeData = {};
                            routeData.router_rloc = i << ot::Mle::kRouterIdOffset;
                            routeData.link_quality_out = routeTlv->GetLinkQualityOut(i);
                            routeData.link_quality_in = routeTlv->GetLinkQualityIn(i);
                            routeData.route_cost = routeTlv->GetRouteCost(i);
                            if (!pb_encode_tag_for_field(strm, field)) {
                                return false;
                            }
                            if (!pb_encode_submessage(strm, PB(DiagnosticInfo_Route64_RouteData_fields), &routeData)) {
                                return false;
                            }
                        }
                        return true;
                    };
                }
                break;
            }
            case NetworkDiagnosticTlv::kLeaderData: {
                auto leaderDataTlv = static_cast<const LeaderDataTlv*>(tlv);
                if (leaderDataTlv->IsValid()) {
                    auto& leaderData = pbRep.leader_data;
                    leaderData.partition_id = leaderDataTlv->GetPartitionId();
                    leaderData.weighting = leaderDataTlv->GetWeighting();
                    leaderData.data_version = leaderDataTlv->GetDataVersion();
                    leaderData.stable_data_version = leaderDataTlv->GetStableDataVersion();
                    leaderData.leader_rloc = leaderDataTlv->GetLeaderRouterId() << ot::Mle::kRouterIdOffset;
                }
                break;
            }
            case NetworkDiagnosticTlv::kNetworkData: {
                auto networkDataTlv = (NetworkDataTlv*)(tlv);
                if (networkDataTlv->IsValid()) {
                    findNetworkDataTlv(&stableNetData, networkDataTlv->GetNetworkData(), networkDataTlv->GetLength(), true);
                    findNetworkDataTlv(&temporaryNetData, networkDataTlv->GetNetworkData(), networkDataTlv->GetLength(), false);
                    encodeNetworkData(&stableNetData, &temporaryNetData, &pbRep);
                }
                break;
            }
            case NetworkDiagnosticTlv::kIp6AddressList: {
                auto ip6AddrList = static_cast<const Ip6AddressListTlv*>(tlv);
                if (ip6AddrList->IsValid()) {
                    auto& pbList = pbRep.ipv6_address_list;
                    pbList.arg = (void*)ip6AddrList;
                    pbList.funcs.encode = [](pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
                        auto ip6AddrList = reinterpret_cast<const Ip6AddressListTlv*>(*arg);
                        const unsigned totalAddrs = ip6AddrList->GetLength() / sizeof(otIp6Address);
                        for (unsigned i = 0; i < totalAddrs; i++) {
                            const otIp6Address* addr = &ip6AddrList->GetIp6Address(i);
                            particle_ctrl_Ipv6Address pbAddr = {};
                            memcpy(pbAddr.address, addr->mFields.m8, sizeof(pbAddr.address));
                            if (!pb_encode_tag_for_field(strm, field)) {
                                return false;
                            }
                            if (!pb_encode_submessage(strm, particle_ctrl_Ipv6Address_fields, &pbAddr)) {
                                return false;
                            }
                        }
                        return true;
                    };
                }
                break;
            }
            case NetworkDiagnosticTlv::kMacCounters: {
                auto macCountersTlv = static_cast<const MacCountersTlv*>(tlv);
                if (macCountersTlv->IsValid()) {
                    auto& pbMacCounters = pbRep.mac_counters;
                    pbMacCounters.if_in_unknown_protos = macCountersTlv->GetIfInUnknownProtos();
                    pbMacCounters.if_in_errors = macCountersTlv->GetIfInErrors();
                    pbMacCounters.if_out_errors = macCountersTlv->GetIfOutErrors();
                    pbMacCounters.if_in_ucast_pkts = macCountersTlv->GetIfInUcastPkts();
                    pbMacCounters.if_in_broadcast_pkts = macCountersTlv->GetIfInBroadcastPkts();
                    pbMacCounters.if_in_discards = macCountersTlv->GetIfInDiscards();
                    pbMacCounters.if_out_ucast_pkts = macCountersTlv->GetIfOutUcastPkts();
                    pbMacCounters.if_out_broadcast_pkts = macCountersTlv->GetIfOutBroadcastPkts();
                    pbMacCounters.if_out_discards = macCountersTlv->GetIfOutDiscards();
                }
                break;
            }
            case NetworkDiagnosticTlv::kBatteryLevel: {
                // Not implemented in OpenThread as of now
                break;
            }
            case NetworkDiagnosticTlv::kSupplyVoltage: {
                // Not implemented in OpenThread as of now
                break;
            }
            case NetworkDiagnosticTlv::kChildTable: {
                auto childTable = (ChildTableTlv*)(tlv);
                // This check is broken in OpenThread :|
                // if (childTable->IsValid()) {
                if (childTable->GetLength() >= sizeof(NetworkDiagnosticTlv)) {
                    auto& pbChildren = pbRep.child_table.children;
                    pbChildren.arg = (void*)childTable;
                    pbChildren.funcs.encode = [](pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
                        auto childTable = reinterpret_cast<ChildTableTlv*>(*arg);
                        for (unsigned i = 0; i < childTable->GetNumEntries(); i++) {
                            const auto& entry = childTable->GetEntry(i);
                            PB(DiagnosticInfo_ChildTable_ChildEntry) pbChild = {};
                            pbChild.timeout = entry.GetTimeout();
                            pbChild.child_id = entry.GetChildId();
                            pbChild.mode = entry.GetMode();
                            if (!pb_encode_tag_for_field(strm, field)) {
                                return false;
                            }
                            if (!pb_encode_submessage(strm, PB(DiagnosticInfo_ChildTable_ChildEntry_fields), &pbChild)) {
                                return false;
                            }
                        }
                        return true;
                    };

                    if (diagResult->pbReq->flags & PB(GetNetworkDiagnosticsRequest_Flags_QUERY_CHILDREN)) {
                        // Make child RLOC set
                        for (unsigned i = 0; i < childTable->GetNumEntries(); i++) {
                            const auto& entry = childTable->GetEntry(i);
                            uint16_t childRloc = rloc | entry.GetChildId();
                            if (!diagResult->children.contains(childRloc)) {
                                diagResult->children.append(childRloc);
                            }
                        }
                    }
                }
                break;
            }
            case NetworkDiagnosticTlv::kChannelPages: {
                auto channelPagesTlv = (ChannelPagesTlv*)(tlv);
                if (channelPagesTlv->IsValid()) {
                    // Only 1 channel page supported in OpenThread at the moment
                    pbRep.channel_pages.size = 1;
                    pbRep.channel_pages.bytes[0] = *channelPagesTlv->GetChannelPages();
                }
                break;
            }
            case NetworkDiagnosticTlv::kTypeList: {
                // Should not be returned
                break;
            }
            case NetworkDiagnosticTlv::kMaxChildTimeout: {
                auto timeoutTlv = static_cast<const MaxChildTimeoutTlv*>(tlv);
                if (timeoutTlv->IsValid()) {
                    pbRep.max_child_timeout = timeoutTlv->GetTimeout();
                }
                break;
            }
        }
    }

    if (pbRep.rloc == 0x0000) {
        // Even if we did not request RLOC TLV, we can still deduce it from the source address
        pbRep.rloc = rloc;
    }

    if (diagResult->pbReq->flags & PB(GetNetworkDiagnosticsRequest_Flags_RESOLVE_DEVICE_ID)) {
        for (int i = 0; i < diagResult->deviceIdMapping.size(); ++i) {
            const auto& entry = diagResult->deviceIdMapping.at(i);
            if (entry.resolved && entry.rloc == pbRep.rloc) {
                pbRep.device_id.size = HAL_DEVICE_ID_SIZE;
                memcpy(pbRep.device_id.bytes, entry.deviceId, HAL_DEVICE_ID_SIZE);
                // Remove the entry
                diagResult->deviceIdMapping.removeAt(i);
                break;
            }
        }
    }

    auto r = appendReplySubmessage(diagResult->req, diagResult->written, &PB(GetNetworkDiagnosticsReply_fields)[0],
            PB(DiagnosticInfo_fields), &pbRep);
    if (r > 0) {
        diagResult->written += r;
    } else {
        LOG(ERROR, "appendReplySubmessage %d", r);
        return r;
    }

    return SYSTEM_ERROR_NONE;
}

int getNetworkDiagnostics(ctrl_request* req) {
    // Parse request
    PB(GetNetworkDiagnosticsRequest) pbReq = {};
    // FIXME: repeated enums should probably not be decoded this way
    DecodedString diagTypes(&pbReq.diagnostic_types);
    int ret = decodeRequestMessage(req, PB(GetNetworkDiagnosticsRequest_fields), &pbReq);
    if (ret != 0) {
        LOG(ERROR, "Failed to decode GetNetworkDiagnosticsRequest message %d", ret);
        return ret;
    }

    // At least one diagnostic type should have been requested
    if (diagTypes.size < 1) {
        LOG(ERROR, "At least one diagnostic type should have been specified");
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    bool childTableRequested = false;

    // Validate requested tlvs
    for (unsigned i = 0; i < diagTypes.size; i++) {
        switch(diagTypes.data[i]) {
            case PB(DiagnosticType_MAC_EXTENDED_ADDRESS):
            case PB(DiagnosticType_MAC_ADDRESS):
            case PB(DiagnosticType_MODE):
            case PB(DiagnosticType_TIMEOUT):
            case PB(DiagnosticType_CONNECTIVITY):
            case PB(DiagnosticType_ROUTE64):
            case PB(DiagnosticType_LEADER_DATA):
            case PB(DiagnosticType_NETWORK_DATA):
            case PB(DiagnosticType_IPV6_ADDRESS_LIST):
            case PB(DiagnosticType_MAC_COUNTERS):
            case PB(DiagnosticType_BATTERY_LEVEL):
            case PB(DiagnosticType_SUPPLY_VOLTAGE):
            case PB(DiagnosticType_CHANNEL_PAGES):
            // case PB(DiagnosticType_TYPE_LIST):
            case PB(DiagnosticType_MAX_CHILD_TIMEOUT): {
                break;
            }
            case PB(DiagnosticType_CHILD_TABLE): {
                childTableRequested = true;
                break;
            }
            default: {
                return SYSTEM_ERROR_INVALID_ARGUMENT;
            }
        }
    }

    if ((pbReq.flags & PB(GetNetworkDiagnosticsRequest_Flags_QUERY_CHILDREN)) && !childTableRequested) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    THREAD_LOCK(lock);
    const auto thread = threadInstance();
    if (!thread) {
        return SYSTEM_ERROR_INVALID_STATE;
    }

    // Enable Thread
    if (!otDatasetIsCommissioned(thread)) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (!otIp6IsEnabled(thread)) {
        CHECK_THREAD(otIp6SetEnabled(thread, true));
    }
    if (otThreadGetDeviceRole(thread) == OT_DEVICE_ROLE_DISABLED) {
        CHECK_THREAD(otThreadSetEnabled(thread, true));
    }
    WAIT_UNTIL(lock, otThreadGetDeviceRole(thread) != OT_DEVICE_ROLE_DETACHED);

    // Get list of routers
    Vector<otRouterInfo> routers;
    auto maxRouterId = otThreadGetMaxRouterId(thread);
    for (auto i = 0; i <= maxRouterId; i++) {
        otRouterInfo routerInfo;
        if (otThreadGetRouterInfo(thread, i, &routerInfo) != OT_ERROR_NONE) {
            continue;
        }

        CHECK_TRUE(routers.append(routerInfo), SYSTEM_ERROR_NO_MEMORY);
    }

    auto localPrefix = otThreadGetMeshLocalPrefix(thread);
    CHECK_TRUE(localPrefix, SYSTEM_ERROR_INVALID_STATE);

    DiagnosticResult diagResult = {};
    diagResult.req = req;
    diagResult.pbReq = &pbReq;
    diagResult.routersCount = routers.size();

    const unsigned diagReplyTimeout = pbReq.timeout > 0 ? pbReq.timeout * 1000 : DIAGNOSTIC_REPLY_TIMEOUT;
    const unsigned resolveReplyTimeout = std::max(diagReplyTimeout / 2, DIAGNOSTIC_REPLY_TIMEOUT / 2);

    otThreadSetReceiveDiagnosticGetCallback(thread,
            [](otMessage *message, const otMessageInfo *messageInfo, void *ctx) {
                DiagnosticResult* diagResult = static_cast<DiagnosticResult*>(ctx);
                const auto& m16 = messageInfo->mPeerAddr.mFields.m16;
                auto rloc = ntohs(m16[(sizeof(m16) / sizeof(m16[0])) - 1]);
                LOG_DEBUG(TRACE, "Received diagnostics from %04x, size = %d", rloc, otMessageGetLength(message));

                auto r = processNetworkDiagnosticTlvs(diagResult, message, messageInfo, rloc);

                if (r != SYSTEM_ERROR_NONE) {
                    diagResult->error = r;
                }

                if ((rloc & ot::Mle::kMaxChildId) == 0x0000) {
                    // Router
                    ++diagResult->routersReplied;
                } else {
                    // Child
                    ++diagResult->childReplied;
                }
            }, &diagResult);

    SCOPE_GUARD({
        otThreadSetReceiveDiagnosticGetCallback(thread, nullptr, nullptr);
    });

    otExtParticleSetReceiveDeviceIdGetCallback(thread,
            [](otError err, otCoapCode coapResult, const uint8_t* deviceId, size_t length,
                    const otMessageInfo* msgInfo, void* ctx) {
                DiagnosticResult* diagResult = static_cast<DiagnosticResult*>(ctx);
                const auto& m16 = msgInfo->mPeerAddr.mFields.m16;
                auto rloc = ntohs(m16[(sizeof(m16) / sizeof(m16[0])) - 1]);
                LOG_DEBUG(TRACE, "Received Device Id response from %04x %d %d", rloc, err, coapResult);

                if (err == OT_ERROR_NONE && deviceId && length) {
                    DiagnosticResult::RlocDeviceId mapping = {true, rloc};
                    memcpy(mapping.deviceId, deviceId, sizeof(mapping.deviceId));
                    diagResult->deviceIdMapping.append(mapping);
                }

                if ((rloc & ot::Mle::kMaxChildId) == 0x0000) {
                    // Router
                    ++diagResult->routersResolved;
                } else {
                    // Child
                    ++diagResult->childrenResolved;
                }
    }, &diagResult);

    SCOPE_GUARD({
        otExtParticleSetReceiveDeviceIdGetCallback(thread, nullptr, nullptr);
    });

    if (pbReq.flags & PB(GetNetworkDiagnosticsRequest_Flags_RESOLVE_DEVICE_ID)) {
        for (const auto& router : routers) {
            otIp6Address addr = {};
            setIp6RlocAddress(&addr, localPrefix, router.mRloc16);
            LOG_DEBUG(TRACE, "Sending GET Device Id to router %04x", router.mRloc16);
            CHECK_THREAD(otExtParticleSendDeviceIdGet(thread, &addr));
        }
        // FIXME: it's probably better to use something like a semaphore here
        WAIT_UNTIL_EX(lock, 100, resolveReplyTimeout, (diagResult.routersResolved >= diagResult.routersCount));
    }

    for (const auto& router : routers) {
        otIp6Address addr = {};
        setIp6RlocAddress(&addr, localPrefix, router.mRloc16);
        LOG_DEBUG(TRACE, "Sending DIAG_GET.req to router %04x", router.mRloc16);
        CHECK_THREAD(otThreadSendDiagnosticGet(thread, &addr, (const uint8_t*)diagTypes.data, diagTypes.size));
    }

    routers.clear();

    // FIXME: it's probably better to use something like a semaphore here
    WAIT_UNTIL_EX(lock, 100, diagReplyTimeout, (diagResult.routersReplied >= diagResult.routersCount));

    if (!diagResult.error && diagResult.children.size() > 0) {
        // Query children as well, if requested
        diagResult.childCount = diagResult.children.size();

        if (pbReq.flags & PB(GetNetworkDiagnosticsRequest_Flags_RESOLVE_DEVICE_ID)) {
            for (const auto& child : diagResult.children) {
                otIp6Address addr = {};
                setIp6RlocAddress(&addr, localPrefix, child);
                LOG_DEBUG(TRACE, "Sending GET Device Id to child %04x", child);
                CHECK_THREAD(otExtParticleSendDeviceIdGet(thread, &addr));
            }
            // FIXME: it's probably better to use something like a semaphore here
            WAIT_UNTIL_EX(lock, 100, resolveReplyTimeout, (diagResult.childrenResolved >= diagResult.childCount));
        }

        for (const auto& child : diagResult.children) {
            otIp6Address addr = {};
            setIp6RlocAddress(&addr, localPrefix, child);
            LOG_DEBUG(TRACE, "Sending DIAG_GET.req to child %04x", child);
            CHECK_THREAD(otThreadSendDiagnosticGet(thread, &addr, (const uint8_t*)diagTypes.data, diagTypes.size));
        }
        diagResult.children.clear();
        // FIXME: it's probably better to use something like a semaphore here
        WAIT_UNTIL_EX(lock, 100, diagReplyTimeout, (diagResult.childReplied >= diagResult.childCount));
    }

    if (diagResult.error) {
        // Discard any reply data
        system_ctrl_alloc_reply_data(req, 0, nullptr);
        return diagResult.error;
    }

    // Trim reply size to the actual data size
    return system_ctrl_alloc_reply_data(req, diagResult.written, nullptr);
}

int test(ctrl_request* req) {
    const int ret = system_ctrl_alloc_reply_data(req, req->request_size, nullptr);
    if (ret != 0) {
        return ret;
    }
    memcpy(req->reply_data, req->request_data, req->request_size);
    return 0;
}

} // particle::ctrl::mesh

} // particle::ctrl

} // particle

#endif // SYSTEM_CONTROL_ENABLED && HAL_PLATFORM_MESH
