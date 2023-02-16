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

#pragma once

#include "system_control.h"
#include "control/network.pb.h"
#include "spark_wiring_vector.h"
#include "system_network_configuration.h"
#include <pb.h>
#include <pb_encode.h>
#include <pb_decode.h>

namespace particle {
namespace control {
namespace network {

int getInterfaceList(ctrl_request* req);
int getInterface(ctrl_request* req);

// Helper methods/classes

#if HAL_USE_SOCKET_HAL_POSIX

int ip4AddressToSockAddr(particle_ctrl_Ipv4Address* addr, sockaddr* saddr);
int ip6AddressToSockAddr(particle_ctrl_Ipv6Address* addr, sockaddr* saddr);
int ipAddressToSockAddr(particle_ctrl_IpAddress* addr, sockaddr* saddr);
int sockAddrToIp4Address(const sockaddr* saddr, particle_ctrl_Ipv4Address* addr);
int sockAddrToIp6Address(const sockaddr* saddr, particle_ctrl_Ipv6Address* addr);
int sockaddrToIpAddress(const sockaddr* saddr, particle_ctrl_IpAddress* addr);

struct DecodeInterfaceAddressList {
    DecodeInterfaceAddressList(pb_callback_t* cb) {
        cb->arg = this;
        cb->funcs.decode = [](pb_istream_t* strm, const pb_field_iter_t* field, void** arg) {
            particle_ctrl_InterfaceAddress pbInterfaceAddr = {};
            if (!pb_decode_noinit(strm, particle_ctrl_InterfaceAddress_fields, &pbInterfaceAddr)) {
                return false;
            }
            SockAddr saddr;
            if (ipAddressToSockAddr(&pbInterfaceAddr.address, saddr.toRaw())) {
                return false;
            }
            auto self = (DecodeInterfaceAddressList*)*arg;
            return self->addresses.append(NetworkInterfaceAddress(saddr, pbInterfaceAddr.prefix_length));
        };
    }

    spark::Vector<NetworkInterfaceAddress> addresses;
};

struct DecodeIpv4AddressList {
    DecodeIpv4AddressList(pb_callback_t* cb) {
        cb->arg = this;
        cb->funcs.decode = [](pb_istream_t* strm, const pb_field_iter_t* field, void** arg) {
            particle_ctrl_Ipv4Address pbAddr = {};
            if (!pb_decode_noinit(strm, particle_ctrl_Ipv4Address_fields, &pbAddr)) {
                return false;
            }
            auto self = (DecodeIpv4AddressList*)*arg;
            SockAddr saddr;
            if (ip4AddressToSockAddr(&pbAddr, saddr.toRaw())) {
                return false;
            }
            return self->addresses.append(saddr);
        };
    }

    spark::Vector<SockAddr> addresses;
};

struct DecodeIpv6AddressList {
    DecodeIpv6AddressList(pb_callback_t* cb) {
        cb->arg = this;
        cb->funcs.decode = [](pb_istream_t* strm, const pb_field_iter_t* field, void** arg) {
            particle_ctrl_Ipv6Address pbAddr = {};
            if (!pb_decode_noinit(strm, particle_ctrl_Ipv6Address_fields, &pbAddr)) {
                return false;
            }
            auto self = (DecodeIpv6AddressList*)*arg;
            SockAddr saddr;
            if (ip6AddressToSockAddr(&pbAddr, saddr.toRaw())) {
                return false;
            }
            return self->addresses.append(saddr);
        };
    }

    spark::Vector<SockAddr> addresses;
};

struct EncodeInterfaceAddressList {
    EncodeInterfaceAddressList(pb_callback_t* cb, const spark::Vector<NetworkInterfaceAddress>& addrs) {
        cb->arg = (void*)&addrs;
        cb->funcs.encode = [](pb_ostream_t* strm, const pb_field_iter_t* field, void* const* arg) {
            const auto addrs = (const spark::Vector<NetworkInterfaceAddress>*)(*arg);
            for (const auto& addr: *addrs) {
                particle_ctrl_InterfaceAddress pbInterfaceAddr = {};
                if (sockaddrToIpAddress(addr.address().toRaw(), &pbInterfaceAddr.address)) {
                    return false;
                }
                pbInterfaceAddr.prefix_length = addr.prefixLength();
                if (!pb_encode_tag_for_field(strm, field)) {
                    return false;
                }
                if (!pb_encode_submessage(strm, particle_ctrl_InterfaceAddress_fields, &pbInterfaceAddr)) {
                    return false;
                }
            }
            return true;
        };
    }
};

struct EncodeIp4AddressList {
    EncodeIp4AddressList(pb_callback_t* cb, const spark::Vector<SockAddr>& addrs) {
        cb->arg = (void*)&addrs;
        cb->funcs.encode = [](pb_ostream_t* strm, const pb_field_iter_t* field, void* const* arg) {
            const auto addrs = (const spark::Vector<SockAddr>*)(*arg);
            for (const auto& addr: *addrs) {
                particle_ctrl_Ipv4Address pbAddr = {};
                if (sockAddrToIp4Address(addr.toRaw(), &pbAddr)) {
                    return false;
                }
                if (!pb_encode_tag_for_field(strm, field)) {
                    return false;
                }
                if (!pb_encode_submessage(strm, particle_ctrl_Ipv4Address_fields, &pbAddr)) {
                    return false;
                }
            }
            return true;
        };
    }
};

struct EncodeIp6AddressList {
    EncodeIp6AddressList(pb_callback_t* cb, const spark::Vector<SockAddr>& addrs) {
        cb->arg = (void*)&addrs;
        cb->funcs.encode = [](pb_ostream_t* strm, const pb_field_iter_t* field, void* const* arg) {
            const auto addrs = (const spark::Vector<SockAddr>*)(*arg);
            for (const auto& addr: *addrs) {
                particle_ctrl_Ipv6Address pbAddr = {};
                if (sockAddrToIp6Address(addr.toRaw(), &pbAddr)) {
                    return false;
                }
                if (!pb_encode_tag_for_field(strm, field)) {
                    return false;
                }
                if (!pb_encode_submessage(strm, particle_ctrl_Ipv6Address_fields, &pbAddr)) {
                    return false;
                }
            }
            return true;
        };
    }
};

#endif // HAL_USE_SOCKET_HAL_POSIX


} } } /* namespace particle::control::network */
