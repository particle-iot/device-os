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

#include "wifi.h"

#if SYSTEM_CONTROL_ENABLED && !HAL_PLATFORM_NCP

#include "common.h"
#include "wifi.pb.h"
#include "spark_wiring_platform.h"
#include "spark_wiring_vector.h"

#if Wiring_WiFi == 1

#include "wlan_hal.h"

namespace particle {

namespace control {

namespace wifi {

using namespace particle::control::common;

using wlan_common_function_ptr = decltype(&wlan_scan);

#if PLATFORM_ID != 14 && PLATFORM_ID != 24

int handleGetAntennaRequest(ctrl_request* req) {
    particle_ctrl_WiFiGetAntennaReply reply = {};
    WLanSelectAntenna_TypeDef ant = wlan_get_antenna(nullptr);
    // Re-map
    switch (ant) {
        case ANT_INTERNAL:
            reply.antenna = particle_ctrl_WiFiAntenna_INTERNAL;
        break;
        case ANT_EXTERNAL:
            reply.antenna = particle_ctrl_WiFiAntenna_EXTERNAL;
        break;
        case ANT_AUTO:
            reply.antenna = particle_ctrl_WiFiAntenna_AUTO;
        break;
    }
    return encodeReplyMessage(req, particle_ctrl_WiFiGetAntennaReply_fields, &reply);
}

int handleSetAntennaRequest(ctrl_request* req) {
    particle_ctrl_WiFiSetAntennaRequest request = {};
    int r = decodeRequestMessage(req, particle_ctrl_WiFiSetAntennaRequest_fields, &request);
    WLanSelectAntenna_TypeDef ant = ANT_NONE;
    if (r == SYSTEM_ERROR_NONE) {
        // Re-map
        switch (request.antenna) {
            case particle_ctrl_WiFiAntenna_INTERNAL:
                ant = ANT_INTERNAL;
            break;
            case particle_ctrl_WiFiAntenna_EXTERNAL:
                ant = ANT_EXTERNAL;
            break;
            case particle_ctrl_WiFiAntenna_AUTO:
                ant = ANT_AUTO;
            break;
        }
        if (ant != ANT_NONE) {
            r = wlan_select_antenna(ant) == 0 ? SYSTEM_ERROR_NONE : SYSTEM_ERROR_UNKNOWN;
        } else {
            r = SYSTEM_ERROR_INVALID_ARGUMENT;
        }
    }
    return r;
}

template<wlan_common_function_ptr f>
int handleCommonCredentialsRequest(ctrl_request* req) {
    spark::Vector<particle_ctrl_WiFiAccessPoint> aps;
    int ret = f([](WiFiAccessPoint* sap, void* ptr) {
        spark::Vector<particle_ctrl_WiFiAccessPoint>& aps = *static_cast<spark::Vector<particle_ctrl_WiFiAccessPoint>*>(ptr);
        if (aps.append(particle_ctrl_WiFiAccessPoint())) {
            particle_ctrl_WiFiAccessPoint& ap = aps.last();
            strncpy(ap.ssid, sap->ssid, sizeof(sap->ssid) - 1);
            memcpy(ap.bssid, sap->bssid, sizeof(sap->bssid));
            ap.security = (particle_ctrl_WiFiSecurityType)sap->security;
            ap.cipher = (particle_ctrl_WiFiSecurityCipher)sap->cipher;
            ap.channel = sap->channel;
            ap.max_data_rate = sap->maxDataRate;
            ap.rssi = sap->rssi;
        }
    }, &aps);
    if (ret >= 0) {
        const auto aplistEncoder = [](pb_ostream_t* stream, const pb_field_t* field, void* const* arg) -> bool {
            const spark::Vector<particle_ctrl_WiFiAccessPoint>& aps = *static_cast<const spark::Vector<particle_ctrl_WiFiAccessPoint>*>(*arg);
            for (const auto& ap: aps) {
                // Encode tag
                if (!pb_encode_tag_for_field(stream, field)) {
                    return false;
                }

                if (!pb_encode_submessage(stream, particle_ctrl_WiFiAccessPoint_fields, &ap)) {
                    return false;
                }
            }

            return true;
        };

        if (req->type == CTRL_REQUEST_WIFI_SCAN) {
            particle_ctrl_WiFiScanReply reply = {};
            reply.list.aps.arg = (void*)&aps;
            reply.list.aps.funcs.encode = aplistEncoder;
            ret = encodeReplyMessage(req, particle_ctrl_WiFiScanReply_fields, &reply);
        } else {
            particle_ctrl_WiFiGetCredentialsReply reply = {};
            reply.list.aps.arg = (void*)&aps;
            reply.list.aps.funcs.encode = aplistEncoder;
            ret = encodeReplyMessage(req, particle_ctrl_WiFiGetCredentialsReply_fields, &reply);
        }
    } else {
        ret = SYSTEM_ERROR_UNKNOWN;
    }
    return ret;
}

int handleScanRequest(ctrl_request* req) {
    return handleCommonCredentialsRequest<&wlan_scan>(req);
}

int handleGetCredentialsRequest(ctrl_request* req) {
    return handleCommonCredentialsRequest<&wlan_get_credentials>(req);
}

int handleSetCredentialsRequest(ctrl_request* req) {
    particle_ctrl_WiFiSetCredentialsRequest request = {};

    WLanCredentials credentials = {};
    credentials.size = sizeof(credentials);
    credentials.version = WLAN_CREDENTIALS_CURRENT_VERSION;

    ProtoDecodeBytesLengthHelper<unsigned> h1(&request.ap.password, (const void**)&credentials.password, &credentials.password_len);
    ProtoDecodeBytesLengthHelper<uint16_t> h2(&request.ap.inner_identity, (const void**)&credentials.inner_identity, &credentials.inner_identity_len);
    ProtoDecodeBytesLengthHelper<uint16_t> h3(&request.ap.outer_identity, (const void**)&credentials.outer_identity, &credentials.outer_identity_len);
    ProtoDecodeBytesLengthHelper<uint16_t> h4(&request.ap.private_key, (const void**)&credentials.private_key, &credentials.private_key_len);
    ProtoDecodeBytesLengthHelper<uint16_t> h5(&request.ap.client_certificate, (const void**)&credentials.client_certificate, &credentials.client_certificate_len);
    ProtoDecodeBytesLengthHelper<uint16_t> h6(&request.ap.ca_certificate, (const void**)&credentials.ca_certificate, &credentials.ca_certificate_len);

    int r = decodeRequestMessage(req, particle_ctrl_WiFiSetCredentialsRequest_fields, &request);

    credentials.ssid = request.ap.ssid;
    credentials.ssid_len = strnlen(request.ap.ssid, sizeof(request.ap.ssid) - 1);
    credentials.security = static_cast<WLanSecurityType>(request.ap.security);
    credentials.cipher = static_cast<WLanSecurityCipher>(request.ap.cipher);
    credentials.channel = request.ap.channel;
    credentials.eap_type = static_cast<WLanEapType>(request.ap.eap_type);

    if (r == SYSTEM_ERROR_NONE) {
        r = wlan_set_credentials(&credentials) == 0 ? SYSTEM_ERROR_NONE : SYSTEM_ERROR_UNKNOWN;
    }
    return r;
}

int handleClearCredentialsRequest(ctrl_request* req) {
    return wlan_clear_credentials() == 0 ? SYSTEM_ERROR_NONE : SYSTEM_ERROR_UNKNOWN;
}

#else // PLATFORM_ID == 14 || PLATFORM_ID == 24

// TODO
int handleGetAntennaRequest(ctrl_request* req) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int handleSetAntennaRequest(ctrl_request* req) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int handleScanRequest(ctrl_request* req) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int handleGetCredentialsRequest(ctrl_request* req) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int handleSetCredentialsRequest(ctrl_request* req) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int handleClearCredentialsRequest(ctrl_request* req) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

#endif

} // particle::control::wifi

} // particle::control

} // particle

#endif // Wiring_WiFi == 1

#endif // SYSTEM_CONTROL_ENABLED && !HAL_PLATFORM_NCP
