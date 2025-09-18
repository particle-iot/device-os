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

#include "wifi_new.h"
#include "hal_platform.h"

#if SYSTEM_CONTROL_ENABLED && HAL_PLATFORM_NCP && HAL_PLATFORM_WIFI

#include "common.h"

#include "network/ncp/wifi/ncp.h"
#include "network/ncp/wifi/wifi_network_manager.h"
#include "network/ncp/wifi/wifi_ncp_client.h"
#include "system_network.h"

#include "scope_guard.h"
#include "check.h"

#include "spark_wiring_vector.h"

#include "control/wifi_new.pb.h"
#include "system_event.h"
#include "delay_hal.h"

#define PB(_name) particle_ctrl_wifi_##_name

namespace particle {

namespace ctrl {

namespace wifi {

namespace {

using namespace particle::control::common;
using spark::Vector;

template<typename T>
void bssidToPb(const MacAddress& bssid, T* pbBssid) {
    if (bssid != INVALID_MAC_ADDRESS) {
        memcpy(pbBssid->bytes, &bssid, MAC_ADDRESS_SIZE);
        pbBssid->size = MAC_ADDRESS_SIZE;
    }
}

template<typename T>
void bssidFromPb(MacAddress* bssid, const T& pbBssid) {
    if (pbBssid.size == MAC_ADDRESS_SIZE) {
        memcpy(bssid, pbBssid.bytes, MAC_ADDRESS_SIZE);
    }
}

void network_credentials_cleanup(void* ptr) {
    auto creds = static_cast<NetworkCredentials*>(ptr);
    free(creds);
}

NetworkCredentials* alloc_network_credentials_deep_copy(WifiNetworkConfig conf)
{
    const size_t ssid_bytes = conf.ssid() ? (strlen(conf.ssid()) + 1) : 0;
    const size_t pass_bytes = conf.credentials().password() ? (strlen(conf.credentials().password()) + 1) : 0;
    const size_t total = sizeof(NetworkCredentials) + ssid_bytes + pass_bytes;

    unsigned char* mem = static_cast<unsigned char*>(malloc(total));
    if (!mem) {
        return nullptr;
    }

    auto creds = reinterpret_cast<NetworkCredentials*>(mem);
    memset(creds, 0, sizeof(*creds));

    unsigned char* p = mem + sizeof(NetworkCredentials);

    if (ssid_bytes) {
        char* ssid_buf = reinterpret_cast<char*>(p);
        memcpy(ssid_buf, conf.ssid(), ssid_bytes);
        creds->ssid = ssid_buf;
        creds->ssid_len = static_cast<unsigned>(ssid_bytes - 1);
        p += ssid_bytes;
    }

    if (pass_bytes) {
        char* pass_buf = reinterpret_cast<char*>(p);
        memcpy(pass_buf, conf.credentials().password(), pass_bytes);
        creds->password = pass_buf;
        creds->password_len = static_cast<unsigned>(pass_bytes - 1);
        p += pass_bytes;
    }

    creds->security = fromWifiSecurity(conf.security());
    creds->size = sizeof(NetworkCredentials);

    return creds;
}

} // unnamed

int joinNewNetwork(ctrl_request* req) {
    PB(JoinNewNetworkRequest) pbReq = {};
    DecodedCString dSsid(&pbReq.ssid);
    DecodedCString dPwd(&pbReq.credentials.password);
    CHECK(decodeRequestMessage(req, PB(JoinNewNetworkRequest_fields), &pbReq));
    // Parse new network configuration
    if (pbReq.credentials.type != PB(CredentialsType_NO_CREDENTIALS) &&
            pbReq.credentials.type != PB(CredentialsType_PASSWORD)) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    WifiCredentials cred;
    cred.type((WifiCredentials::Type)pbReq.credentials.type);
    if (pbReq.credentials.type == PB(CredentialsType_PASSWORD)) {
        cred.password(dPwd.data);
    }
    WifiNetworkConfig conf;
    conf.ssid(dSsid.data);
    MacAddress bssid = INVALID_MAC_ADDRESS;
    bssidFromPb(&bssid, pbReq.bssid);
    conf.bssid(bssid);
    conf.hidden(pbReq.hidden);
    const auto wifiMgr = wifiNetworkManager();
    CHECK_TRUE(wifiMgr, SYSTEM_ERROR_UNKNOWN);
    const auto ncpClient = wifiMgr->ncpClient();
    CHECK_TRUE(ncpClient, SYSTEM_ERROR_UNKNOWN);
    const NcpClientLock lock(ncpClient);
#if HAL_PLATFORM_RTL872X
    if (!conf.hidden()) {
        // scan checks that the NcpState is ON
        CHECK(ncpClient->on());
        // Scan for networks to detect the network security type
        Vector<WifiScanResult> networks;
        CHECK(ncpClient->scan([](WifiScanResult network, void* data) -> int {
            const auto networks = (Vector<WifiScanResult>*)data;
            CHECK_TRUE(networks->append(std::move(network)), SYSTEM_ERROR_NO_MEMORY);
            return 0;
        }, &networks));
        for (auto network : networks) {
            if (!strcmp(dSsid.data, network.ssid())) {
                conf.security((WifiSecurity)network.security());
                break;
            }
        }    
    } 

    // If the network is hidden, or the scan does not find it, use the security provided in the control request
    if (conf.hidden() || conf.security() == WifiSecurity::NONE) {
        conf.security((WifiSecurity)pbReq.security);    
    }
#else
    conf.security((WifiSecurity)pbReq.security);
#endif
    conf.credentials(std::move(cred));
    // Connect to the network
    CHECK(ncpClient->on());
    // FIXME: synchronize NCP client / NcpNetif and system network manager state
    bool needToConnect = network_connecting(NETWORK_INTERFACE_WIFI_STA, 0, nullptr) ||
            network_ready(NETWORK_INTERFACE_WIFI_STA, NETWORK_READY_TYPE_ANY, nullptr);
    // To unblock
    ncpClient->disable();
    CHECK(ncpClient->enable());
    // These two are in sync now
    ncpClient->disconnect(); // ignore the error
    network_disconnect(NETWORK_INTERFACE_WIFI_STA, NETWORK_DISCONNECT_REASON_USER, nullptr);
    // FIXME: We are wiating for ncpNetif to potentially fully disconnect
    // FIXME: synchronize NCP client / NcpNetif and system network manager state
    CHECK(ncpClient->enable());
    CHECK(ncpClient->on());
    // Set new configuration
    CHECK(wifiMgr->setNetworkConfig(conf, WifiNetworkConfigFlag::VALIDATE));
    // Deep copy credentials and pass object to system_notify_event with a cleanup function
    auto creds = alloc_network_credentials_deep_copy(conf);

    if (creds) {
        system_notify_event(
            network_credentials,
            network_credentials_added,
            creds,
            network_credentials_cleanup,
            creds
        );
    }

    network_connect(NETWORK_INTERFACE_WIFI_STA, NETWORK_CONNECT_FLAG_FORCE, 0, nullptr);
    NAMED_SCOPE_GUARD(networkDisconnectGuard, {
        // FIXME: synchronize NCP client / NcpNetif and system network manager state
        if (!needToConnect) {
            network_disconnect(NETWORK_INTERFACE_WIFI_STA, NETWORK_DISCONNECT_REASON_USER, nullptr);
        }
    });
    networkDisconnectGuard.dismiss();
    return 0;
}

int joinKnownNetwork(ctrl_request* req) {
    PB(JoinKnownNetworkRequest) pbReq = {};
    DecodedCString dSsid(&pbReq.ssid);
    CHECK(decodeRequestMessage(req, PB(JoinKnownNetworkRequest_fields), &pbReq));
    const auto wifiMgr = wifiNetworkManager();
    CHECK_TRUE(wifiMgr, SYSTEM_ERROR_UNKNOWN);
    const auto ncpClient = wifiMgr->ncpClient();
    CHECK_TRUE(ncpClient, SYSTEM_ERROR_UNKNOWN);
    const NcpClientLock lock(ncpClient);
    CHECK(ncpClient->on());
    // FIXME: synchronize NCP client / NcpNetif and system network manager state
    network_disconnect(NETWORK_INTERFACE_WIFI_STA, NETWORK_DISCONNECT_REASON_USER, nullptr);
    CHECK(ncpClient->disconnect());
    // FIXME: synchronize NCP client / NcpNetif and system network manager state
    network_connect(NETWORK_INTERFACE_WIFI_STA, 0, 0, nullptr);
    NAMED_SCOPE_GUARD(networkDisconnectGuard, {
        // FIXME: synchronize NCP client / NcpNetif and system network manager state
        network_disconnect(NETWORK_INTERFACE_WIFI_STA, NETWORK_DISCONNECT_REASON_USER, nullptr);
    });
    CHECK(wifiMgr->connect(dSsid.data));
    networkDisconnectGuard.dismiss();
    return 0;
}

int getKnownNetworks(ctrl_request* req) {
    const auto wifiMgr = wifiNetworkManager();
    CHECK_TRUE(wifiMgr, SYSTEM_ERROR_UNKNOWN);
    // Enumerate configured networks
    Vector<WifiNetworkConfig> networks;
    CHECK(wifiMgr->getNetworkConfig([](WifiNetworkConfig conf, void* data) -> int {
        const auto networks = (Vector<WifiNetworkConfig>*)data;
        CHECK_TRUE(networks->append(std::move(conf)), SYSTEM_ERROR_NO_MEMORY);
        return 0;
    }, &networks));
    // Encode a reply
    PB(GetKnownNetworksReply) pbRep = {};
    pbRep.networks.arg = &networks;
    pbRep.networks.funcs.encode = [](pb_ostream_t* strm, const pb_field_iter_t* field, void* const* arg) {
        const auto networks = (const Vector<WifiNetworkConfig>*)*arg;
        for (const WifiNetworkConfig& conf: *networks) {
            PB(GetKnownNetworksReply_Network) pbConf = {};
            EncodedString eSsid(&pbConf.ssid, conf.ssid(), strlen(conf.ssid()));
            pbConf.security = (PB(Security))conf.security();
            pbConf.credentials_type = (PB(CredentialsType))conf.credentials().type();
            if (!pb_encode_tag_for_field(strm, field)) {
                return false;
            }
            if (!pb_encode_submessage(strm, PB(GetKnownNetworksReply_Network_fields), &pbConf)) {
                return false;
            }
        }
        return true;
    };
    CHECK(encodeReplyMessage(req, PB(GetKnownNetworksReply_fields), &pbRep));
    return 0;
}

int removeKnownNetwork(ctrl_request* req) {
    PB(RemoveKnownNetworkRequest) pbReq = {};
    DecodedCString dSsid(&pbReq.ssid);
    CHECK(decodeRequestMessage(req, PB(RemoveKnownNetworkRequest_fields), &pbReq));
    const auto wifiMgr = wifiNetworkManager();
    CHECK_TRUE(wifiMgr, SYSTEM_ERROR_UNKNOWN);
    wifiMgr->removeNetworkConfig(dSsid.data);
    return 0;
}

int clearKnownNetworks(ctrl_request* req) {
    const auto wifiMgr = wifiNetworkManager();
    CHECK_TRUE(wifiMgr, SYSTEM_ERROR_UNKNOWN);
    wifiMgr->clearNetworkConfig();
    return 0;
}

int getCurrentNetwork(ctrl_request* req) {
    const auto wifiMgr = wifiNetworkManager();
    CHECK_TRUE(wifiMgr, SYSTEM_ERROR_UNKNOWN);
    const auto ncpClient = wifiMgr->ncpClient();
    CHECK_TRUE(ncpClient, SYSTEM_ERROR_UNKNOWN);
    WifiNetworkInfo info;
    CHECK(ncpClient->getNetworkInfo(&info));
    // Encode a reply
    PB(GetCurrentNetworkReply) pbRep = {};
    EncodedString eSsid(&pbRep.ssid, info.ssid(), strlen(info.ssid()));
    bssidToPb(info.bssid(), &pbRep.bssid);
    pbRep.channel = info.channel();
    pbRep.rssi = info.rssi();
    CHECK(encodeReplyMessage(req, PB(GetCurrentNetworkReply_fields), &pbRep));
    return 0;
}

int scanNetworks(ctrl_request* req) {
    const auto wifiMgr = wifiNetworkManager();
    CHECK_TRUE(wifiMgr, SYSTEM_ERROR_UNKNOWN);
    const auto ncpClient = wifiMgr->ncpClient();
    CHECK_TRUE(ncpClient, SYSTEM_ERROR_UNKNOWN);
    const NcpClientLock lock(ncpClient);
    CHECK(ncpClient->on());
    // Scan for networks
    Vector<WifiScanResult> networks;
    CHECK(ncpClient->scan([](WifiScanResult network, void* data) -> int {
        const auto networks = (Vector<WifiScanResult>*)data;
        CHECK_TRUE(networks->append(std::move(network)), SYSTEM_ERROR_NO_MEMORY);
        return 0;
    }, &networks));
    // Encode a reply
    PB(ScanNetworksReply) pbRep = {};
    pbRep.networks.arg = &networks;
    pbRep.networks.funcs.encode = [](pb_ostream_t* strm, const pb_field_iter_t* field, void* const* arg) {
        const auto networks = (const Vector<WifiScanResult>*)*arg;
        for (const WifiScanResult& network: *networks) {
            PB(ScanNetworksReply_Network) pbNetwork = {};
            EncodedString eSsid(&pbNetwork.ssid, network.ssid(), strlen(network.ssid()));
            bssidToPb(network.bssid(), &pbNetwork.bssid);
            pbNetwork.security = (PB(Security))network.security();
            pbNetwork.channel = network.channel();
            pbNetwork.rssi = network.rssi();
            if (!pb_encode_tag_for_field(strm, field)) {
                return false;
            }
            if (!pb_encode_submessage(strm, PB(ScanNetworksReply_Network_fields), &pbNetwork)) {
                return false;
            }
        }
        return true;
    };
    CHECK(encodeReplyMessage(req, PB(ScanNetworksReply_fields), &pbRep));
    return 0;
}

int setNetworkCredentials(ctrl_request* req) {
    PB(SetNetworkCredentialsRequest) pbReq = {};
    DecodedCString dSsid(&pbReq.ssid);
    DecodedCString dPwd(&pbReq.credentials.password);
    CHECK(decodeRequestMessage(req, PB(SetNetworkCredentialsRequest_fields), &pbReq));
    // Parse new network configuration
    if (pbReq.credentials.type != PB(CredentialsType_NO_CREDENTIALS) &&
            pbReq.credentials.type != PB(CredentialsType_PASSWORD)) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    WifiCredentials cred;
    cred.type((WifiCredentials::Type)pbReq.credentials.type);
    if (pbReq.credentials.type == PB(CredentialsType_PASSWORD)) {
        cred.password(dPwd.data);
    }
    WifiNetworkConfig conf;
    conf.ssid(dSsid.data);
    MacAddress bssid = INVALID_MAC_ADDRESS;
    bssidFromPb(&bssid, pbReq.bssid);
    conf.bssid(bssid);
    conf.hidden(pbReq.hidden);
    conf.security((WifiSecurity)pbReq.security);
    conf.credentials(std::move(cred));

    const auto wifiMgr = wifiNetworkManager();
    CHECK_TRUE(wifiMgr, SYSTEM_ERROR_UNKNOWN);
    CHECK(wifiMgr->setNetworkConfig(conf, WifiNetworkConfigFlag::NONE));
    // Deep copy credentials and pass object to system_notify_event with a cleanup function
    auto creds = alloc_network_credentials_deep_copy(conf);

    if (creds) {
        system_notify_event(
            network_credentials,
            network_credentials_added,
            creds,
            network_credentials_cleanup,
            creds
        );
    }

    return 0;
}

} // particle::ctrl::wifi

} // particle::ctrl

} // particle

#endif // SYSTEM_CONTROL_ENABLED && HAL_PLATFORM_NCP && HAL_PLATFORM_WIFI
