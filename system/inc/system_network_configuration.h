/*
 * Copyright (c) 2023 Particle Industries, Inc.  All rights reserved.
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

#include "hal_platform.h"

#if HAL_USE_SOCKET_HAL_POSIX

#include "spark_wiring_ipaddress.h"
#include "socket_hal.h" // For AF_XXX
#include "spark_wiring_vector.h"
#include "spark_wiring_posix_common.h"
#include "scope_guard.h"
#include "check.h"
#include "enumclass.h"
#include "inet_hal.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Must match particle.ctrl.InterfaceConfigurationSource
typedef enum network_configuration_source_t {
    NETWORK_CONFIGURATION_SOURCE_NONE = 0,
    NETWORK_CONFIGURATION_SOURCE_DHCP = 1,
    NETWORK_CONFIGURATION_SOURCE_STATIC = 2,
    NETWORK_CONFIGURATION_SOURCE_SLAAC = 3,
    NETWORK_CONFIGURATION_SOURCE_DHCPV6 = 4
} network_configuration_source_t;

typedef struct sockaddr_list {
    struct sockaddr* addr;
    struct sockaddr* netmask;
    uint8_t prefix_len;
    uint8_t reserved[3];
    void* reserved1;
    struct sockaddr_list* next;
} sockaddr_list;

typedef struct network_configuration_proto_t {
    uint16_t size;
    uint16_t version;

    uint8_t source; // network_configuration_source_t

    sockaddr_list* addrs;
    sockaddr_list* dns;

    struct sockaddr* gateway;
} network_configuration_proto_t;

typedef struct network_configuration_t {
    uint16_t size;
    uint16_t version;

    network_configuration_proto_t* ip4;
    network_configuration_proto_t* ip6;

    char* profile;
    uint16_t profile_len;

    char* hostname; // reserved/unused for now
} network_configuration_t;

#ifdef __cplusplus
} // extern "C"

namespace particle {

// Must match particle.ctrl.InterfaceConfigurationSource
enum class NetworkInterfaceConfigSource {
    NONE = NETWORK_CONFIGURATION_SOURCE_NONE,
    DHCP = NETWORK_CONFIGURATION_SOURCE_DHCP,
    STATIC = NETWORK_CONFIGURATION_SOURCE_STATIC,
    SLAAC = NETWORK_CONFIGURATION_SOURCE_SLAAC,
    DHCPV6 = NETWORK_CONFIGURATION_SOURCE_DHCPV6,
};

class SockAddr {
public:
    SockAddr();
    SockAddr(IPAddress addr, uint16_t port = 0);
    SockAddr(const sockaddr* addr);
    SockAddr(const char* addr);

    IPAddress address() const;
    SockAddr& address(IPAddress addr);
    SockAddr& address(const sockaddr* addr);

    uint16_t port() const;
    SockAddr& port(uint16_t p);

    int family() const;

    sockaddr* toRaw() const;

    operator IPAddress() const;

    bool isAddrAny() const;

    bool operator==(const SockAddr& other) const;
    bool operator!=(const SockAddr& other) const;

    // Some IPAddress-compatible methods
    operator bool() const;
    void clear(int family = AF_UNSPEC);
    String toString() const;

private:
    sockaddr_storage addr_;
};

bool operator==(const IPAddress& iaddr, const SockAddr& saddr);
bool operator!=(const IPAddress& iaddr, const SockAddr& saddr);
bool operator==(const SockAddr& saddr, const IPAddress& iaddr);
bool operator!=(const SockAddr& saddr, const IPAddress& iaddr);

class NetworkInterfaceAddress {
public:
    NetworkInterfaceAddress(SockAddr addr, SockAddr mask);
    NetworkInterfaceAddress(SockAddr addr, uint8_t prefixLen);

    NetworkInterfaceAddress(IPAddress addr, IPAddress mask);
    NetworkInterfaceAddress(IPAddress addr, uint8_t prefixLen);

    SockAddr address() const;
    NetworkInterfaceAddress& address(SockAddr addr, SockAddr mask = SockAddr());
    NetworkInterfaceAddress& address(SockAddr addr, uint8_t prefixLen = 0);
    NetworkInterfaceAddress& address(IPAddress addr, IPAddress mask = IPAddress());
    NetworkInterfaceAddress& address(IPAddress addr, uint8_t prefixLen = 0);

    SockAddr mask() const;
    NetworkInterfaceAddress& mask(SockAddr mask);
    NetworkInterfaceAddress& mask(IPAddress mask);

    uint8_t prefixLength() const;
    NetworkInterfaceAddress& prefixLength(uint8_t len);

    int family() const;

    bool operator==(const NetworkInterfaceAddress& other) const;
    bool operator!=(const NetworkInterfaceAddress& other) const;

protected:
    NetworkInterfaceAddress();

private:
    SockAddr addr_;
    SockAddr mask_;
    uint8_t prefixLen_;
};

class NetworkInterfaceConfig {
public:
    NetworkInterfaceConfig();
    NetworkInterfaceConfig(const network_configuration_t* conf);

    NetworkInterfaceConfig& source(NetworkInterfaceConfigSource source, int family = AF_INET);
    NetworkInterfaceConfigSource source(int family = AF_INET) const;

    NetworkInterfaceConfig& address(const NetworkInterfaceAddress& addr);
    NetworkInterfaceConfig& address(SockAddr addr, SockAddr mask);
    NetworkInterfaceConfig& address(IPAddress addr, IPAddress mask);
    spark::Vector<NetworkInterfaceAddress> addresses(int family = AF_UNSPEC) const;

    NetworkInterfaceConfig& gateway(SockAddr addr);
    SockAddr gateway(int family = AF_INET) const;

    NetworkInterfaceConfig& dns(SockAddr dns);
    spark::Vector<SockAddr> dns(int family = AF_UNSPEC) const;

    NetworkInterfaceConfig& profile(String id);
    NetworkInterfaceConfig& profile(const char* profile, size_t len = 0);
    spark::Vector<char> profile() const;

    bool isValid() const;

    int exportAsNetworkConfiguration(network_configuration_t* conf) const;
    static void deallocNetworkConfiguration(network_configuration_t* conf);

private:
    static int copySockAddr(SockAddr addr, sockaddr** saddr);
    static int exportAsProtoConfiguration(const spark::Vector<NetworkInterfaceAddress> addrs, NetworkInterfaceConfigSource source, const SockAddr& gateway, spark::Vector<SockAddr> dns, network_configuration_proto_t** proto);
    static void deallocNetworkProto(network_configuration_proto_t* proto);

private:
    spark::Vector<NetworkInterfaceAddress> addr4_;
    spark::Vector<NetworkInterfaceAddress> addr6_;
    NetworkInterfaceConfigSource source4_;
    NetworkInterfaceConfigSource source6_;
    SockAddr gateway4_;
    SockAddr gateway6_;
    spark::Vector<SockAddr> dns4_;
    spark::Vector<SockAddr> dns6_;
    spark::Vector<char> profile_;
};

// SockAddr

inline SockAddr::SockAddr()
        : addr_{} {
}

inline SockAddr::SockAddr(IPAddress addr, uint16_t port)
        : SockAddr() {
    spark::detail::ipAddressPortToSockaddr(addr, port, (sockaddr*)&addr_);
}

inline SockAddr::SockAddr(const sockaddr* addr)
        : SockAddr() {
    if (!addr || addr->sa_family == AF_UNSPEC) {
        return;
    }
    memcpy(&addr_, addr, addr->sa_len);
}

inline SockAddr::SockAddr(const char* addr)
        : SockAddr() {
    String host(addr);
    String port;
    int portDelim = -1;
    if (host.indexOf('.') >= 0) {
        // Probably IPv4
        portDelim = host.lastIndexOf(':');
    } else {
        // Probably IPv6
        portDelim = host.lastIndexOf('#');
    }
    if (portDelim > 0) {
        port = host.substring(portDelim + 1);
        host = host.substring(0, portDelim);
    }
    if (inet_inet_pton(AF_INET, host.c_str(), &((sockaddr_in*)&addr_)->sin_addr)) {
        addr_.ss_family = AF_INET;
        addr_.s2_len = sizeof(sockaddr_in);
        if (port.length() > 0) {
            unsigned long p = 0;
            p = std::strtoul(port.c_str(), nullptr, 10);
            if (p <= std::numeric_limits<uint16_t>::max()) {
                ((sockaddr_in*)&addr_)->sin_port = p;
            }
        }
    } else if (inet_inet_pton(AF_INET6, host.c_str(), &((sockaddr_in6*)&addr_)->sin6_addr)) {
        addr_.ss_family = AF_INET6;
        addr_.s2_len = sizeof(sockaddr_in6);
        if (port.length() > 0) {
            unsigned long p = 0;
            p = std::strtoul(port.c_str(), nullptr, 10);
            if (p <= std::numeric_limits<uint16_t>::max()) {
                ((sockaddr_in6*)&addr_)->sin6_port = p;
            }
        }
    }
}

inline bool SockAddr::isAddrAny() const {
    if (family() == AF_INET) {
        auto inaddr = (sockaddr_in*)&addr_;
        if (inaddr->sin_addr.s_addr == INADDR_ANY) {
            return true;
        }
    } else if (family() == AF_INET6) {
        auto inaddr = (sockaddr_in6*)&addr_;
        in6_addr empty = {};
        if (!memcmp(empty.s6_addr, inaddr->sin6_addr.s6_addr, sizeof(inaddr->sin6_addr.s6_addr))) {
            return true;
        }
    } else if (family() == AF_UNSPEC) {
        return true;
    }

    return false;
}

inline IPAddress SockAddr::address() const {
    uint16_t dummy;
    IPAddress addr;
    spark::detail::sockaddrToIpAddressPort((sockaddr*)&addr_, addr, &dummy);
    return addr;
}

inline SockAddr& SockAddr::address(IPAddress addr) {
    spark::detail::ipAddressPortToSockaddr(addr, port(), (sockaddr*)&addr_);
    return *this;
}

inline SockAddr& SockAddr::address(const sockaddr* addr) {
    if (!addr) {
        return *this;
    }
    memcpy(&addr_, addr, addr->sa_len);
    return *this;
}

inline uint16_t SockAddr::port() const {
    IPAddress dummy;
    uint16_t port = 0;
    spark::detail::sockaddrToIpAddressPort((sockaddr*)&addr_, dummy, &port);
    return port;
}

inline SockAddr& SockAddr::port(uint16_t p) {
    spark::detail::ipAddressPortToSockaddr(address(), p, (sockaddr*)&addr_);
    return *this;
}

inline int SockAddr::family() const {
    return addr_.ss_family;
}

inline sockaddr* SockAddr::toRaw() const {
    return (sockaddr*)&addr_;
}

inline SockAddr::operator IPAddress() const {
    return address();
}

inline bool SockAddr::operator==(const SockAddr& other) const {
    if (family() != other.family()) {
        return false;
    }

    if (family() == AF_INET) {
        auto sin = (sockaddr_in*)toRaw();
        auto sinOther = (sockaddr_in*)other.toRaw();
        return ntohl(sin->sin_addr.s_addr) == ntohl(sinOther->sin_addr.s_addr) &&
                ntohs(sin->sin_port) == ntohs(sinOther->sin_port);
    } else if (family() == AF_INET6) {
        auto sin6 = (sockaddr_in6*)toRaw();
        auto sin6Other = (sockaddr_in6*)other.toRaw();
        return !memcmp(sin6->sin6_addr.s6_addr, sin6Other->sin6_addr.s6_addr, sizeof(sin6->sin6_addr.s6_addr)) &&
                ntohs(sin6->sin6_port) == ntohs(sin6Other->sin6_port) &&
                sin6->sin6_flowinfo == sin6Other->sin6_flowinfo &&
                sin6->sin6_scope_id == sin6Other->sin6_scope_id;
    }

    return false;
}

inline bool SockAddr::operator!=(const SockAddr& other) const {
    return !(*this == other);
}

inline SockAddr::operator bool() const {
    return !isAddrAny();
}

inline void SockAddr::clear(int family) {
    memset(&addr_, 0, sizeof(addr_));
    addr_.ss_family = family;
}

inline String SockAddr::toString() const {
    char host[INET6_ADDRSTRLEN + 7] = {};
    if (family() == AF_INET) {
        inet_inet_ntop(family(), &((sockaddr_in*)&addr_)->sin_addr, host, sizeof(host));
        if (port()) {
            return String(host) + ":" + String(port());
        }
        return String(host);
    } else if (family() == AF_INET6) {
        inet_inet_ntop(family(), &((sockaddr_in6*)&addr_)->sin6_addr, host, sizeof(host));
        if (port()) {
            return String(host) + "#" + String(port());
        }
        return String(host);
    }
    return String();
}

inline bool operator==(const IPAddress& iaddr, const SockAddr& saddr) {
    return SockAddr(iaddr) == saddr;
}

inline bool operator!=(const IPAddress& iaddr, const SockAddr& saddr) {
    return !(iaddr == saddr);
}

inline bool operator==(const SockAddr& saddr, const IPAddress& iaddr) {
    return iaddr == saddr;
}

inline bool operator!=(const SockAddr& saddr, const IPAddress& iaddr) {
    return iaddr != saddr;
}

// NetworkInterfaceAddress

inline NetworkInterfaceAddress::NetworkInterfaceAddress()
        : prefixLen_(0) {
}

inline NetworkInterfaceAddress::NetworkInterfaceAddress(SockAddr addr, SockAddr mask)
        : NetworkInterfaceAddress() {
    address(addr, mask);
}

inline NetworkInterfaceAddress::NetworkInterfaceAddress(SockAddr addr, uint8_t prefixLen)
        : NetworkInterfaceAddress() {
    address(addr, prefixLen);
}

inline NetworkInterfaceAddress::NetworkInterfaceAddress(IPAddress addr, IPAddress mask)
        : NetworkInterfaceAddress() {
    address(addr, mask);
}

inline NetworkInterfaceAddress::NetworkInterfaceAddress(IPAddress addr, uint8_t prefixLen)
        : NetworkInterfaceAddress() {
    address(addr, prefixLen);
}


inline SockAddr NetworkInterfaceAddress::address() const {
    return addr_;
}

inline NetworkInterfaceAddress& NetworkInterfaceAddress::address(SockAddr addr, SockAddr mask) {
    addr_ = SockAddr(addr);
    mask_ = SockAddr(mask);
    prefixLen_ = spark::detail::netmaskToPrefixLength(mask_.toRaw());
    return *this;
}

inline NetworkInterfaceAddress& NetworkInterfaceAddress::address(SockAddr addr, uint8_t prefixLen) {
    addr_ = SockAddr(addr);
    prefixLen_ = prefixLen;
    mask_ = addr_; // For address family to match
    spark::detail::prefixLengthToNetmask(mask_.toRaw(), prefixLen);
    return *this;
}

inline NetworkInterfaceAddress& NetworkInterfaceAddress::address(IPAddress addr, IPAddress mask) {
    return address(SockAddr(addr), SockAddr(mask));
}

inline NetworkInterfaceAddress& NetworkInterfaceAddress::address(IPAddress addr, uint8_t prefixLen) {
    return address(SockAddr(addr), prefixLen);
}

inline SockAddr NetworkInterfaceAddress::mask() const {
    return mask_;
}

inline NetworkInterfaceAddress& NetworkInterfaceAddress::mask(SockAddr mask) {
    mask_ = mask;
    prefixLen_ = spark::detail::netmaskToPrefixLength(mask_.toRaw());
    return *this;
}

inline NetworkInterfaceAddress& NetworkInterfaceAddress::mask(IPAddress mask) {
    return this->mask(SockAddr(mask));
}

inline uint8_t NetworkInterfaceAddress::prefixLength() const {
    return prefixLen_;
}

inline NetworkInterfaceAddress& NetworkInterfaceAddress::prefixLength(uint8_t prefixLen) {
    if (addr_.family() == AF_UNSPEC) {
        return *this;
    }
    prefixLen_ = prefixLen;
    mask_ = addr_; // For family to match
    spark::detail::prefixLengthToNetmask(mask_.toRaw(), prefixLen);
    return *this;
}

inline int NetworkInterfaceAddress::family() const {
    return addr_.family();
}

inline bool NetworkInterfaceAddress::operator==(const NetworkInterfaceAddress& other) const {
    return addr_ == other.addr_ && mask_ == other.mask_ && prefixLen_ == other.prefixLen_;
}

inline bool NetworkInterfaceAddress::operator!=(const NetworkInterfaceAddress& other) const {
    return !(*this == other);
}

// NetworkInterfaceConfig
inline NetworkInterfaceConfig::NetworkInterfaceConfig()
        : source4_(NetworkInterfaceConfigSource::NONE),
          source6_(NetworkInterfaceConfigSource::NONE) {
}

inline NetworkInterfaceConfig::NetworkInterfaceConfig(const network_configuration_t* conf)
        : NetworkInterfaceConfig() {
    if (!conf) {
        return;
    }

    if (conf->profile && conf->profile_len) {
        profile_.append(conf->profile, conf->profile_len);
    }

    if (conf->ip4) {
        if (conf->ip4->gateway) {
            gateway4_ = SockAddr(conf->ip4->gateway);
        }
        source4_ = (NetworkInterfaceConfigSource)conf->ip4->source;
        if (conf->ip4->addrs) {
            for (auto a = conf->ip4->addrs; a != nullptr; a = a->next) {
                addr4_.append(NetworkInterfaceAddress(SockAddr(a->addr), SockAddr(a->netmask)));
            }
        }
        if (conf->ip4->dns) {
            for (auto a = conf->ip4->dns; a != nullptr; a = a->next) {
                dns4_.append(SockAddr(a->addr));
            }
        }
    }
    if (conf->ip6) {
        if (conf->ip6->gateway) {
            gateway6_ = SockAddr(conf->ip6->gateway);
        }
        source6_ = (NetworkInterfaceConfigSource)conf->ip6->source;
        if (conf->ip6->addrs) {
            for (auto a = conf->ip6->addrs; a != nullptr; a = a->next) {
                addr6_.append(NetworkInterfaceAddress(SockAddr(a->addr), SockAddr(a->netmask)));
            }
        }
        if (conf->ip6->dns) {
            for (auto a = conf->ip6->dns; a != nullptr; a = a->next) {
                dns6_.append(SockAddr(a->addr));
            }
        }
    }
}

inline NetworkInterfaceConfig& NetworkInterfaceConfig::source(NetworkInterfaceConfigSource source, int family) {
    if (family == AF_INET) {
        source4_ = source;
    } else if (family == AF_INET6) {
        source6_ = source;
    }
    return *this;
}

inline NetworkInterfaceConfigSource NetworkInterfaceConfig::source(int family) const {
    if (family == AF_INET) {
        return source4_;
    } else if (family == AF_INET6) {
        return source6_;
    }
    return NetworkInterfaceConfigSource::NONE;
}

inline NetworkInterfaceConfig& NetworkInterfaceConfig::address(const NetworkInterfaceAddress& addr) {
    if (addr.address().family() == AF_INET) {
        addr4_.append(addr);
    } else if (addr.address().family() == AF_INET6) {
        addr6_.append(addr);
    }
    return *this;
}

inline NetworkInterfaceConfig& NetworkInterfaceConfig::address(SockAddr addr, SockAddr mask) {
    return address(NetworkInterfaceAddress(addr, mask));
}

inline NetworkInterfaceConfig& NetworkInterfaceConfig::address(IPAddress addr, IPAddress mask) {
    return address(NetworkInterfaceAddress(addr, mask));
}

inline spark::Vector<NetworkInterfaceAddress> NetworkInterfaceConfig::addresses(int family) const {
    if (family == AF_UNSPEC) {
        auto tmp = addr4_;
        tmp.append(addr6_);
        return tmp;
    } else if (family == AF_INET) {
        return addr4_;
    } else if (family == AF_INET6) {
        return addr6_;
    }
    return spark::Vector<NetworkInterfaceAddress>();
}

inline NetworkInterfaceConfig& NetworkInterfaceConfig::gateway(SockAddr addr) {
    if (addr.family() == AF_INET) {
        gateway4_ = addr;
    } else if (addr.family() == AF_INET6) {
        gateway6_ = addr;
    }
    return *this;
}

inline SockAddr NetworkInterfaceConfig::gateway(int family) const {
    if (family == AF_INET) {
        return gateway4_;
    } else if (family == AF_INET6) {
        return gateway6_;
    }
    return SockAddr();
}

inline NetworkInterfaceConfig& NetworkInterfaceConfig::dns(SockAddr addr) {
    if (addr.family() == AF_INET) {
        dns4_.append(addr);
    } else if (addr.family() == AF_INET6) {
        dns6_.append(addr);
    }
    return *this;
}

inline spark::Vector<SockAddr> NetworkInterfaceConfig::dns(int family) const {
    spark::Vector<SockAddr> res;
    if (family == AF_INET || family == AF_UNSPEC) {
        for (const auto& a: dns4_) {
            res.append(a);
        }
    }
    if (family == AF_INET6 || family == AF_UNSPEC) {
        for (const auto& a: dns6_) {
            res.append(a);
        }
    }
    return res;
}

inline NetworkInterfaceConfig& NetworkInterfaceConfig::profile(String id) {
    profile_ = spark::Vector<char>(id.c_str(), id.length());
    return *this;
}

inline NetworkInterfaceConfig& NetworkInterfaceConfig::profile(const char* profile, size_t len) {
    if (profile && len == 0) {
        len = strlen(profile);
    }
    profile_ = spark::Vector<char>(profile, len);
    return *this;
}


inline spark::Vector<char> NetworkInterfaceConfig::profile() const {
    return profile_;
}

inline bool NetworkInterfaceConfig::isValid() const {
    if (addr4_.size() || dns4_.size() || !gateway4_.isAddrAny()) {
        if (source4_ == NetworkInterfaceConfigSource::NONE) {
            return false;
        }
    }

    if (addr6_.size() || dns6_.size() || !gateway6_.isAddrAny()) {
        if (source6_ == NetworkInterfaceConfigSource::NONE) {
            return false;
        }
    }
    return true;
}

inline int NetworkInterfaceConfig::exportAsProtoConfiguration(const spark::Vector<NetworkInterfaceAddress> addrs, NetworkInterfaceConfigSource source, const SockAddr& gateway, spark::Vector<SockAddr> dns, network_configuration_proto_t** proto) {
    CHECK_TRUE(proto, SYSTEM_ERROR_INVALID_ARGUMENT);
    *proto = new network_configuration_proto_t;
    CHECK_TRUE(*proto, SYSTEM_ERROR_NO_MEMORY);
    memset(*proto, 0, sizeof(network_configuration_proto_t));
    (*proto)->size = sizeof(network_configuration_proto_t);
    (*proto)->source = particle::to_underlying(source);

    if (addrs.size() > 0) {
        sockaddr_list** next = &(*proto)->addrs;
        for (const auto& a: addrs) {
            *next = new sockaddr_list;
            CHECK_TRUE(*next, SYSTEM_ERROR_NO_MEMORY);
            memset(*next, 0, sizeof(sockaddr_list));
            CHECK(copySockAddr(a.address(), &(*next)->addr));
            CHECK(copySockAddr(a.mask(), &(*next)->netmask));
            (*next)->prefix_len = a.prefixLength();
            next = &(*next)->next;
        }
    }

    if (dns.size() > 0) {
        sockaddr_list** next = &(*proto)->dns;
        for (const auto& a: dns) {
            *next = new sockaddr_list;
            CHECK_TRUE(*next, SYSTEM_ERROR_NO_MEMORY);
            memset(*next, 0, sizeof(sockaddr_list));
            CHECK(copySockAddr(a, &(*next)->addr));
            next = &(*next)->next;
        }
    }

    if (gateway.family() != AF_UNSPEC) {
        CHECK(copySockAddr(gateway, &(*proto)->gateway));
    }

    return 0;
}

inline int NetworkInterfaceConfig::copySockAddr(SockAddr addr, sockaddr** saddr) {
    *saddr = nullptr;
    if (addr.family() == AF_UNSPEC) {
        return 0;
    }
    *saddr = (sockaddr*)malloc(addr.toRaw()->sa_len);
    CHECK_TRUE(*saddr, SYSTEM_ERROR_NO_MEMORY);
    memcpy(*saddr, addr.toRaw(), addr.toRaw()->sa_len);
    return 0;
}

inline int NetworkInterfaceConfig::exportAsNetworkConfiguration(network_configuration_t* conf) const {
    CHECK_TRUE(conf, SYSTEM_ERROR_INVALID_ARGUMENT);
    CHECK_TRUE(isValid(), SYSTEM_ERROR_BAD_DATA);

    conf->size = sizeof(*conf);

    NAMED_SCOPE_GUARD(sg, {
        deallocNetworkConfiguration(conf);
    });

    if (profile_.size()) {
        conf->profile = (char*)malloc(profile_.size());
        CHECK_TRUE(conf->profile, SYSTEM_ERROR_NO_MEMORY);
        memcpy(conf->profile, profile_.data(), profile_.size());
        conf->profile_len = profile_.size();
    }

    CHECK(exportAsProtoConfiguration(addr4_, source4_, gateway4_, dns4_, &conf->ip4));
    CHECK(exportAsProtoConfiguration(addr6_, source6_, gateway6_, dns6_, &conf->ip6));

    sg.dismiss();
    return 0;
}

inline void NetworkInterfaceConfig::deallocNetworkConfiguration(network_configuration_t* conf) {
    if (!conf) {
        return;
    }
    if (conf->profile) {
        free(conf->profile);
        conf->profile = nullptr;
    }
    if (conf->ip4) {
        deallocNetworkProto(conf->ip4);
        conf->ip4 = nullptr;
    }
    if (conf->ip6) {
        deallocNetworkProto(conf->ip6);
        conf->ip6 = nullptr;
    }
}

inline void NetworkInterfaceConfig::deallocNetworkProto(network_configuration_proto_t* proto) {
    if (proto->gateway) {
        free(proto->gateway);
        proto->gateway = nullptr;
    }
    sockaddr_list* cur = proto->addrs;
    while (cur != nullptr) {
        if (cur->addr) {
            free(cur->addr);
        }
        if (cur->netmask) {
            free(cur->netmask);
        }
        auto next = cur->next;
        free(cur);
        cur = next;
    }
    cur = proto->dns;
    while (cur != nullptr) {
        if (cur->addr) {
            free(cur->addr);
        }
        if (cur->netmask) {
            free(cur->netmask);
        }
        auto next = cur->next;
        free(cur);
        cur = next;
    }
}


} // particle

#endif // __cplusplus

#endif // HAL_USE_SOCKET_HAL_POSIX
