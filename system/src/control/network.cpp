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

#include "network.h"

#if SYSTEM_CONTROL_ENABLED

#include "common.h"
#include "network.pb.h"
#include "spark_wiring_platform.h"

#if Wiring_WiFi == 1
#include "wlan_hal.h"
#endif /* Wiring_WiFi == 1 */

#if Wiring_Cellular == 1
#include "cellular_hal.h"
#endif /* Wiring_WiFi == 1 */

#include "system_network.h"

#include <algorithm>

namespace particle {
namespace control {
namespace network {

using namespace particle::control::common;

int handleGetConfigurationRequest(ctrl_request* req) {
    particle_ctrl_NetworkGetConfigurationRequest request = {};
    int r = decodeRequestMessage(req, particle_ctrl_NetworkGetConfigurationRequest_fields, &request);
    if (r == SYSTEM_ERROR_NONE && request.interface == 1) {
        particle_ctrl_NetworkGetConfigurationReply reply = {};
        auto& conf = reply.config;
        auto& ipconfig = conf.ipconfig;
        auto& dnsconfig = conf.dnsconfig;

        conf.interface = 1;
#if Wiring_WiFi == 1
        IPConfig sipconf = {};

        char hostname[255] = {};
        wlan_get_hostname(hostname, sizeof(hostname), nullptr);
        EncodedString hostnameEncoder(&ipconfig.hostname, hostname, strlen(hostname));

        switch (wlan_get_ipaddress_source(nullptr)) {
            case STATIC_IP:
                ipconfig.type = particle_ctrl_IPConfiguration_Type_STATIC;
                break;
            case DYNAMIC_IP:
                ipconfig.type = particle_ctrl_IPConfiguration_Type_DHCP;
                break;
        }

        if (ipconfig.type == particle_ctrl_IPConfiguration_Type_STATIC) {
            if (wlan_get_ipaddress(&sipconf, nullptr) == 0) {
                protoIpFromHal(&ipconfig.address, &sipconf.nw.aucIP);
                protoIpFromHal(&ipconfig.netmask, &sipconf.nw.aucSubnetMask);
                protoIpFromHal(&ipconfig.gateway, &sipconf.nw.aucDefaultGateway);

                // DNS
                dnsconfig.servers.arg = (void*)&sipconf;
                dnsconfig.servers.funcs.encode = [](pb_ostream_t* stream, const pb_field_t* field, void* const* arg) -> bool {
                    const IPConfig& sipconf = *static_cast<const IPConfig*>(*arg);
                    particle_ctrl_IPAddress ip = {};
                    protoIpFromHal(&ip, &sipconf.nw.aucDNSServer);

                    // Encode tag
                    if (!pb_encode_tag_for_field(stream, field)) {
                        return false;
                    }

                    if (!pb_encode_submessage(stream, particle_ctrl_IPAddress_fields, &ip)) {
                        return false;
                    }

                    return true;
                };
            }
        }
#elif Wiring_Cellular == 1
        ipconfig.type = particle_ctrl_IPConfiguration_Type_DHCP;
        (void)dnsconfig;
#else
        r = SYSTEM_ERROR_NOT_SUPPORTED;
#endif // Wiring_WiFi == 1
        if (r == SYSTEM_ERROR_NONE) {
            r = encodeReplyMessage(req, particle_ctrl_NetworkGetConfigurationReply_fields, &reply);
        }
    }
    return r;
}

int handleGetStatusRequest(ctrl_request* req) {
    particle_ctrl_NetworkGetStatusRequest request = {};
    int r = decodeRequestMessage(req, particle_ctrl_NetworkGetStatusRequest_fields, &request);

    if (r == SYSTEM_ERROR_NONE && request.interface == 1) {
        particle_ctrl_NetworkGetStatusReply reply = {};
        auto& conf = reply.config;
        auto& ipconfig = conf.ipconfig;
        auto& dnsconfig = conf.dnsconfig;

        conf.interface = 1;
        conf.state = network_ready(0, 0, nullptr) ? particle_ctrl_NetworkState_UP : particle_ctrl_NetworkState_DOWN;
#if Wiring_WiFi == 1
        const WLanConfig* wlanconf = static_cast<const WLanConfig*>(network_config(0, 0, nullptr));

        char hostname[255] = {};
        wlan_get_hostname(hostname, sizeof(hostname), nullptr);
        EncodedString hostnameEncoder(&ipconfig.hostname, hostname, strlen(hostname));

        // Running configuration
        if (wlanconf != nullptr && network_ready(0, 0, nullptr)) {
            memcpy(conf.mac.bytes, wlanconf->nw.uaMacAddr, sizeof(wlanconf->nw.uaMacAddr));
            conf.mac.size = sizeof(wlanconf->nw.uaMacAddr);
            protoIpFromHal(&ipconfig.address, &wlanconf->nw.aucIP);
            protoIpFromHal(&ipconfig.netmask, &wlanconf->nw.aucSubnetMask);
            protoIpFromHal(&ipconfig.gateway, &wlanconf->nw.aucDefaultGateway);
            protoIpFromHal(&ipconfig.dhcp_server, &wlanconf->nw.aucDHCPServer);
            // DNS
            dnsconfig.servers.arg = (void*)wlanconf;
            dnsconfig.servers.funcs.encode = [](pb_ostream_t* stream, const pb_field_t* field, void* const* arg) -> bool {
                const WLanConfig& wlanconf = *static_cast<const WLanConfig*>(*arg);
                particle_ctrl_IPAddress ip = {};
                protoIpFromHal(&ip, &wlanconf.nw.aucDNSServer);

                if (!pb_encode_tag_for_field(stream, field)) {
                    return false;
                }

                if (!pb_encode_submessage(stream, particle_ctrl_IPAddress_fields, &ip)) {
                    return false;
                }

                return true;
            };
        }
#elif Wiring_Cellular == 1
        const CellularConfig* cellconf = static_cast<const CellularConfig*>(network_config(0, 0, nullptr));
        conf.state = network_ready(0, 0, nullptr) ? particle_ctrl_NetworkState_UP : particle_ctrl_NetworkState_DOWN;
        (void)dnsconfig;

        if (network_ready(0, 0, nullptr) && cellconf != nullptr) {
            protoIpFromHal(&ipconfig.address, &cellconf->nw.aucIP);
        }
#else
        r = SYSTEM_ERROR_NOT_SUPPORTED;
#endif
        if (r == SYSTEM_ERROR_NONE) {
            r = encodeReplyMessage(req, particle_ctrl_NetworkGetStatusReply_fields, &reply);
        }
    }
    return r;
}

int handleSetConfigurationRequest(ctrl_request* req) {
    int r = SYSTEM_ERROR_NONE;
#if Wiring_WiFi == 1
    particle_ctrl_NetworkSetConfigurationRequest request = {};
    auto& netconf = request.config;

    DecodedString hostname(&netconf.ipconfig.hostname);

    HAL_IPAddress host = {};
    HAL_IPAddress netmask = {};
    HAL_IPAddress gateway = {};

    struct tmp_dns {
        size_t count = 0;
        HAL_IPAddress servers[2] = {};
    } dns;
    netconf.dnsconfig.servers.arg = &dns;
    netconf.dnsconfig.servers.funcs.decode = [](pb_istream_t* stream, const pb_field_t* field, void** arg) -> bool {
        tmp_dns& dns = *static_cast<tmp_dns*>(*arg);
        if (dns.count < 2) {
            particle_ctrl_IPAddress ip = {};
            if (pb_decode_noinit(stream, particle_ctrl_IPAddress_fields, &ip)) {
                halIpFromProto(&ip, &dns.servers[dns.count++]);
                return true;
            }
        }
        return false;
    };

    r = decodeRequestMessage(req, particle_ctrl_NetworkSetConfigurationRequest_fields, &request);

    halIpFromProto(&netconf.ipconfig.address, &host);
    halIpFromProto(&netconf.ipconfig.netmask, &netmask);
    halIpFromProto(&netconf.ipconfig.gateway, &gateway);

    if (r == SYSTEM_ERROR_NONE) {
        if (netconf.ipconfig.type == particle_ctrl_IPConfiguration_Type_DHCP) {
            /* r = */wlan_set_ipaddress_source(DYNAMIC_IP, true, nullptr);
        } else if (netconf.ipconfig.type == particle_ctrl_IPConfiguration_Type_STATIC) {
            /* r = */wlan_set_ipaddress(&host, &netmask, &gateway, &dns.servers[0], &dns.servers[1], nullptr);
            if (r == SYSTEM_ERROR_NONE) {
                /* r = */wlan_set_ipaddress_source(STATIC_IP, true, nullptr);
            }
        }
        if (r == SYSTEM_ERROR_NONE) {
            if (hostname.size >= 0 && hostname.data) {
                char tmp[255] = {};
                memcpy(tmp, hostname.data, std::min(hostname.size, sizeof(tmp) - 1));
                r = wlan_set_hostname(tmp, nullptr);
            }
        }
        if (r != SYSTEM_ERROR_NONE) {
            r = SYSTEM_ERROR_UNKNOWN;
        }
    }
#else
    r = SYSTEM_ERROR_NOT_SUPPORTED;
#endif
    return r;
}

} } } /* namespace particle::control::network */

#endif /* #if SYSTEM_CONTROL_ENABLED */
