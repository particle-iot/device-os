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

#if HAL_PLATFORM_OPENTHREAD
#include "ota_flash_hal_impl.h"
#else
#include "ota_flash_hal_stm32f2xx.h"
#endif

#if HAL_PLATFORM_NCP
#include "network/ncp.h"
#include "wifi_network_manager.h"
#include "wifi_ncp_client.h"
#endif

#include "config.pb.h"

#include <cstdio>

#define PB(_name) particle_ctrl_##_name
#define PB_FIELDS(_name) particle_ctrl_##_name##_fields

namespace particle {

namespace control {

namespace config {

using namespace particle::control::common;

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
    pbRep.flags |= PB(SystemCapabilityFlag_COMPRESSED_OTA);
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

#if !HAL_PLATFORM_OPENTHREAD

int handleSetSecurityKeyRequest(ctrl_request* req) {
    particle_ctrl_SetSecurityKeyRequest pbReq = {};
    DecodedString pbKey(&pbReq.data);
    int ret = decodeRequestMessage(req, particle_ctrl_SetSecurityKeyRequest_fields, &pbReq);
    if (ret == 0) {
        ret = store_security_key_data((security_key_type)pbReq.type, pbKey.data, pbKey.size);
    }
    return ret;
}

int handleGetSecurityKeyRequest(ctrl_request* req) {
    particle_ctrl_GetSecurityKeyRequest pbReq = {};
    int ret = decodeRequestMessage(req, particle_ctrl_GetSecurityKeyRequest_fields, &pbReq);
    if (ret == 0) {
        particle_ctrl_GetSecurityKeyReply pbRep = {};
        EncodedString pbKey(&pbRep.data);
        ret = lock_security_key_data((security_key_type)pbReq.type, &pbKey.data, &pbKey.size);
        if (ret == 0) {
            ret = encodeReplyMessage(req, particle_ctrl_GetSecurityKeyReply_fields, &pbRep);
            unlock_security_key_data((security_key_type)pbReq.type);
        }
    }

    return ret;
}

int handleSetServerAddressRequest(ctrl_request* req) {
    particle_ctrl_SetServerAddressRequest pbReq = {};
    int ret = decodeRequestMessage(req, particle_ctrl_SetServerAddressRequest_fields, &pbReq);
    if (ret == 0) {
        ServerAddress addr = {};
        // Check if the address string contains an IP address
        // TODO: Move IP address parsing/encoding to separate functions
        unsigned n1 = 0, n2 = 0, n3 = 0, n4 = 0;
        if (sscanf(pbReq.address, "%u.%u.%u.%u", &n1, &n2, &n3, &n4) == 4) {
            addr.addr_type = IP_ADDRESS;
            addr.ip = ((n1 & 0xff) << 24) | ((n2 & 0xff) << 16) | ((n3 & 0xff) << 8) | (n4 & 0xff);
        } else {
            const size_t n = strlen(pbReq.address);
            if (n < sizeof(ServerAddress::domain)) {
                addr.addr_type = DOMAIN_NAME;
                addr.length = n;
                memcpy(addr.domain, pbReq.address, n);
            } else {
                ret = SYSTEM_ERROR_TOO_LARGE;
            }
        }
        if (ret == 0) {
            addr.port = pbReq.port;
            ret = store_server_address((server_protocol_type)pbReq.protocol, &addr);
        }
    }
    return ret;
}

int handleGetServerAddressRequest(ctrl_request* req) {
    particle_ctrl_GetServerAddressRequest pbReq = {};
    int ret = decodeRequestMessage(req, particle_ctrl_GetServerAddressRequest_fields, &pbReq);
    if (ret == 0) {
        ServerAddress addr = {};
        ret = load_server_address((server_protocol_type)pbReq.protocol, &addr);
        if (ret == 0) {
            if (addr.addr_type == IP_ADDRESS) {
                const unsigned n1 = (addr.ip >> 24) & 0xff,
                        n2 = (addr.ip >> 16) & 0xff,
                        n3 = (addr.ip >> 8) & 0xff,
                        n4 = addr.ip & 0xff;
                const int n = snprintf(addr.domain, sizeof(addr.domain), "%u.%u.%u.%u", n1, n2, n3, n4);
                if (n > 0 && (size_t)n < sizeof(addr.domain)) {
                    addr.length = n;
                } else {
                    ret = SYSTEM_ERROR_TOO_LARGE;
                }
            }
            if (ret == 0) {
                particle_ctrl_GetServerAddressReply pbRep = {};
                EncodedString pbAddr(&pbRep.address, addr.domain, addr.length);
                pbRep.port = addr.port;
                ret = encodeReplyMessage(req, particle_ctrl_GetServerAddressReply_fields, &pbRep);
            }
        }
    }
    return ret;
}

int handleSetServerProtocolRequest(ctrl_request* req) {
    particle_ctrl_SetServerProtocolRequest pbReq = {};
    int ret = decodeRequestMessage(req, particle_ctrl_SetServerProtocolRequest_fields, &pbReq);
    if (ret == 0) {
        bool udpEnabled = false;
        if (pbReq.protocol == particle_ctrl_ServerProtocolType_TCP_PROTOCOL ||
                (udpEnabled = (pbReq.protocol == particle_ctrl_ServerProtocolType_UDP_PROTOCOL))) {
            ret = HAL_Feature_Set(FEATURE_CLOUD_UDP, udpEnabled);
        } else {
            ret = SYSTEM_ERROR_NOT_SUPPORTED;
        }
    }
    return ret;
}

int handleGetServerProtocolRequest(ctrl_request* req) {
    particle_ctrl_GetServerProtocolReply pbRep = {};
    if (HAL_Feature_Get(FEATURE_CLOUD_UDP)) {
        pbRep.protocol = particle_ctrl_ServerProtocolType_UDP_PROTOCOL;
    } else {
        pbRep.protocol = particle_ctrl_ServerProtocolType_TCP_PROTOCOL;
    }
    const int ret = encodeReplyMessage(req, particle_ctrl_GetServerProtocolReply_fields, &pbRep);
    return ret;
}

int handleSetSoftapSsidRequest(ctrl_request* req) {
    particle_ctrl_SetSoftApSsidRequest pbReq = {};
    int ret = decodeRequestMessage(req, particle_ctrl_SetSoftApSsidRequest_fields, &pbReq);
    if (ret == 0 && (!HAL_Set_System_Config(SYSTEM_CONFIG_SOFTAP_PREFIX, pbReq.prefix, strlen(pbReq.prefix)) ||
            !HAL_Set_System_Config(SYSTEM_CONFIG_SOFTAP_SUFFIX, pbReq.suffix, strlen(pbReq.suffix)))) {
        ret = SYSTEM_ERROR_UNKNOWN;
    }
    return ret;
}

#else // HAL_PLATFORM_OPENTHREAD

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

#endif

} // particle::control::config

} // particle::control

} // particle

#endif // SYSTEM_CONTROL_ENABLED
