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

#include "mesh.h"

#if SYSTEM_CONTROL_ENABLED

#include "system_openthread.h"

#include "common.h"

#include "rng_hal.h"

#include "logging.h"
#include "bytes2hexbuf.h"
#include "hex_to_bytes.h"
#include "preprocessor.h"

#include "spark_wiring_vector.h"

#include "openthread/thread.h"
#include "openthread/thread_ftd.h"
#include "openthread/instance.h"
#include "openthread/commissioner.h"
#include "openthread/joiner.h"
#include "openthread/dataset.h"
#include "openthread/dataset_ftd.h"
#include "openthread/ip6.h"

#include "proto/mesh.pb.h"

#include <cstdlib>

#define CHECK_THREAD(_expr) \
    do { \
        const auto ret = _expr; \
        if (ret != OT_ERROR_NONE) { \
            LOG(ERROR, #_expr " failed: %d", (int)ret); \
            return SYSTEM_ERROR_UNKNOWN; \
        } \
    } while (false)

#define PB(_name) particle_ctrl_mesh_##_name

using spark::Vector;

namespace particle {

using namespace control::common;
using namespace system;

namespace ctrl {

namespace mesh {

namespace {

// Default IEEE 802.15.4 channel
const unsigned DEFAULT_CHANNEL = 11;

// Timeout in seconds after which a joiner is automatically removed from the commissioner's list
const unsigned JOINER_TIMEOUT = 120;

// Minimum size of the joining device credential
const size_t JOINER_PASSWORD_MIN_SIZE = 6;

// Maximum size of the joining device credential
const size_t JOINER_PASSWORD_MAX_SIZE = 32;

// Vendor data
const char* const VENDOR_NAME = "Particle";
const char* const VENDOR_MODEL = PP_STR(PLATFORM_NAME);
const char* const VENDOR_SW_VERSION = PP_STR(SYSTEM_VERSION_STRING);
const char* const VENDOR_DATA = "";

// Current joining device credential
char g_joinPwd[JOINER_PASSWORD_MAX_SIZE + 1] = {}; // +1 character for term. null

// TODO: Implement a thread-safe system API for pseudo-random number generation
class Random {
public:
    Random() :
            seed_(HAL_RNG_GetRandomNumber()) {
    }

    template<typename T>
    T gen() {
        T val = 0;
        gen((char*)&val, sizeof(val));
        return val;
    }

    void gen(char* data, size_t size) {
        while (size > 0) {
            const int v = rand_r(&seed_);
            const size_t n = std::min(size, sizeof(v));
            memcpy(data, &v, n);
            data += n;
            size -= n;
        }
    }

    void genBase32(char* str, size_t size) {
        static const char alpha[32] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '2', '3', '4', '5', '6', '7' }; // RFC 4648
        for (size_t i = 0; i < size; ++i) {
            const auto ai = gen<size_t>() % sizeof(alpha);
            str[i] = alpha[ai];
        }
    }

    static void genRandom(char* data, size_t size) {
        while (size > 0) {
            const uint32_t v = HAL_RNG_GetRandomNumber();
            const size_t n = std::min(size, sizeof(v));
            memcpy(data, &v, n);
            data += n;
            size -= n;
        }
    }

private:
    unsigned seed_;
};

} // particle::ctrl::mesh::

int auth(ctrl_request* req) {
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
    const uint8_t* extPanId = otThreadGetExtendedPanId(thread);
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
    const auto thread = threadInstance();
    if (!thread) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Parse request
    PB(CreateNetworkRequest) pbReq = {};
    DecodedCString dName(&pbReq.name); // Network name
    DecodedCString dPwd(&pbReq.password); // Commissioning credential
    int ret = decodeRequestMessage(req, PB(CreateNetworkRequest_fields), &pbReq);
    if (ret != 0) {
        return ret;
    }
    // Network name: up to 16 characters, UTF-8 encoded
    // Commissioning credential: 6 to 255 characters, UTF-8 encoded
    if (dName.size == 0 || dName.size >= OT_NETWORK_NAME_MAX_SIZE || dPwd.size < OT_COMMISSIONING_PASSPHRASE_MIN_SIZE ||
            dPwd.size > OT_COMMISSIONING_PASSPHRASE_MAX_SIZE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    CHECK_THREAD(otThreadSetEnabled(thread, false));
    CHECK_THREAD(otIp6SetEnabled(thread, false));
    // Determine IEEE 802.15.4 channel
    // TODO: Perform an energy scan?
    const unsigned channel = (pbReq.channel != 0) ? pbReq.channel : DEFAULT_CHANNEL;
    CHECK_THREAD(otLinkSetChannel(thread, channel));
    // Perform an IEEE 802.15.4 active scan
    struct ActiveScanResult {
        Vector<uint64_t> extPanIds;
        Vector<uint16_t> panIds;
        int error;
        bool done;
    };
    ActiveScanResult actScan = {};
    if (!actScan.extPanIds.reserve(4) || !actScan.panIds.reserve(4)) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    CHECK_THREAD(otLinkActiveScan(thread, (uint32_t)1 << channel, 0 /* aScanDuration */,
            [](otActiveScanResult* result, void* data) {
        const auto scan = (ActiveScanResult*)data;
        if (!result) {
            scan->done = true;
            return;
        }
        uint64_t extPanId = 0;
        static_assert(sizeof(result->mExtendedPanId) == sizeof(extPanId), "");
        memcpy(&extPanId, &result->mExtendedPanId, sizeof(extPanId));
        if (!scan->extPanIds.contains(extPanId)) {
            if (!scan->extPanIds.append(extPanId)) {
                scan->error = SYSTEM_ERROR_NO_MEMORY;
            }
        }
        const uint16_t panId = result->mPanId;
        if (!scan->panIds.contains(panId)) {
            if (!scan->panIds.append(panId)) {
                scan->error = SYSTEM_ERROR_NO_MEMORY;
            }
        }
    }, &actScan));
    do {
        // FIXME: Move OpenThread's loop to a separate thread
        threadProcess();
    } while (!actScan.done);
    if (actScan.error != 0) {
        return actScan.error;
    }
    // Generate PAN ID
    Random rand;
    uint16_t panId = 0;
    do {
        panId = rand.gen<uint16_t>();
    } while (panId == 0xffff || actScan.panIds.contains(panId));
    CHECK_THREAD(otLinkSetPanId(thread, panId));
    // Generate extended PAN ID
    uint64_t extPanId = 0;
    do {
        extPanId = rand.gen<uint64_t>();
    } while (actScan.extPanIds.contains(extPanId));
    static_assert(sizeof(extPanId) == OT_EXT_PAN_ID_SIZE, "");
    CHECK_THREAD(otThreadSetExtendedPanId(thread, (const uint8_t*)&extPanId));
    // Set network name
    CHECK_THREAD(otThreadSetNetworkName(thread, dName.data));
    // Generate mesh-local prefix (see section 3 of RFC 4193)
    uint8_t prefix[OT_MESH_LOCAL_PREFIX_SIZE] = {
            0xfd, // Prefix, L
            0x00, 0x00, 0x00, 0x00, 0x00, // Global ID
            0x00, 0x00 }; // Subnet ID
    rand.gen((char*)prefix + 1, 5); // Generate global ID
    CHECK_THREAD(otThreadSetMeshLocalPrefix(thread, prefix));
    // Generate master key
    otMasterKey key = {};
    rand.genRandom((char*)&key, sizeof(key));
    CHECK_THREAD(otThreadSetMasterKey(thread, &key));
    // Set PSKc
    uint8_t pskc[OT_PSKC_MAX_SIZE] = {};
    CHECK_THREAD(otCommissionerGeneratePSKc(thread, dPwd.data, dName.data, (const uint8_t*)&extPanId, pskc));
    CHECK_THREAD(otThreadSetPSKc(thread, pskc));
    // Enable Thread
    CHECK_THREAD(otIp6SetEnabled(thread, true));
    CHECK_THREAD(otThreadSetEnabled(thread, true));
    // Encode a reply
    char extPanIdStr[sizeof(extPanId) * 2] = {};
    bytes2hexbuf_lower_case((const uint8_t*)&extPanId, sizeof(extPanId), extPanIdStr);
    PB(CreateNetworkReply) pbRep = {};
    EncodedString eName(&pbRep.network.name, dName.data, dName.size);
    EncodedString eExtPanId(&pbRep.network.ext_pan_id, extPanIdStr, sizeof(extPanIdStr));
    pbRep.network.pan_id = panId;
    pbRep.network.channel = channel;
    ret = encodeReplyMessage(req, PB(CreateNetworkReply_fields), &pbRep);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int startCommissioner(ctrl_request* req) {
    const auto thread = threadInstance();
    if (!thread) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    const auto state = otCommissionerGetState(thread);
    if (state == OT_COMMISSIONER_STATE_DISABLED) {
        CHECK_THREAD(otCommissionerStart(thread));
        CHECK_THREAD(otCommissionerSetProvisioningUrl(thread, nullptr));
    }
    return 0;
}

int stopCommissioner(ctrl_request* req) {
    const auto thread = threadInstance();
    if (!thread) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    const auto state = otCommissionerGetState(thread);
    if (state != OT_COMMISSIONER_STATE_DISABLED) {
        CHECK_THREAD(otCommissionerStop(thread));
    }
    return 0;
}

int prepareJoiner(ctrl_request* req) {
    const auto thread = threadInstance();
    if (!thread) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Parse request
    PB(PrepareJoinerRequest) pbReq = {};
    DecodedCString dName(&pbReq.network.name);
    DecodedCString dExtPanIdStr(&pbReq.network.ext_pan_id);
    int ret = decodeRequestMessage(req, PB(PrepareJoinerRequest_fields), &pbReq);
    if (ret != 0) {
        return ret;
    }
    if (dName.size == 0 || dName.size > OT_NETWORK_NAME_MAX_SIZE) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    // Disable Thread
    CHECK_THREAD(otThreadSetEnabled(thread, false));
    CHECK_THREAD(otIp6SetEnabled(thread, false));
    // Set network name
    CHECK_THREAD(otThreadSetNetworkName(thread, dName.data));
    // Set channel
    CHECK_THREAD(otLinkSetChannel(thread, pbReq.network.channel));
    // Set PAN ID
    CHECK_THREAD(otLinkSetPanId(thread, pbReq.network.pan_id));
    // Set extended PAN ID
    uint8_t extPanId[OT_EXT_PAN_ID_SIZE] = {};
    hexToBytes(dExtPanIdStr.data, (char*)&extPanId, OT_EXT_PAN_ID_SIZE);
    CHECK_THREAD(otThreadSetExtendedPanId(thread, extPanId));
    // Get factory-assigned EUI-64
    otExtAddress eui64 = {}; // OT_EXT_ADDRESS_SIZE
    otLinkGetFactoryAssignedIeeeEui64(thread, &eui64);
    char eui64Str[sizeof(eui64) * 2] = {};
    bytes2hexbuf_lower_case((const uint8_t*)&eui64, sizeof(eui64), eui64Str);
    // Generate joining device credential
    Random rand;
    rand.genBase32(g_joinPwd, JOINER_PASSWORD_MAX_SIZE);
    // Encode a reply
    PB(PrepareJoinerReply) pbRep = {};
    EncodedString eEuiStr(&pbRep.eui64, eui64Str, sizeof(eui64Str));
    EncodedString eJoinPwd(&pbRep.password, g_joinPwd, JOINER_PASSWORD_MAX_SIZE);
    ret = encodeReplyMessage(req, PB(PrepareJoinerReply_fields), &pbRep);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int addJoiner(ctrl_request* req) {
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
    CHECK_THREAD(otCommissionerAddJoiner(thread, &eui64, dJoinPwd.data, JOINER_TIMEOUT));
    return 0;
}

int removeJoiner(ctrl_request* req) {
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
    CHECK_THREAD(otCommissionerRemoveJoiner(thread, &eui64));
    return 0;
}

void joinNetwork(ctrl_request* req) {
    const auto thread = threadInstance();
    if (!thread) {
        system_ctrl_set_result(req, SYSTEM_ERROR_INVALID_STATE, nullptr, nullptr, nullptr);
        return;
    }
    auto tRet = otIp6SetEnabled(thread, true);
    if (tRet != OT_ERROR_NONE) {
        LOG(ERROR, "otIp6SetEnabled() failed: %u", (unsigned)tRet);
        system_ctrl_set_result(req, -1, nullptr, nullptr, nullptr);
        return;
    }
    otJoinerCallback cb = [](otError tRet, void* ctx) {
        const auto req = (ctrl_request*)ctx;
        int ret = -1;
        if (tRet == OT_ERROR_NONE) {
            const auto thread = threadInstance();
            tRet = otThreadSetEnabled(thread, true);
            if (tRet == OT_ERROR_NONE) {
                ret = 0;
            } else {
                LOG(ERROR, "otThreadSetEnabled() failed: %u", (unsigned)tRet);
            }
        } else {
            LOG(ERROR, "otJoinerStart() failed: %u", (unsigned)tRet);
        }
        memset(g_joinPwd, 0, sizeof(g_joinPwd));
        system_ctrl_set_result(req, ret, nullptr, nullptr, nullptr);
    };
    tRet = otJoinerStart(thread, g_joinPwd, nullptr, VENDOR_NAME, VENDOR_MODEL, VENDOR_SW_VERSION,
            VENDOR_DATA, cb, req);
    if (tRet != OT_ERROR_NONE) {
        LOG(ERROR, "otJoinerStart() failed: %u", (unsigned)tRet);
        system_ctrl_set_result(req, -1, nullptr, nullptr, nullptr);
    }
}

int leaveNetwork(ctrl_request* req) {
    const auto thread = threadInstance();
    if (!thread) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Disable Thread protocol
    CHECK_THREAD(otThreadSetEnabled(thread, false));
    // Clear master key (invalidates datasets in non-volatile memory)
    otMasterKey key = {};
    CHECK_THREAD(otThreadSetMasterKey(thread, &key));
    // Erase persistent data
    CHECK_THREAD(otInstanceErasePersistentInfo(thread));
    return 0;
}

int getNetworkInfo(ctrl_request* req) {
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
        return -1;
    }
    // Channel
    const uint8_t channel = otLinkGetChannel(thread);
    // PAN ID
    const otPanId panId = otLinkGetPanId(thread);
    // Extended PAN ID
    const uint8_t* extPanId = otThreadGetExtendedPanId(thread);
    if (!extPanId) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    char extPanIdStr[OT_EXT_PAN_ID_SIZE * 2] = {};
    bytes2hexbuf_lower_case(extPanId, OT_EXT_PAN_ID_SIZE, extPanIdStr);
    // Enode a reply
    PB(GetNetworkInfoReply) pbRep = {};
    EncodedString eName(&pbRep.network.name, name, strlen(name));
    EncodedString eExtPanIdStr(&pbRep.network.ext_pan_id, extPanIdStr, sizeof(extPanIdStr));
    pbRep.network.channel = channel;
    pbRep.network.pan_id = panId;
    const int ret = encodeReplyMessage(req, PB(GetNetworkInfoReply_fields), &pbRep);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int scanNetworks(ctrl_request* req) {
    return SYSTEM_ERROR_NOT_SUPPORTED; // TODO
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

#endif // SYSTEM_CONTROL_ENABLED
