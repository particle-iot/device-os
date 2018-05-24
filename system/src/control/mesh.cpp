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

#include "system_openthread.h"

#include "common.h"

#include "logging.h"
#include "bytes2hexbuf.h"
#include "hex_to_bytes.h"

#include "openthread/thread.h"
#include "openthread/thread_ftd.h"
#include "openthread/instance.h"
#include "openthread/commissioner.h"
#include "openthread/joiner.h"
#include "openthread/dataset.h"
#include "openthread/dataset_ftd.h"
#include "openthread/ip6.h"

#include "proto/mesh.pb.h"

#define CHECK_THREAD(_expr) \
    do { \
        const auto ret = _expr; \
        if (ret != OT_ERROR_NONE) { \
            LOG(ERROR, #_expr " failed: %d", (int)ret); \
            return SYSTEM_ERROR_UNKNOWN; \
        } \
    } while (false)

#define PB(_name) particle_ctrl_mesh_##_name

namespace particle {

using namespace control::common;
using namespace system;

namespace ctrl {

namespace mesh {

namespace {

// FIXME
const char* const VENDOR_NAME = "Particle";
const char* const VENDOR_MODEL = "HW 2.0";
const char* const VENDOR_SW_VERSION = "0.1.0";
const char* const VENDOR_DATA = "";

const char* g_joinPwd = "JNERPASSWRD"; // FIXME

} // ::

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
    const uint8_t* extPan = otThreadGetExtendedPanId(thread);
    const uint8_t* curPskc = otThreadGetPSKc(thread);
    if (!name || !extPan || !curPskc) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Generate PSKc for the provided commissioning credential
    uint8_t pskc[OT_PSKC_MAX_SIZE] = {};
    CHECK_THREAD(otCommissionerGeneratePSKc(thread, dPwd.data, name, extPan, pskc));
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
    if (dName.size == 0 || dName.size >= 16 || dPwd.size < 6 || dPwd.size > 255) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    CHECK_THREAD(otThreadSetEnabled(thread, false));
    CHECK_THREAD(otIp6SetEnabled(thread, false));
    // Generate PAN ID
    const otPanId pan = 0x1234; // FIXME
    CHECK_THREAD(otLinkSetPanId(thread, pan));
    // Generate extended PAN ID
    const uint8_t extPan[OT_EXT_PAN_ID_SIZE] = { 0xa0, 0xb1, 0xc2, 0xd3, 0xe4, 0xf5, 0xe4, 0xd3 }; // FIXME
    CHECK_THREAD(otThreadSetExtendedPanId(thread, extPan));
    // Set network name
    CHECK_THREAD(otThreadSetNetworkName(thread, dName.data));
    // Set network channel
    const uint8_t channel = 11; // FIXME
    CHECK_THREAD(otLinkSetChannel(thread, channel));
    // Update PSKc
    uint8_t pskc[OT_PSKC_MAX_SIZE] = {};
    CHECK_THREAD(otCommissionerGeneratePSKc(thread, dPwd.data, dName.data, extPan, pskc));
    CHECK_THREAD(otThreadSetPSKc(thread, pskc));
    // Generate master key
    otMasterKey key = {};
    memset(&key, 0x1a, sizeof(key)); // FIXME
    CHECK_THREAD(otThreadSetMasterKey(thread, &key));
    // Enable Thread
    CHECK_THREAD(otIp6SetEnabled(thread, true));
    CHECK_THREAD(otThreadSetEnabled(thread, true));
    // TODO: Do not use hex encoding in the protocol
    char panStr[sizeof(pan) * 2] = {};
    bytes2hexbuf_lower_case((const uint8_t*)&pan, sizeof(pan), panStr);
    char extPanStr[sizeof(extPan) * 2] = {};
    bytes2hexbuf_lower_case((const uint8_t*)&extPan, sizeof(extPan), extPanStr);
    PB(CreateNetworkReply) pbRep = {};
    EncodedString eName(&pbRep.network.name, dName.data, dName.size);
    EncodedString ePan(&pbRep.network.pan, panStr, sizeof(panStr));
    EncodedString eExtPan(&pbRep.network.ext_pan, extPanStr, sizeof(extPanStr));
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
    DecodedCString dPanStr(&pbReq.network.pan);
    DecodedCString dExtPanStr(&pbReq.network.ext_pan);
    int ret = decodeRequestMessage(req, PB(PrepareJoinerRequest_fields), &pbReq);
    if (ret != 0) {
        return ret;
    }
    if (dName.size == 0 || dName.size > 16 || dPanStr.size != sizeof(otPanId) * 2 ||
            dExtPanStr.size != OT_EXT_PAN_ID_SIZE * 2) {
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
    otPanId pan = 0;
    hexToBytes(dPanStr.data, (char*)&pan, sizeof(otPanId));
    CHECK_THREAD(otLinkSetPanId(thread, pan));
    // Set extended PAN ID
    uint8_t extPan[OT_EXT_PAN_ID_SIZE] = {};
    hexToBytes(dExtPanStr.data, (char*)&extPan, OT_EXT_PAN_ID_SIZE);
    CHECK_THREAD(otThreadSetExtendedPanId(thread, extPan));
    // Get factory-assigned EUI-64
    otExtAddress eui = {}; // OT_EXT_ADDRESS_SIZE
    otLinkGetFactoryAssignedIeeeEui64(thread, &eui);
    char euiStr[sizeof(eui) * 2] = {};
    bytes2hexbuf_lower_case((const uint8_t*)&eui, sizeof(eui), euiStr);
    // TODO: Generate joining device credential
    PB(PrepareJoinerReply) pbRep = {};
    EncodedString eEuiStr(&pbRep.eui64, euiStr, sizeof(euiStr));
    EncodedString eJoinPwd(&pbRep.password, g_joinPwd, strlen(g_joinPwd));
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
    DecodedCString dEuiStr(&pbReq.eui64);
    DecodedCString dJoinPwd(&pbReq.password);
    int ret = decodeRequestMessage(req, PB(AddJoinerRequest_fields), &pbReq);
    if (ret != 0) {
        return ret;
    }
    if (dEuiStr.size != sizeof(otExtAddress) * 2) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    // Add joiner
    otExtAddress eui = {};
    hexToBytes(dEuiStr.data, (char*)&eui, sizeof(otExtAddress));
    CHECK_THREAD(otCommissionerAddJoiner(thread, &eui, dJoinPwd.data, 60));
    return 0;
}

int removeJoiner(ctrl_request* req) {
    const auto thread = threadInstance();
    if (!thread) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    // Parse request
    PB(RemoveJoinerRequest) pbReq = {};
    DecodedCString dEuiStr(&pbReq.eui64);
    int ret = decodeRequestMessage(req, PB(RemoveJoinerRequest_fields), &pbReq);
    if (ret != 0) {
        return ret;
    }
    if (dEuiStr.size != sizeof(otExtAddress) * 2) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    // Remove joiner
    otExtAddress eui = {};
    hexToBytes(dEuiStr.data, (char*)&eui, sizeof(otExtAddress));
    CHECK_THREAD(otCommissionerRemoveJoiner(thread, &eui));
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
    // Clear master key (invalidates operational datasets in non-volatile memory)
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
    const otPanId pan = otLinkGetPanId(thread);
    char panStr[sizeof(otPanId) * 2] = {};
    bytes2hexbuf_lower_case((const uint8_t*)&pan, sizeof(pan), panStr);
    // Extended PAN ID
    const uint8_t* extPan = otThreadGetExtendedPanId(thread);
    if (!extPan) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    char extPanStr[OT_EXT_PAN_ID_SIZE * 2] = {};
    bytes2hexbuf_lower_case(extPan, OT_EXT_PAN_ID_SIZE, extPanStr);
    // Enode reply
    PB(GetNetworkInfoReply) pbRep = {};
    EncodedString eName(&pbRep.network.name, name, strlen(name));
    EncodedString ePanStr(&pbRep.network.pan, panStr, sizeof(panStr));
    EncodedString eExtPanStr(&pbRep.network.ext_pan, extPanStr, sizeof(extPanStr));
    pbRep.network.channel = channel;
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
