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
#include "timer_hal.h"
#include "ota_flash_hal_impl.h"
#include "hal_platform.h"
#include "platforms.h"
#include "dct.h"

#include "eckeygen.h"
#include "sha256.h"
#include "random.h"
#include "bytes2hexbuf.h"
#include "endian_util.h"
#include "check.h"

#if HAL_PLATFORM_NCP
#include "network/ncp/wifi/ncp.h"
#include "network/ncp/wifi/wifi_network_manager.h"
#include "network/ncp/wifi/wifi_ncp_client.h"
#endif

#include "control/config.pb.h"

#include <memory>
#include <cstdio>

#include <mbedtls/pk.h>

#define PB(_name) particle_ctrl_##_name
#define PB_FIELDS(_name) particle_ctrl_##_name##_fields

namespace particle {

namespace control {

namespace config {

namespace {

using namespace particle::control::common;

struct SecurityModeChangeContext {
    char clientNonce[32];
    uint64_t requestTime;
};

std::unique_ptr<SecurityModeChangeContext> g_securityModeChangeCtx;
bool g_securityModeChangePending = false;

// Completion handler for system_ctrl_set_result()
void systemResetCompletionHandler(int result, void* data) {
    system_reset(SYSTEM_RESET_MODE_NORMAL, RESET_REASON_CONFIG_UPDATE, 0 /* value */, 0 /* flags */, nullptr /* reserved */);
}

int getDevicePrivateKey(mbedtls_pk_context& pk) {
    std::unique_ptr<uint8_t[]> keyData(new(std::nothrow) uint8_t[DCT_ALT_DEVICE_PRIVATE_KEY_SIZE]);
    int r = dct_read_app_data_copy(DCT_ALT_DEVICE_PRIVATE_KEY_OFFSET, keyData.get(), DCT_ALT_DEVICE_PRIVATE_KEY_SIZE);
    if (r != 0) {
        return SYSTEM_ERROR_IO;
    }
    size_t keyLen = determine_der_length(keyData.get(), DCT_ALT_DEVICE_PRIVATE_KEY_SIZE);
    if (!keyLen) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    CHECK_MBEDTLS(mbedtls_pk_parse_key(&pk, keyData.get(), keyLen, nullptr /* pwd */, 0 /* pwdlen */));
    if (!mbedtls_pk_can_do(&pk, MBEDTLS_PK_ECDSA)) { // Sanity check
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    return 0;
}

int getServerPublicKey(mbedtls_pk_context& pk) {
    std::unique_ptr<uint8_t[]> keyData(new(std::nothrow) uint8_t[DCT_ALT_SERVER_PUBLIC_KEY_SIZE]);
    int r = dct_read_app_data_copy(DCT_ALT_SERVER_PUBLIC_KEY_OFFSET, keyData.get(), DCT_ALT_SERVER_PUBLIC_KEY_SIZE);
    if (r != 0) {
        return SYSTEM_ERROR_IO;
    }
    size_t keyLen = determine_der_length(keyData.get(), DCT_ALT_SERVER_PUBLIC_KEY_SIZE);
    if (!keyLen) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    CHECK_MBEDTLS(mbedtls_pk_parse_public_key(&pk, keyData.get(), keyLen));
    return 0;
}

int updateSha256Delimited(Sha256& sha, const char* data, size_t size) {
    uint32_t n = nativeToLittleEndian(size);
    CHECK(sha.update((const char*)&n, sizeof(n)));
    CHECK(sha.update(data, size));
    return 0;
}

int setProtectedStateImpl(ctrl_request* req) {
    // TODO: Remove this check once the support for TCP is fully removed
    if (!HAL_Feature_Get(FEATURE_CLOUD_UDP)) {
        return SYSTEM_ERROR_PROTOCOL;
    }
    if (g_securityModeChangePending) {
        return SYSTEM_ERROR_BUSY;
    }

    PB(SetProtectedStateRequest) pbReq = {};
    DecodedString pbServSig(&pbReq.server_signature);
    CHECK(decodeRequestMessage(req, &PB(SetProtectedStateRequest_msg), &pbReq));

    PB(SetProtectedStateReply) pbRep = {};
    bool changePending = false;

    switch (pbReq.action) {
    case PB(SetProtectedStateRequest_Action_RESET): {
        if (security_mode_is_overridden()) {
            security_mode_clear_override();
            changePending = true;
        }
        g_securityModeChangeCtx.reset();
        break;
    }
    case PB(SetProtectedStateRequest_Action_DISABLE_REQUEST): {
        g_securityModeChangeCtx.reset();
        if (security_mode_get(nullptr) == MODULE_INFO_SECURITY_MODE_NONE) {
            return 0; // Not protected
        }
        std::unique_ptr<SecurityModeChangeContext> ctx(new(std::nothrow) SecurityModeChangeContext());
        if (!ctx) {
            return SYSTEM_ERROR_NO_MEMORY;
        }

        // Get the device ID and private key
        uint8_t devId[HAL_DEVICE_ID_SIZE] = {}; // Binary-encoded
        const auto n = hal_get_device_id(devId, sizeof(devId));
        if (n != HAL_DEVICE_ID_SIZE) {
            return SYSTEM_ERROR_UNKNOWN;
        }
        mbedtls_pk_context pk = {};
        mbedtls_pk_init(&pk);
        SCOPE_GUARD({
            mbedtls_pk_free(&pk);
        });
        CHECK(getDevicePrivateKey(pk));

        // Generate a client nonce and signature
        Random::genSecure(ctx->clientNonce, sizeof(ctx->clientNonce));
        Sha256 sha;
        CHECK(sha.init());
        CHECK(sha.start());
        CHECK(sha.update("client", 6));
        CHECK(sha.update((const char*)devId, HAL_DEVICE_ID_SIZE));
        updateSha256Delimited(sha, ctx->clientNonce, sizeof(ctx->clientNonce));
        char hash[Sha256::HASH_SIZE] = {};
        CHECK(sha.finish(hash));

        uint8_t sig[MBEDTLS_ECDSA_MAX_LEN] = {};
        size_t sigLen = 0;
        CHECK_MBEDTLS(mbedtls_pk_sign(&pk, MBEDTLS_MD_SHA256, (const uint8_t*)hash, sizeof(hash), sig, &sigLen,
                mbedtls_default_rng, nullptr /* p_rng */));

        // Encode a reply
        EncodedString pbSig(&pbRep.client_signature, (const char*)sig, sigLen);
        static_assert(sizeof(ctx->clientNonce) <= sizeof(pbRep.client_nonce.bytes));
        std::memcpy(pbRep.client_nonce.bytes, ctx->clientNonce, sizeof(ctx->clientNonce));
        pbRep.client_nonce.size = sizeof(ctx->clientNonce);
        pbRep.has_client_nonce = true;
        CHECK(encodeReplyMessage(req, &PB(SetProtectedStateReply_msg), &pbRep));

        ctx->requestTime = hal_timer_millis(nullptr);
        g_securityModeChangeCtx = std::move(ctx);
        break;
    }
    case PB(SetProtectedStateRequest_Action_DISABLE_CONFIRM): {
        if (!pbReq.has_server_nonce || !pbReq.server_nonce.size) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if (!g_securityModeChangeCtx) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        if (hal_timer_millis(nullptr) - g_securityModeChangeCtx->requestTime >= 60000) {
            g_securityModeChangeCtx.reset();
            return SYSTEM_ERROR_TIMEOUT;
        }

        // Get the device ID and server public key
        uint8_t devId[HAL_DEVICE_ID_SIZE] = {};
        const auto n = hal_get_device_id(devId, sizeof(devId));
        if (n != HAL_DEVICE_ID_SIZE) {
            return SYSTEM_ERROR_UNKNOWN;
        }
        mbedtls_pk_context pk = {};
        mbedtls_pk_init(&pk);
        SCOPE_GUARD({
            mbedtls_pk_free(&pk);
        });
        CHECK(getServerPublicKey(pk));

        // Validate the server signature
        Sha256 sha;
        CHECK(sha.init());
        CHECK(sha.start());
        CHECK(sha.update("server", 6));
        CHECK(sha.update((const char*)devId, HAL_DEVICE_ID_SIZE));
        updateSha256Delimited(sha, (const char*)pbReq.server_nonce.bytes, pbReq.server_nonce.size);
        updateSha256Delimited(sha, g_securityModeChangeCtx->clientNonce, sizeof(g_securityModeChangeCtx->clientNonce));
        char hash[Sha256::HASH_SIZE] = {};
        CHECK(sha.finish(hash));

        int r = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, (const uint8_t*)hash, sizeof(hash), (const uint8_t*)pbServSig.data,
                pbServSig.size);
        if (r != 0) {
            return SYSTEM_ERROR_NOT_ALLOWED;
        }

        g_securityModeChangeCtx.reset();
        security_mode_set_override();
        changePending = true;
        break;
    }
    default:
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    g_securityModeChangePending = changePending;
    auto compHandler = changePending ? systemResetCompletionHandler : nullptr;
    system_ctrl_set_result(req, 0 /* result */, compHandler, nullptr /* data */, nullptr /* reserved */);

    return 0;
}

} // namespace

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

int getProtectedState(ctrl_request* req) {
    PB(GetProtectedStateReply) pbRep = {};
    pbRep.state = security_mode_get(nullptr) == MODULE_INFO_SECURITY_MODE_PROTECTED;
    pbRep.overridden = security_mode_is_overridden();
    CHECK(encodeReplyMessage(req, PB(GetProtectedStateReply_fields), &pbRep));
    return 0;
}

void setProtectedState(ctrl_request* req) {
    int r = setProtectedStateImpl(req);
    if (r < 0) {
        system_ctrl_set_result(req, r, nullptr /* handler */, nullptr /* data */, nullptr /* reserved */);
    }
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
