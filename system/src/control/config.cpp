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
#include "parse_server_address.h"

#include "deviceid_hal.h"
#include "core_hal.h"
#include "hal_platform.h"
#include "platforms.h"

#include "bytes2hexbuf.h"
#include "check.h"

#include "dct.h"

#if HAL_PLATFORM_STM32F2XX
#include "ota_flash_hal_stm32f2xx.h"
#else
#include "ota_flash_hal_impl.h"
#endif

#if HAL_PLATFORM_NCP
#include "network/ncp/wifi/ncp.h"
#include "network/ncp/wifi/wifi_network_manager.h"
#include "network/ncp/wifi/wifi_ncp_client.h"
#endif

#include "config.pb.h"

#include <cstdio>

#define PB(_name) particle_ctrl_##_name
#define PB_FIELDS(_name) particle_ctrl_##_name##_fields

namespace particle {

namespace control {

namespace config {

namespace {

using namespace particle::control::common;

template<typename T>
int getSystemConfig(hal_system_config_t param, std::unique_ptr<T>* data) {
    data->reset(new(std::nothrow) T());
    if (!data) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    return HAL_Get_System_Config(param, data->get(), sizeof(T));
}

template<>
int getSystemConfig<char[]>(hal_system_config_t param, std::unique_ptr<char[]>* data) {
    const size_t size = CHECK(HAL_Get_System_Config(param, nullptr, 0));
    data->reset(new(std::nothrow) char[size]);
    if (!data) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    return HAL_Get_System_Config(param, data->get(), size);
}

} // namespace

int getDeviceId(ctrl_request* req) {
    uint8_t id[HAL_DEVICE_ID_SIZE] = {};
    const auto n = HAL_device_ID(id, sizeof(id));
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
#if HAL_PLATFORM_COMPRESSED_BINARIES
    pbRep.flags |= PB(SystemCapabilityFlag_COMPRESSED_OTA);
#endif
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
#if HAL_PLATFORM_DCT_SETUP_DONE
    PB(SetDeviceSetupDoneRequest) pbReq = {};
    int ret = decodeRequestMessage(req, PB(SetDeviceSetupDoneRequest_fields), &pbReq);
    if (ret != 0) {
        return ret;
    }
    LOG_DEBUG(TRACE, "%s device setup flag", pbReq.done ? "Setting" : "Clearing");
    const uint8_t val = pbReq.done ? 0x01 : 0xff;
    ret = dct_write_app_data(&val, DCT_SETUP_DONE_OFFSET, 1);
    if (ret != 0) {
        return ret;
    }
    return 0;
#else
    return SYSTEM_ERROR_NOT_SUPPORTED;
#endif // HAL_PLATFORM_DCT_SETUP_DONE
}

int isDeviceSetupDone(ctrl_request* req) {
#if HAL_PLATFORM_DCT_SETUP_DONE
    uint8_t val = 0;
    int ret = dct_read_app_data_copy(DCT_SETUP_DONE_OFFSET, &val, 1);
    if (ret != 0) {
        return ret;
    }
    PB(IsDeviceSetupDoneReply) pbRep = {};
    pbRep.done = (val == 0x01) ? true : false;
    ret = encodeReplyMessage(req, PB(IsDeviceSetupDoneReply_fields), &pbRep);
    if (ret != 0) {
        return ret;
    }
    return 0;
#else
    return SYSTEM_ERROR_NOT_SUPPORTED;
#endif // HAL_PLATFORM_DCT_SETUP_DONE
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

int setDevicePrivateKey(ctrl_request* req) {
    PB(SetDevicePrivateKeyRequest) pbReq = {};
    DecodedString pbKey(&pbReq.data);
    CHECK(decodeRequestMessage(req, PB(SetDevicePrivateKeyRequest_fields), &pbReq));
    CHECK(HAL_Set_System_Config(SYSTEM_CONFIG_DEVICE_PRIVATE_KEY, pbKey.data, pbKey.size));
    return 0;
}

int getDevicePrivateKey(ctrl_request* req) {
    std::unique_ptr<char[]> key;
    const size_t keySize = CHECK(getSystemConfig(SYSTEM_CONFIG_DEVICE_PRIVATE_KEY, &key));
    PB(GetDevicePrivateKeyReply) pbRep = {};
    EncodedString pbKey(&pbRep.data, key.get(), keySize);
    CHECK(encodeReplyMessage(req, PB(GetDevicePrivateKeyReply_fields), &pbRep));
    return 0;
}

int getDevicePublicKey(ctrl_request* req) {
    std::unique_ptr<char[]> key;
    const size_t keySize = CHECK(getSystemConfig(SYSTEM_CONFIG_DEVICE_PUBLIC_KEY, &key));
    PB(GetDevicePublicKeyReply) pbRep = {};
    EncodedString pbKey(&pbRep.data, key.get(), keySize);
    CHECK(encodeReplyMessage(req, PB(GetDevicePublicKeyReply_fields), &pbRep));
    return 0;
}

int setServerPublicKey(ctrl_request* req) {
    PB(SetServerPublicKeyRequest) pbReq = {};
    DecodedString pbKey(&pbReq.data);
    CHECK(decodeRequestMessage(req, PB(SetServerPublicKeyRequest_fields), &pbReq));
    CHECK(HAL_Set_System_Config(SYSTEM_CONFIG_SERVER_PUBLIC_KEY, pbKey.data, pbKey.size));
    return 0;
}

int getServerPublicKey(ctrl_request* req) {
    std::unique_ptr<char[]> key;
    const size_t keySize = CHECK(getSystemConfig(SYSTEM_CONFIG_SERVER_PUBLIC_KEY, &key));
    PB(GetServerPublicKeyReply) pbRep = {};
    EncodedString pbKey(&pbRep.data, key.get(), keySize);
    CHECK(encodeReplyMessage(req, PB(GetServerPublicKeyReply_fields), &pbRep));
    return 0;
}

int setServerAddress(ctrl_request* req) {
    PB(SetServerAddressRequest) pbReq = {};
    CHECK(decodeRequestMessage(req, PB(SetServerAddressRequest_fields), &pbReq));
    ServerAddress addr = {};
    CHECK(serverAddressFromString(&addr, pbReq.address));
    addr.port = pbReq.port;
    CHECK(HAL_Set_System_Config(SYSTEM_CONFIG_SERVER_ADDRESS, &addr, sizeof(addr)));
    return 0;
}

int getServerAddress(ctrl_request* req) {
    std::unique_ptr<ServerAddress> addr;
    CHECK(getSystemConfig(SYSTEM_CONFIG_SERVER_ADDRESS, &addr));
    char addrStr[64] = {};
    const size_t addrStrLen = CHECK(serverAddressToString(*addr, addrStr, sizeof(addrStr)));
    PB(GetServerAddressReply) pbRep = {};
    EncodedString pbAddr(&pbRep.address, addrStr, addrStrLen);
    pbRep.port = addr->port;
    CHECK(encodeReplyMessage(req, PB(GetServerAddressReply_fields), &pbRep));
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

#if PLATFORM_ID == PLATFORM_PHOTON

int handleSetSoftapSsidRequest(ctrl_request* req) {
    particle_ctrl_SetSoftApSsidRequest pbReq = {};
    int ret = decodeRequestMessage(req, particle_ctrl_SetSoftApSsidRequest_fields, &pbReq);
    if (ret == 0 && (!HAL_Set_System_Config(SYSTEM_CONFIG_SOFTAP_PREFIX, pbReq.prefix, strlen(pbReq.prefix)) ||
            !HAL_Set_System_Config(SYSTEM_CONFIG_SOFTAP_SUFFIX, pbReq.suffix, strlen(pbReq.suffix)))) {
        ret = SYSTEM_ERROR_UNKNOWN;
    }
    return ret;
}

#endif // PLATFORM_ID == PLATFORM_PHOTON

} // particle::control::config

} // particle::control

} // particle

#endif // SYSTEM_CONTROL_ENABLED
