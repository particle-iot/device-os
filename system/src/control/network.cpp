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

#include "hal_platform.h"

#if HAL_USE_SOCKET_HAL_POSIX && HAL_PLATFORM_IFAPI

#include "common.h"
#include "socket_hal_posix.h"
#include "ifapi.h"
#include "endian_util.h"
#include "str_util.h"
#include "scope_guard.h"
#include "check.h"
#include "debug.h"

#include "network.pb.h"

#include <algorithm>

#define PB(_name) particle_ctrl_##_name

namespace particle {
namespace control {
namespace network {

namespace {

using namespace particle::control::common;

int sockaddrToIpAddress(const sockaddr* saddr, PB(IpAddress)* addr) {
    if (saddr->sa_family == AF_INET) {
        const auto inaddr = (const sockaddr_in*)saddr;
        addr->which_address = PB(IpAddress_v4_tag);
        static_assert(sizeof(addr->address.v4.address) == sizeof(inaddr->sin_addr), "");
        memcpy(&addr->address.v4.address, &inaddr->sin_addr, sizeof(inaddr->sin_addr));
        addr->address.v4.address = bigEndianToNative(addr->address.v4.address);
    } else if (saddr->sa_family == AF_INET6) {
        const auto inaddr = (const sockaddr_in6*)saddr;
        addr->which_address = PB(IpAddress_v6_tag);
        static_assert(sizeof(addr->address.v6.address) == sizeof(inaddr->sin6_addr), "");
        memcpy(addr->address.v6.address, &inaddr->sin6_addr, sizeof(inaddr->sin6_addr));
    } else {
        return SYSTEM_ERROR_UNKNOWN;
    }
    return 0;
}

PB(InterfaceType) ifaceTypeFromName(const char* name) {
    if (startsWith(name, "lo")) {
        return PB(InterfaceType_LOOPBACK);
    } else if (startsWith(name, "th")) {
        return PB(InterfaceType_THREAD);
    } else if (startsWith(name, "en")) {
        return PB(InterfaceType_ETHERNET);
    } else if (startsWith(name, "wl")) {
        return PB(InterfaceType_WIFI);
    } else if (startsWith(name, "pp")) {
        return PB(InterfaceType_PPP);
    } else {
        return PB(InterfaceType_INVALID_INTERFACE_TYPE);
    }
}

} // particle::control::network::

int getInterfaceList(ctrl_request* req) {
    if_list* ifList = nullptr;
    CHECK(if_get_list(&ifList));
    SCOPE_GUARD({
        if_free_list(ifList);
    });
    PB(GetInterfaceListReply) pbRep = {};
    pbRep.interfaces.arg = ifList;
    pbRep.interfaces.funcs.encode = [](pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
        const auto ifList = (if_list*)*arg;
        for (if_list* i = ifList; i; i = i->next) {
            uint8_t index = 0;
            if (if_get_index(i->iface, &index) != 0) {
                return false;
            }
            char name[IF_NAMESIZE] = {};
            if (if_get_name(i->iface, name) != 0) {
                return false;
            }
            const auto type = ifaceTypeFromName(name);
            if (type == PB(InterfaceType_INVALID_INTERFACE_TYPE)) {
                return false;
            }
            PB(InterfaceEntry) pbIface = {};
            EncodedString eName(&pbIface.name, name, strlen(name));
            pbIface.index = index;
            pbIface.type = type;
            if (!pb_encode_tag_for_field(strm, field)) {
                return false;
            }
            if (!pb_encode_submessage(strm, PB(InterfaceEntry_fields), &pbIface)) {
                return false;
            }
        }
        return true;
    };
    CHECK(encodeReplyMessage(req, PB(GetInterfaceListReply_fields), &pbRep));
    return 0;
}

int getInterface(ctrl_request* req) {
    PB(GetInterfaceRequest) pbReq = {};
    CHECK(decodeRequestMessage(req, PB(GetInterfaceRequest_fields), &pbReq));
    if_t iface = nullptr;
    CHECK(if_get_by_index(pbReq.index, &iface));
    SPARK_ASSERT(iface);
    PB(GetInterfaceReply) pbRep = {};
    auto& pbIface = pbRep.interface;
    // Index
    pbIface.index = pbReq.index;
    // Name
    char name[IF_NAMESIZE] = {};
    CHECK(if_get_name(iface, name));
    EncodedString eName(&pbIface.name, name, strlen(name));
    // Type
    pbIface.type = ifaceTypeFromName(name);
    if (pbIface.type == PB(InterfaceType_INVALID_INTERFACE_TYPE)) {
        return SYSTEM_ERROR_UNKNOWN;
    }
    // Flags
    unsigned val = 0;
    CHECK(if_get_flags(iface, &val));
    pbIface.flags = val;
    // Extended flags
    CHECK(if_get_xflags(iface, &val));
    pbIface.ext_flags = val;
    // MTU
    CHECK(if_get_mtu(iface, &val));
    pbIface.mtu = val;
    // Metric
    CHECK(if_get_metric(iface, &val));
    pbIface.metric = val;
    // Hardware address
    sockaddr_ll haddr = {};
    CHECK(if_get_lladdr(iface, &haddr));
    pbIface.hw_address.size = std::min((size_t)haddr.sll_halen, sizeof(pbIface.hw_address.bytes));
    memcpy(pbIface.hw_address.bytes, haddr.sll_addr, pbIface.hw_address.size);
    // IPv4 config
    if_addrs* ifAddrList = nullptr;
    CHECK(if_get_addrs(iface, &ifAddrList));
    SCOPE_GUARD({
        if_free_if_addrs(ifAddrList);
    });
    auto& pbIpv4Config = pbIface.ipv4_config;
    pbIpv4Config.addresses.arg = ifAddrList;
    pbIpv4Config.addresses.funcs.encode = [](pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
        const auto ifAddrList = (if_addrs*)*arg;
        for (if_addrs* i = ifAddrList; i; i = i->next) {
            const auto ifAddr = i->if_addr;
            if (ifAddr->addr->sa_family != AF_INET) { // Skip non-IPv4 addresses
                continue;
            }
            PB(InterfaceAddress) pbIfAddr = {};
            if (sockaddrToIpAddress(ifAddr->addr, &pbIfAddr.address) != 0) {
                return false;
            }
            pbIfAddr.prefix_length = ifAddr->prefixlen;
            if (!pb_encode_tag_for_field(strm, field)) {
                return false;
            }
            if (!pb_encode_submessage(strm, PB(InterfaceAddress_fields), &pbIfAddr)) {
                return false;
            }
        }
        return true;
    };
    // IPv6 config
    auto& pbIpv6Config = pbIface.ipv6_config;
    pbIpv6Config.addresses.arg = ifAddrList;
    pbIpv6Config.addresses.funcs.encode = [](pb_ostream_t* strm, const pb_field_t* field, void* const* arg) {
        const auto ifAddrList = (if_addrs*)*arg;
        for (if_addrs* i = ifAddrList; i; i = i->next) {
            const auto ifAddr = i->if_addr;
            if (ifAddr->addr->sa_family != AF_INET6) { // Skip non-IPv6 addresses
                continue;
            }
            PB(InterfaceAddress) pbIfAddr = {};
            if (sockaddrToIpAddress(ifAddr->addr, &pbIfAddr.address) != 0) {
                return false;
            }
            pbIfAddr.prefix_length = ifAddr->prefixlen;
            if (!pb_encode_tag_for_field(strm, field)) {
                return false;
            }
            if (!pb_encode_submessage(strm, PB(InterfaceAddress_fields), &pbIfAddr)) {
                return false;
            }
        }
        return true;
    };
    // Encode a reply
    CHECK(encodeReplyMessage(req, PB(GetInterfaceReply_fields), &pbRep));
    return 0;
}

} } } /* namespace particle::control::network */

#endif // HAL_USE_SOCKET_HAL_POSIX && HAL_PLATFORM_IFAPI

#endif /* #if SYSTEM_CONTROL_ENABLED */
