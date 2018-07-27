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

#ifndef HAL_NETWORK_LWIP_NAT64_H
#define HAL_NETWORK_LWIP_NAT64_H

#include <cstdint>
#include <lwip/ip_addr.h>
#include <lwip/ip4.h>
#include <lwip/ip6.h>
#include <lwip/netif.h>
#include <lwip/pbuf.h>
#include <memory>
#include <cstring>
#include "intrusive_list.h"
#include "simple_pool_allocator.h"
#include "logging.h"
#include "ipaddr_util.h"

namespace particle { namespace net { namespace nat {

template <typename AddrT, typename DerivedT>
class IpTransportAddressGeneric {
public:
    IpTransportAddressGeneric() = default;
    IpTransportAddressGeneric(const IpTransportAddressGeneric<AddrT, DerivedT>& rhs) = default;
    IpTransportAddressGeneric(const AddrT& addr, uint16_t l4Id);

    bool isV4() const;
    bool isV6() const;

    const AddrT& address() const;
    void setAddress(const AddrT& addr);

    uint16_t l4Id() const;
    uint16_t port() const;
    uint16_t icmpId() const;

    void setL4Id(uint16_t l4Id);
    void setPort(uint16_t port);
    void setIcmpId(uint16_t icmpId);

    bool operator==(const DerivedT& rhs) const;
    bool operator!=(const DerivedT& rhs) const;

    template <typename OtherT>
    bool operator==(const OtherT& rhs) const;
    template <typename OtherT>
    bool operator!=(const OtherT& rhs) const;

protected:
    AddrT addr_ = {};
    uint16_t l4Id_ = 0;
};

class Ip4TransportAddress;
class Ip6TransportAddress;
class IpTransportAddress;

class IpTransportAddress : public IpTransportAddressGeneric<ip_addr_t, IpTransportAddress> {
public:
    /* Needed for <= C++17 */
    IpTransportAddress() = default;
    IpTransportAddress(const IpTransportAddress& rhs) = default;

    IpTransportAddress(const Ip4TransportAddress& rhs);
    IpTransportAddress(const Ip6TransportAddress& rhs);
    using IpTransportAddressGeneric<ip_addr_t, IpTransportAddress>::IpTransportAddressGeneric;

    bool isV4Impl() const;
    bool isV6Impl() const;

    bool equals(const IpTransportAddress& rhs) const;
};

class Ip4TransportAddress : public IpTransportAddressGeneric<ip4_addr_t, Ip4TransportAddress> {
public:
    /* Needed for <= C++17 */
    Ip4TransportAddress() = default;
    Ip4TransportAddress(const Ip4TransportAddress& rhs) = default;

    Ip4TransportAddress(const IpTransportAddress& rhs);
    using IpTransportAddressGeneric<ip4_addr_t, Ip4TransportAddress>::IpTransportAddressGeneric;

    bool isV4Impl() const;
    bool isV6Impl() const;

    bool equals(const Ip4TransportAddress& rhs) const;
};

class Ip6TransportAddress : public IpTransportAddressGeneric<ip6_addr_t, Ip6TransportAddress> {
public:
    /* Needed for <= C++17 */
    Ip6TransportAddress() = default;
    Ip6TransportAddress(const Ip6TransportAddress& rhs) = default;

    Ip6TransportAddress(const IpTransportAddress& rhs);
    using IpTransportAddressGeneric<ip6_addr_t, Ip6TransportAddress>::IpTransportAddressGeneric;

    bool isV4Impl() const;
    bool isV6Impl() const;

    bool equals(const Ip6TransportAddress& rhs) const;
};

enum L4Protocol {
    L4_PROTO_NONE = 0,
    /* Both ICMPv4 and ICMPv6 */
    L4_PROTO_ICMP = 1,
    /* Unsupported */
    /* L4_PROTO_TCP  = 6, */
    L4_PROTO_UDP  = 17
};

class Rule {
public:
    Rule(netif* in, netif* out);
    Rule(const Rule& rhs) = default;

    netif* inside() const;
    netif* outside() const;

private:
    netif* inside_;
    netif* outside_;
};

class BibEntry;
class SessionEntry;
class RuleEntry;

using BibTable = particle::IntrusiveList<BibEntry>;
using SessionTable = particle::IntrusiveList<SessionEntry>;
using RuleTable = particle::IntrusiveList<RuleEntry>;

template <typename DerivedT>
class ListNode {
public:
    DerivedT* next;
};

class BibEntry : public ListNode<BibEntry> {
public:
    BibEntry(const Ip6TransportAddress& src6, const Ip4TransportAddress& dst4);

    const Ip6TransportAddress& src6() const;
    const Ip4TransportAddress& dst4() const;

    bool matches(const IpTransportAddress& addr) const;
    bool empty() const;

    SessionEntry* lookupSession(const IpTransportAddress& src, const IpTransportAddress& dst);
    SessionEntry* addSession(const Ip6TransportAddress& dst, uint32_t lifetime, particle::SimpleAllocator& allocator);

    bool timeout(uint32_t dt, particle::SimpleAllocator& allocator);

private:
    Ip6TransportAddress src6_;
    Ip4TransportAddress dst4_;

    SessionTable sessions_;
};

class SessionEntry : public ListNode<SessionEntry> {
public:
    SessionEntry(BibEntry* bib, const Ip6TransportAddress& dst6);

    BibEntry* bib();

    const Ip6TransportAddress& src6() const;
    const Ip6TransportAddress& dst6() const;
    const Ip4TransportAddress& src4() const;
    Ip4TransportAddress dst4() const;

    bool matches(const IpTransportAddress& src, const IpTransportAddress& dst);

    uint32_t setLifetime(uint32_t lifetime);
    uint32_t lifetime() const;

    bool timeout(uint32_t dt);

private:
    BibEntry* bib_;
    Ip6TransportAddress dst6_;

    uint32_t lifetime_;
};

static const size_t NAT64_ENTRY_SIZE = std::max(sizeof(BibEntry), sizeof(SessionEntry));

class Nat64 {
public:
    Nat64();
    ~Nat64();

    void setPref64(const ip6_addr_t* pref64);
    ip6_addr_t getPref64() const;
    bool enable(const Rule& rule);
    bool disable(const Rule& rule);
    bool disable(netif* any);

    int ip4Input(pbuf* p, ip_hdr* header, netif* in);
    int ip6Input(pbuf* p, ip6_hdr* header, netif* in);

protected:
    int natInput(const ip_addr_t* src, const ip_addr_t* dst, L4Protocol proto, pbuf* p, netif* in, void* ipheader);
    bool filter(const IpTransportAddress& src, const IpTransportAddress& dst, netif* in) const;

    BibEntry* lookupBib(const IpTransportAddress& src, const IpTransportAddress& dst, L4Protocol proto);
    BibEntry* addBib(const IpTransportAddress& src, const IpTransportAddress& dst, L4Protocol proto);

    bool findNextL4Id(Ip4TransportAddress& src, L4Protocol proto);
    bool findNextUdpPort(Ip4TransportAddress& src);
    bool findNextIcmpId(Ip4TransportAddress& src);

    void timeout(uint32_t dt);

    void enableSessionTimer();
    void disableSessionTimer();

    static void timeoutHandlerCb(void* arg);

private:

private:
    /* TODO: a list of rules */
    Rule* rule_ = nullptr;

    /* Defaults to 64:ff9b::/96 */
    ip6_addr_t pref64_;

    BibTable udpBibTable_;
    uint16_t udpNextPort_;
    BibTable icmpBibTable_;
    uint16_t icmpNextId_;

    std::unique_ptr<SimpleAllocedPool> pool_;
};

/* IpTransportAddressGeneric */
template <typename AddrT, typename DerivedT>
inline IpTransportAddressGeneric<AddrT, DerivedT>::IpTransportAddressGeneric(const AddrT& addr, uint16_t l4Id)
        : addr_(addr),
          l4Id_(l4Id) {
}

template <typename AddrT, typename DerivedT>
inline bool IpTransportAddressGeneric<AddrT, DerivedT>::isV4() const {
    return static_cast<const DerivedT*>(this)->isV4Impl();
}

template <typename AddrT, typename DerivedT>
inline bool IpTransportAddressGeneric<AddrT, DerivedT>::isV6() const {
    return static_cast<const DerivedT*>(this)->isV6Impl();
}

template <typename AddrT, typename DerivedT>
inline void IpTransportAddressGeneric<AddrT, DerivedT>::setAddress(const AddrT& addr) {
    ::memcpy(&addr_, &addr, sizeof(AddrT));
}

template <typename AddrT, typename DerivedT>
inline const AddrT& IpTransportAddressGeneric<AddrT, DerivedT>::address() const {
    return addr_;
}

template <typename AddrT, typename DerivedT>
inline uint16_t IpTransportAddressGeneric<AddrT, DerivedT>::l4Id() const {
    return l4Id_;
}

template <typename AddrT, typename DerivedT>
inline uint16_t IpTransportAddressGeneric<AddrT, DerivedT>::port() const {
    return l4Id();
}

template <typename AddrT, typename DerivedT>
inline uint16_t IpTransportAddressGeneric<AddrT, DerivedT>::icmpId() const {
    return l4Id();
}

template <typename AddrT, typename DerivedT>
inline void IpTransportAddressGeneric<AddrT, DerivedT>::setL4Id(uint16_t l4Id) {
    l4Id_ = l4Id;
}

template <typename AddrT, typename DerivedT>
inline void IpTransportAddressGeneric<AddrT, DerivedT>::setPort(uint16_t port) {
    setL4Id(port);
}

template <typename AddrT, typename DerivedT>
inline void IpTransportAddressGeneric<AddrT, DerivedT>::setIcmpId(uint16_t icmpId) {
    setL4Id(icmpId);
}

template <typename AddrT, typename DerivedT>
inline bool IpTransportAddressGeneric<AddrT, DerivedT>::operator==(const DerivedT& rhs) const {
    return static_cast<const DerivedT*>(this)->equals(rhs);
}

template <typename AddrT, typename DerivedT>
inline bool IpTransportAddressGeneric<AddrT, DerivedT>::operator!=(const DerivedT& rhs) const {
    return !static_cast<const DerivedT*>(this)->equals(rhs);
}

template <typename AddrT, typename DerivedT>
template <typename OtherT>
bool IpTransportAddressGeneric<AddrT, DerivedT>::operator==(const OtherT& rhs) const {
    return static_cast<const DerivedT*>(this)->equals(rhs);
}

template <typename AddrT, typename DerivedT>
template <typename OtherT>
bool IpTransportAddressGeneric<AddrT, DerivedT>::operator!=(const OtherT& rhs) const {
    return !static_cast<const DerivedT*>(this)->equals(rhs);
}

/* IpTransportAddress */
inline IpTransportAddress::IpTransportAddress(const Ip4TransportAddress& rhs) {
    ip_addr_t addr = {};
    ip_addr_copy_from_ip4(addr, rhs.address());
    setAddress(addr);
    setL4Id(rhs.l4Id());
}

inline IpTransportAddress::IpTransportAddress(const Ip6TransportAddress& rhs) {
    ip_addr_t addr = {};
    ip_addr_copy_from_ip6(addr, rhs.address());
    setAddress(addr);
    setL4Id(rhs.l4Id());
}

inline bool IpTransportAddress::isV4Impl() const {
    return IP_IS_V4_VAL(addr_);
}

inline bool IpTransportAddress::isV6Impl() const {
    return IP_IS_V6_VAL(addr_);
}

inline bool IpTransportAddress::equals(const IpTransportAddress& rhs) const {
    return ip_addr_cmp_zoneless(&address(), &rhs.address()) && l4Id() == rhs.l4Id();
}

/* Ip4TransportAddress */
inline Ip4TransportAddress::Ip4TransportAddress(const IpTransportAddress& rhs) {
    ip4_addr_copy(addr_, *ip_2_ip4(&rhs.address()));
    setL4Id(rhs.l4Id());
}

inline bool Ip4TransportAddress::isV4Impl() const {
    return true;
}

inline bool Ip4TransportAddress::isV6Impl() const {
    return false;
}

inline bool Ip4TransportAddress::equals(const Ip4TransportAddress& rhs) const {
    return ip4_addr_cmp(&address(), &rhs.address()) && l4Id() == rhs.l4Id();
}

/* Ip6TransportAddress */
inline Ip6TransportAddress::Ip6TransportAddress(const IpTransportAddress& rhs) {
    ip6_addr_copy(addr_, *ip_2_ip6(&rhs.address()));
    setL4Id(rhs.l4Id());
}

inline bool Ip6TransportAddress::isV4Impl() const {
    return false;
}

inline bool Ip6TransportAddress::isV6Impl() const {
    return true;
}

inline bool Ip6TransportAddress::equals(const Ip6TransportAddress& rhs) const {
    return ip6_addr_cmp_zoneless(&address(), &rhs.address()) && l4Id() == rhs.l4Id();
}

/* Rule */
inline Rule::Rule(netif* in, netif* out)
        : inside_(in),
          outside_(out) {
}

inline netif* Rule::inside() const {
    return inside_;
}

inline netif* Rule::outside() const {
    return outside_;
}

/* BibEntry */
inline BibEntry::BibEntry(const Ip6TransportAddress& src6, const Ip4TransportAddress& dst4)
        : src6_(src6),
          dst4_(dst4) {
}

inline const Ip6TransportAddress& BibEntry::src6() const {
    return src6_;
}

inline const Ip4TransportAddress& BibEntry::dst4() const {
    return dst4_;
}

inline bool BibEntry::matches(const IpTransportAddress& addr) const {
    if (addr.isV4()) {
        return dst4() == addr;
    } else if (addr.isV6()) {
        return src6() == addr;
    }

    return false;
}

inline bool BibEntry::empty() const {
    return sessions_.front() == nullptr;
}

inline SessionEntry* BibEntry::lookupSession(const IpTransportAddress& src, const IpTransportAddress& dst) {
    for (auto* s = sessions_.front(); s != nullptr; s = s->next) {
        if (s->matches(src, dst)) {
            return s;
        }
    }

    return nullptr;
}

inline SessionEntry* BibEntry::addSession(const Ip6TransportAddress& dst, uint32_t lifetime, particle::SimpleAllocator& allocator) {
    auto sess = (SessionEntry*)allocator.alloc(NAT64_ENTRY_SIZE);
    if (sess) {
        new(sess) SessionEntry(this, dst);
        sess->setLifetime(lifetime);
        sessions_.pushFront(sess);
        return sess;
    }

    LOG_DEBUG(TRACE, "Failed to allocate new session");

    return nullptr;
}

inline bool BibEntry::timeout(uint32_t dt, particle::SimpleAllocator& allocator) {
    using namespace particle::net;
    for (auto s = sessions_.front(), p = static_cast<SessionEntry*>(nullptr); s != nullptr;) {
        if (s->timeout(dt)) {
            LOG_DEBUG(TRACE, "Session timed out %s#%u <-> %s#%u, %s#%u <-> %s#%u",
                      IP6ADDR_NTOA(&s->src6().address()), s->src6().l4Id(),
                      IP6ADDR_NTOA(&s->dst6().address()), s->dst6().l4Id(),
                      IP4ADDR_NTOA(&s->src4().address()), s->src4().l4Id(),
                      IP4ADDR_NTOA(&s->dst4().address()), s->dst4().l4Id());
            auto popped = sessions_.pop(s, p);
            s = popped->next;
            allocator.free(popped);
        } else {
            p = s;
            s = s->next;
        }
    }

    return empty();
}

/* SessionEntry */
inline SessionEntry::SessionEntry(BibEntry* bib, const Ip6TransportAddress& dst6)
        : bib_(bib),
          dst6_(dst6),
          lifetime_(0) {
}

inline BibEntry* SessionEntry::bib() {
    return bib_;
}

inline const Ip6TransportAddress& SessionEntry::src6() const {
    return bib_->src6();
}

inline const Ip6TransportAddress& SessionEntry::dst6() const {
    return dst6_;
}

inline const Ip4TransportAddress& SessionEntry::src4() const {
    return bib_->dst4();
}

inline Ip4TransportAddress SessionEntry::dst4() const {
    const auto& dst6Addr = dst6().address();
    ip4_addr_t addr = {};
    unmap_ipv4_mapped_ipv6(&addr, &dst6Addr);
    return Ip4TransportAddress(addr, dst6().port());
}

inline bool SessionEntry::matches(const IpTransportAddress& src, const IpTransportAddress& dst) {
    if (src.isV6()) {
        return src6() == src && dst6() == dst;
    } else if (src.isV4()) {
        return src4() == dst && dst4() == src;
    }

    return false;
}

inline uint32_t SessionEntry::setLifetime(uint32_t lifetime) {
    std::swap(lifetime_, lifetime);
    return lifetime;
}

inline uint32_t SessionEntry::lifetime() const {
    return lifetime_;
}

inline bool SessionEntry::timeout(uint32_t dt) {
    if (lifetime() <= dt) {
        setLifetime(0);
        return true;
    }
    setLifetime(lifetime() - dt);
    return false;
}

} } } /* particle::net::nat */

#endif /* HAL_NETWORK_LWIP_NAT64_H */
