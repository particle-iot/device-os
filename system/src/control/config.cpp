/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
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

#include "config.h"

#if SYSTEM_CONTROL_ENABLED

#include "common.h"
#include "system_cloud_internal.h"
#include "system_update.h"
#include "system_network.h"

#include "deviceid_hal.h"
#include "core_hal.h"
#include "hal_platform.h"
#include "platforms.h"

#include "bytes2hexbuf.h"
#include "check.h"

#include "dct.h"

#include "ota_flash_hal_impl.h"

#if HAL_PLATFORM_NCP
#include "network/ncp/wifi/ncp.h"
#include "network/ncp/wifi/wifi_network_manager.h"
#include "network/ncp/wifi/wifi_ncp_client.h"
#endif

#include "control/config.pb.h"

#include <cstdio>

#define PB(_name) particle_ctrl_##_name
#define PB_FIELDS(_name) particle_ctrl_##_name##_fields

namespace particle {

namespace control {

namespace config {

using namespace particle::control::common;

int getDeviceId(ctrl_request* req) {
    uint8_t id[HAL_DEVICE_ID_SIZE] = {};
    const auto n = hal_get_device_id(id, sizeof(id));
    if (n != HAL_DEVICE_ID_SIZE) {
        return SYSTEM_ERROR_UNKNOWN;
    }
    PB(GetDeviceIdReply) pbRep = {};
    static_assert(sizeof(pbRep.id) >= sizeof(id) * 2, "");
    bytes2hexbuf_lower_case(id, sizeof(id), pbRep.id);
    const int ret = encodeReplyMessage(req, PB_FIELDS(GetDeviceIdReply), &pbRep);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int getSerialNumber(ctrl_request* req) {
    PB(GetSerialNumberReply) pbRep = {};
    static_assert(sizeof(pbRep.serial) >= HAL_DEVICE_SERIAL_NUMBER_SIZE, "");
    int ret = hal_get_device_serial_number(pbRep.serial, sizeof(pbRep.serial), nullptr);
    if (ret < 0) {
        return ret;
    }
    ret = encodeReplyMessage(req, PB_FIELDS(GetSerialNumberReply), &pbRep);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int getSystemVersion(ctrl_request* req) {
    const auto verStr = PP_STR(SYSTEM_VERSION_STRING);
    PB(GetSystemVersionReply) pbRep = {};
    EncodedString eVerStr(&pbRep.version, verStr, strlen(verStr));
    const int ret = encodeReplyMessage(req, PB_FIELDS(GetSystemVersionReply), &pbRep);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int getNcpFirmwareVersion(ctrl_request* req) {
#if HAL_PLATFORM_NCP && HAL_PLATFORM_WIFI
    const auto wifiMgr = wifiNetworkManager();
    CHECK_TRUE(wifiMgr, SYSTEM_ERROR_UNKNOWN);
    const auto ncpClient = wifiMgr->ncpClient();
    CHECK_TRUE(ncpClient, SYSTEM_ERROR_UNKNOWN);
    const NcpClientLock lock(ncpClient);
    CHECK(ncpClient->on());
    char verStr[32] = {};
    CHECK(ncpClient->getFirmwareVersionString(verStr, sizeof(verStr)));
    uint16_t modVer = 0;
    CHECK(ncpClient->getFirmwareModuleVersion(&modVer));
    PB(GetNcpFirmwareVersionReply) pbRep = {};
    EncodedString eVerStr(&pbRep.version, verStr, strlen(verStr));
    pbRep.module_version = modVer;
    CHECK(encodeReplyMessage(req, PB_FIELDS(GetNcpFirmwareVersionReply), &pbRep));
    return 0;
#else
    return SYSTEM_ERROR_NOT_SUPPORTED;
#endif // !(HAL_PLATFORM_NCP && HAL_PLATFORM_WIFI)
}

int getSystemCapabilities(ctrl_request* req) {
    PB(GetSystemCapabilitiesReply) pbRep = {};
    CHECK(encodeReplyMessage(req, PB(GetSystemCapabilitiesReply_fields), &pbRep));
    return 0;
}

int handleSetClaimCodeRequest(ctrl_request* req) {
    particle_ctrl_SetClaimCodeRequest pbReq = {};
    int ret = decodeRequestMessage(req, particle_ctrl_SetClaimCodeRequest_fields, &pbReq);
    if (ret == 0) {
        ret = HAL_Set_Claim_Code(pbReq.code);
    }
    return ret;
}

int handleIsClaimedRequest(ctrl_request* req) {
    particle_ctrl_IsClaimedReply pbRep = {};
    pbRep.claimed = HAL_IsDeviceClaimed(nullptr);
    const int ret = encodeReplyMessage(req, particle_ctrl_IsClaimedReply_fields, &pbRep);
    return ret;
}

int handleStartNyanRequest(ctrl_request* req) {
    Spark_Signal(true, 0, nullptr);
    return SYSTEM_ERROR_NONE;
}

int handleStopNyanRequest(ctrl_request* req) {
    Spark_Signal(false, 0, nullptr);
    return SYSTEM_ERROR_NONE;
}

int getDeviceMode(ctrl_request* req) {
    const bool listening = network_listening(0, 0, nullptr);
    PB(GetDeviceModeReply) pbRep = {};
    pbRep.mode = listening ? PB(DeviceMode_LISTENING_MODE) : PB(DeviceMode_NORMAL_MODE);
    const int ret = encodeReplyMessage(req, PB(GetDeviceModeReply_fields), &pbRep);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int setDeviceSetupDone(ctrl_request* req) {
    // This functionality is currently deprecated.
    // Do not perform any DCT accesses. Instead return an appropriate error code
    PB(SetDeviceSetupDoneRequest) pbReq = {};
    int ret = decodeRequestMessage(req, PB(SetDeviceSetupDoneRequest_fields), &pbReq);
    if (ret != 0) {
        return ret;
    }
    return (pbReq.done ? SYSTEM_ERROR_NONE : SYSTEM_ERROR_NOT_SUPPORTED);
}

int isDeviceSetupDone(ctrl_request* req) {
    PB(IsDeviceSetupDoneReply) pbRep = {};
    // This functionality is currently deprecated. Hard code setup-done to always return true
    pbRep.done = true;
    int ret = encodeReplyMessage(req, PB(IsDeviceSetupDoneReply_fields), &pbRep);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int setStartupMode(ctrl_request* req) {
    PB(SetStartupModeRequest) pbReq = {};
    CHECK(decodeRequestMessage(req, PB(SetStartupModeRequest_fields), &pbReq));
    switch (pbReq.mode) {
    case PB(DeviceMode_LISTENING_MODE):
        CHECK(system_set_flag(SYSTEM_FLAG_STARTUP_LISTEN_MODE, 1, nullptr));
        break;
    default:
        CHECK(system_set_flag(SYSTEM_FLAG_STARTUP_LISTEN_MODE, 0, nullptr));
        break;
    }
    return 0;
}

int setFeature(ctrl_request* req) {
    PB(SetFeatureRequest) pbReq = {};
    CHECK(decodeRequestMessage(req, PB(SetFeatureRequest_fields), &pbReq));
    switch (pbReq.feature) {
    case PB(Feature_ETHERNET_DETECTION):
        CHECK(HAL_Feature_Set(FEATURE_ETHERNET_DETECTION, pbReq.enabled));
        break;
    default:
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    return 0;
}

int getFeature(ctrl_request* req) {
    PB(GetFeatureRequest) pbReq = {};
    CHECK(decodeRequestMessage(req, PB(GetFeatureRequest_fields), &pbReq));
    PB(GetFeatureReply) pbRep = {};
    switch (pbReq.feature) {
    case PB(Feature_ETHERNET_DETECTION):
        pbRep.enabled = HAL_Feature_Get(FEATURE_ETHERNET_DETECTION);
        break;
    default:
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    CHECK(encodeReplyMessage(req, PB(GetFeatureReply_fields), &pbRep));
    return 0;
}

int echo(ctrl_request* req) {
    const int ret = system_ctrl_alloc_reply_data(req, req->request_size, nullptr);
    if (ret != 0) {
        return ret;
    }
    memcpy(req->reply_data, req->request_data, req->request_size);
    return 0;
}

// TODO
int handleSetSecurityKeyRequest(ctrl_request*) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int handleGetSecurityKeyRequest(ctrl_request*) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int handleSetServerAddressRequest(ctrl_request*) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int handleGetServerAddressRequest(ctrl_request*) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int handleSetServerProtocolRequest(ctrl_request*) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int handleGetServerProtocolRequest(ctrl_request*) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int handleSetSoftapSsidRequest(ctrl_request*) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

} // particle::control::config

} // particle::control

} // particle

#endif // SYSTEM_CONTROL_ENABLED
