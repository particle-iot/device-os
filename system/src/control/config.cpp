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
#include "config.pb.h"
#include <cstdio>

#include "core_hal.h"
#include "ota_flash_hal_stm32f2xx.h"
#include "system_cloud_internal.h"

namespace particle {
namespace control {
namespace config {

using namespace particle::control::common;

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

int handleStartNyanRequest(ctrl_request* req) {
    Spark_Signal(true, 0, nullptr);
    return SYSTEM_ERROR_NONE;
}

int handleStopNyanRequest(ctrl_request* req) {
    Spark_Signal(false, 0, nullptr);
    return SYSTEM_ERROR_NONE;
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

} } } /* namespace particle::control::config */

#endif /* #if SYSTEM_CONTROL_ENABLED */
