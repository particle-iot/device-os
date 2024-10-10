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

#undef LOG_COMPILE_TIME_LEVEL
#define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ERROR

#include "logging.h"
LOG_SOURCE_CATEGORY("net.nat64")

#include "nat.h"
#include "hal_platform_config.h"
#include "platforms.h"

/* FIXME: this is a hack for static_assert(MEMP_NUM_SYS_TIMEOUT > LWIP_NUM_SYS_TIMEOUT_INTERNAL)
 * to work correctly
 */
#include <netif/ppp/ppp_opts.h>

#include <lwip/ip.h>
#include <lwip/udp.h>
#include <lwip/inet_chksum.h>
#include <lwip/prot/icmp.h>
#include <lwip/prot/tcp.h>
#include <lwip/timeouts.h>
#include "lwiplock.h"
#include "random.h"

#undef LOG_DEBUG
#define LOG_DEBUG(...) LOG(__VA_ARGS__)

using namespace particle::net;
using namespace particle::net::nat;

namespace {

L4Protocol ipProtoToL4Protocol(uint16_t proto) {
    switch (proto) {
        case IP_PROTO_UDP: {
            return L4_PROTO_UDP;
        }
        case IP_PROTO_ICMP: {
            return L4_PROTO_ICMP;
        }
        case IP_PROTO_TCP: {
            return L4_PROTO_TCP;
        }
    }

    return L4_PROTO_NONE;
}

const char* l4ProtocolToName(L4Protocol protocol) {
    switch (protocol) {
        case L4_PROTO_UDP: {
            return "UDP";
        }
        case L4_PROTO_TCP: {
            return "TCP";
        }
        case L4_PROTO_ICMP: {
            return "ICMP";
        }
    }
    return "UNKNOWN";
}

uint16_t ip6HeaderLen(uint8_t v) {
    return (8 * (1 + v));
}

uint16_t nextBoundId(uint16_t id, uint16_t min, uint16_t max) {
    ++id;
    if (id > max) {
        id = min;
    }

    return id;
}

/* UDP_MIN: 2 minutes (as defined in [RFC4787]) */
//#if PLATFORM_ID != PLATFORM_BORON && PLATFORM_ID != PLATFORM_BSOM && PLATFORM_ID != PLATFORM_B5SOM && PLATFORM_ID != PLATFORM_TRACKER
const uint32_t DEFAULT_UDP_NAT_LIFETIME = 120 * 1000;
// const uint32_t DEFAULT_TCP_NAT_LIFETIME = 86400 * 1000; // RFC
const uint32_t DEFAULT_TCP_NAT_LIFETIME = 5 * 60 * 1000; // non-standard
// #else
// const uint32_t DEFAULT_UDP_NAT_LIFETIME = HAL_PLATFORM_CELLULAR_CLOUD_KEEPALIVE_INTERVAL + 60 * 1000;
// #endif // PLATFORM_ID == PLATFORM_BORON && PLATFORM_ID != PLATFORM_BSOM && PLATFORM_ID != PLATFORM_B5SOM && PLATFORM_ID != PLATFORM_TRACKER
const uint16_t DEFAULT_UDP_NAT_MIN_PORT = 40000;
const uint16_t DEFAULT_UDP_NAT_MAX_PORT = 49000;
const uint16_t DEFAULT_TCP_NAT_MIN_PORT = 40000;
const uint16_t DEFAULT_TCP_NAT_MAX_PORT = 65535;

/* ICMP_DEFAULT: 60 seconds (as defined in [RFC5508]) */
const uint32_t DEFAULT_ICMP_NAT_LIFETIME = 60 * 1000;
const uint16_t DEFAULT_ICMP_NAT_MIN_ID = 0;
const uint16_t DEFAULT_ICMP_NAT_MAX_ID = 65535;

const size_t DEFAULT_POOL_SIZE = 32 * 1024;
const size_t DEFAULT_MAX_TRANSLATION_ENTRIES = DEFAULT_POOL_SIZE / NAT64_ENTRY_SIZE;

const size_t DEFAULT_SESSION_CLEANUP_TIMEOUT = 1000;

static_assert(MEMP_NUM_SYS_TIMEOUT > LWIP_NUM_SYS_TIMEOUT_INTERNAL, "An extra timeout should be allocated for NAT64 service. Increase MEMP_NUM_SYS_TIMEOUT");

} /* anonymous */

Nat64::Nat64() {
    IP6_ADDR(&pref64_, PP_HTONL(0x64ff9b), 0, 0, 0);
    unsigned int rVal;
    particle::Random::genSecure((char*)&rVal, sizeof(rVal));
    udpNextPort_ = rVal % (DEFAULT_UDP_NAT_MAX_PORT - DEFAULT_UDP_NAT_MIN_PORT) + DEFAULT_UDP_NAT_MIN_PORT;
    tcpNextPort_ = rVal % (DEFAULT_TCP_NAT_MAX_PORT - DEFAULT_TCP_NAT_MIN_PORT) + DEFAULT_TCP_NAT_MIN_PORT;
    icmpNextId_ = rVal % (DEFAULT_ICMP_NAT_MAX_ID - DEFAULT_ICMP_NAT_MIN_ID) + DEFAULT_ICMP_NAT_MIN_ID;
}

Nat64::~Nat64() {
    disableSessionTimer();
}

void Nat64::setPref64(const ip6_addr_t* pref64) {
    LwipTcpIpCoreLock lk;
    ip6_addr_set(&pref64_, pref64);
}

ip6_addr_t Nat64::getPref64() const {
    LwipTcpIpCoreLock lk;
    return pref64_;
}

bool Nat64::enable(const Rule& rule) {
    LwipTcpIpCoreLock lk;
    disable(nullptr);
    rule_ = new Rule(rule);
    if (!pool_) {
        pool_.reset(new SimpleAllocedPool(DEFAULT_MAX_TRANSLATION_ENTRIES * NAT64_ENTRY_SIZE));
        udpBibTable_ = decltype(udpBibTable_)();
        tcpBibTable_ = decltype(tcpBibTable_)();
        icmpBibTable_ = decltype(icmpBibTable_)();
        enableSessionTimer();
    }
    return true;
}

bool Nat64::disable(const Rule& rule) {
    LwipTcpIpCoreLock lk;
    return disable(nullptr);
}

bool Nat64::disable(netif* any) {
    LwipTcpIpCoreLock lk;
    if (rule_) {
        delete rule_;
        rule_ = nullptr;
    }
    return true;
}

int Nat64::ip4Input(pbuf* p, ip_hdr* header, netif* in) {
    /* Don't discard by default */
    int r = 0;

    auto proto = ipProtoToL4Protocol(IPH_PROTO(header));

    LOG(INFO, "ip4 input [%c%c%u] %x %x %d", in->name[0], in->name[1], in->num, p, header, proto);

    if ((IPH_OFFSET(header) & PP_HTONS(IP_OFFMASK | IP_MF)) != 0) {
        /* Fragmented packet */
        LOG(WARN, "Fragmented IPv4 packets are not supported");
        return r;
    }

    if (proto != L4_PROTO_NONE) {
        const uint16_t hlen = IPH_HL_BYTES(header);
        pbuf_remove_header(p, hlen);
        r = natInput(ip_current_src_addr(), ip_current_dest_addr(), proto, p, in, header);
        pbuf_add_header_force(p, hlen);
    }

    return r;
}

int Nat64::ip6Input(pbuf* p, ip6_hdr* header, netif* in) {
    /* Discard by default */
    int r = 1;

    uint16_t headerLen = IP6_HLEN;
    L4Protocol proto;

    /* Skip the fixed header */
    pbuf_remove_header(p, IP6_HLEN);

    /* Traverse all the headers */
    const uint8_t* nexth = &IP6H_NEXTH(header);
    while (*nexth != IP6_NEXTH_NONE) {
        uint16_t len = 0;
        switch (*nexth) {
            case IP6_NEXTH_HOPBYHOP: {
                ip6_hbh_hdr* hbh = (ip6_hbh_hdr*)p->payload;
                nexth = &IP6_HBH_NEXTH(hbh);
                len = ip6HeaderLen(hbh->_hlen);
                break;
            }

            case IP6_NEXTH_DESTOPTS: {
                ip6_dest_hdr* dsthdr = (ip6_dest_hdr*)p->payload;
                nexth = &IP6_DEST_NEXTH(dsthdr);
                len = ip6HeaderLen(dsthdr->_hlen);
                break;
            }

            case IP6_NEXTH_ROUTING: {
                ip6_rout_hdr* rhdr = (ip6_rout_hdr*)p->payload;
                nexth = &IP6_ROUT_NEXTH(rhdr);
                len = ip6HeaderLen(rhdr->_hlen);
                break;
            }

            case IP6_NEXTH_FRAGMENT: {
                LOG(WARN, "Fragmented IPv6 packets are not supported");
                goto cleanup;
            }
        }

        if (len == 0) {
            /* We are probably done */
            break;
        }

        if ((p->len < sizeof(uint8_t)) || (len > p->len)) {
            /* Ignore */
            goto cleanup;
        }

        headerLen += len;
        pbuf_remove_header(p, len);
    }

    proto = ipProtoToL4Protocol(*nexth);
    if (proto != L4_PROTO_NONE) {
        r = natInput(ip_current_src_addr(), ip_current_dest_addr(), proto, p, in, header);
    }

cleanup:
    pbuf_add_header_force(p, headerLen);
    return r;
}

int Nat64::natInput(const ip_addr_t* src, const ip_addr_t* dst, L4Protocol proto, pbuf* p, netif* in, void* ipheader) {
    IpTransportAddress srcAddr;
    IpTransportAddress dstAddr;
    srcAddr.setAddress(*src);
    dstAddr.setAddress(*dst);

    uint32_t protoLifetime = 0;

    switch (proto) {
        case L4_PROTO_UDP: {
            udp_hdr* udphdr = (udp_hdr*)p->payload;
            srcAddr.setPort(lwip_ntohs(udphdr->src));
            dstAddr.setPort(lwip_ntohs(udphdr->dest));
            protoLifetime = DEFAULT_UDP_NAT_LIFETIME;
            break;
        }
        case L4_PROTO_ICMP: {
            icmp_echo_hdr* icmphdr = (icmp_echo_hdr*)p->payload;
            srcAddr.setIcmpId(lwip_ntohs(icmphdr->id));
            dstAddr.setIcmpId(lwip_ntohs(icmphdr->id));
            protoLifetime = DEFAULT_ICMP_NAT_LIFETIME;
            break;
        }
        case L4_PROTO_TCP: {
            tcp_hdr* tcphdr = (tcp_hdr*)p->payload;
            srcAddr.setPort(lwip_ntohs(tcphdr->src));
            dstAddr.setPort(lwip_ntohs(tcphdr->dest));
            protoLifetime = DEFAULT_TCP_NAT_LIFETIME;
            break;
        }
        default: {
            /* Unhandled */
            return 0;
        }
    }

    LOG_DEBUG(TRACE, "NAT64 input (%s) %s#%u -> %s#%u", l4ProtocolToName(proto),
              IPADDR_NTOA(&srcAddr.address()), srcAddr.l4Id(),
              IPADDR_NTOA(&dstAddr.address()), dstAddr.l4Id());

    if (filter(srcAddr, dstAddr, in)) {
        /* Unhandled */
        return 0;
    }

    auto bib = lookupBib(in == rule_->inside() ? srcAddr : dstAddr, proto);
    if (!bib) {
        LOG_DEBUG(TRACE, "No BIB found, trying to create one");
        /* Not BIB found. Try to create a new one. */
        bib = addBib(srcAddr, dstAddr, proto, in);
    }

    SessionEntry* session = nullptr;

    if (bib) {
        LOG_DEBUG(TRACE, "BIB %s#%u <-> %s#%u",
                  IP4ADDR_NTOA(&bib->srcIn().address()), bib->srcIn().l4Id(),
                  IP4ADDR_NTOA(&bib->dstOut().address()), bib->dstOut().l4Id());

        if (proto == L4_PROTO_ICMP) {
            dstAddr.setIcmpId(bib->dstOut().icmpId());
        }

        /* Lookup session */
        session = bib->lookupSession(srcAddr, dstAddr);

        /* FIXME: flag to enable full-cone NAT */
        if (!session && in == rule_->inside()) {
            /* Attempt to create a new session */
            LOG_DEBUG(TRACE, "No matching session found, trying to create one");
            session = bib->addSession(dstAddr, protoLifetime, *pool_);
            if (!session) {
                LOG(ERROR, "failed to add session");
                dump();
            }
        } else if (!session) {
            LOG_DEBUG(WARN, "Not creating a new session, full-cone NAT is not enabled");
        }

        if (session) {
            LOG_DEBUG(TRACE, "Session %s#%u <-> %s#%u, %s#%u <-> %s#%u, %lums",
                      IP4ADDR_NTOA(&session->srcIn().address()), session->srcIn().l4Id(),
                      IP4ADDR_NTOA(&session->dstIn().address()), session->dstIn().l4Id(),
                      IP4ADDR_NTOA(&session->srcOut().address()), session->srcOut().l4Id(),
                      IP4ADDR_NTOA(&session->dstOut().address()), session->dstOut().l4Id(),
                      session->lifetime());
            session->setLifetime(protoLifetime);
        }
    } else {
        LOG_DEBUG(TRACE, "No matching BIB");
    }

    if (!session) {
        /* Unhandled */
        return 0;
    }

    /* Copy UDP packet */
    // pbuf* q = pbuf_clone(PBUF_IP, PBUF_RAM, p);
    // if (!q) {
    //     LOG_DEBUG(ERROR, "Failed to duplicate pkt for translation");
    //     /* Consume */
    //     return 1;
    // }
    pbuf* q = p;
    if (in == rule_->inside()) {
        srcAddr = session->srcOut();
        dstAddr = session->dstOut();
    } else {
        dstAddr = session->srcIn();
        srcAddr = session->dstIn();
    }

    if (proto == L4_PROTO_UDP) {
        udp_hdr* udphdr = (udp_hdr*)q->payload;
        udphdr->src = lwip_htons(srcAddr.port());
        udphdr->dest = lwip_htons(dstAddr.port());
        udphdr->chksum = 0x0000;
#if CHECKSUM_GEN_UDP
        udphdr->chksum = inet_chksum_pseudo(q, IP_PROTO_UDP, q->tot_len, ip_2_ip4(&srcAddr.address()), ip_2_ip4(&dstAddr.address()));
#endif /* CHECKSUM_GEN_UDP */
    } else if (proto == L4_PROTO_ICMP) {
        icmp_echo_hdr* icmphdr = (icmp_echo_hdr*)q->payload;
        icmphdr->id = lwip_htons(dstAddr.icmpId());
        icmphdr->chksum = 0x0000;
#if CHECKSUM_GEN_ICMP
        icmphdr->chksum = inet_chksum(icmphdr, q->len);
#endif /* CHECKSUM_GEN_ICMP */
    } else if (proto == L4_PROTO_TCP) {
        tcp_hdr* tcphdr = (tcp_hdr*)q->payload;
        tcphdr->src = lwip_htons(srcAddr.port());
        tcphdr->dest = lwip_htons(dstAddr.port());
        tcphdr->chksum = 0x0000;
#if CHECKSUM_GEN_TCP
        tcphdr->chksum = inet_chksum_pseudo(q, IP_PROTO_TCP, q->tot_len, ip_2_ip4(&srcAddr.address()), ip_2_ip4(&dstAddr.address()));
#endif /* CHECKSUM_GEN_TCP */
        if (TCPH_FLAGS(tcphdr) & (TCP_RST | TCP_FIN)) {
            // 4 minutes
            LOG_DEBUG(INFO, "TCP RST or FIN received, timeout in 4 minutes");
            session->setLifetime(4 * 60 * 1000);
        } else if ((TCPH_FLAGS(tcphdr) & (TCP_SYN | TCP_ACK | TCP_RST)) == (TCP_SYN) || (TCPH_FLAGS(tcphdr) & (TCP_SYN | TCP_ACK | TCP_RST)) == (TCP_SYN | TCP_RST)) {
            size_t hdrlen_bytes = TCPH_HDRLEN_BYTES(tcphdr);
            size_t optlen = (u16_t)(hdrlen_bytes - TCP_HLEN);
            char opts[20] = {};
            pbuf_copy_partial(q, opts, optlen, TCP_HLEN);
            for (char* o = opts; o - opts < (int)optlen;) {
                bool ex = false;
                switch (*o) {
                    case 0: {
                        ex = true;
                        break;
                    }
                    case 1: {
                        o++;
                        continue;
                    }
                    case 2: {
                        // TCP MSS
                        auto outif = ip4_route_src(ip_2_ip4(&srcAddr.address()), ip_2_ip4(&dstAddr.address()));
                        if (outif) {
                            uint16_t newMss = lwip_htons(outif->mtu - 40);
                            // uint16_t oldMss = lwip_ntohs(*(uint16_t*)(o + 2));
                            *(uint16_t*)(o + 2) = newMss;
                            // if (oldMss != newMss) {
                            //     LOG(WARN, "Adjusted MSS from %u to %u", (unsigned)oldMss, outif->mtu - 40);
                            // }
                        }
                        o += 4;
                        break;
                    }
                    default: {
                        size_t len = o[1];
                        o += len;
                        break;
                    }
                }
                if (ex) {
                    break;
                }
            }
        }
    }
    ip_hdr* ip4hdr = (ip_hdr*)ipheader;
    uint8_t hl = IPH_TTL(ip4hdr) - 1;
    if (hl == 0) {
        // pbuf_free(q);
        /* Consume */
        return 1;
    }
    /* Just in case */
    auto outif = ip4_route_src(ip_2_ip4(&srcAddr.address()), ip_2_ip4(&dstAddr.address()));
    if (outif != in) {
        ip4_output(q, ip_2_ip4(&srcAddr.address()),
            ip_2_ip4(&dstAddr.address()),
            hl, IPH_TOS(ip4hdr), proto);
    } else {
        LOG_DEBUG(WARN, "Not outputting translated packet on the same interface it was received");
        // if (rule_->inside()) {
        //     ip6_output_if_src(q, &src, &dst, hl, IPH_TOS(ip4hdr), proto, rule_->inside());
        // }

    }
    // ip4_output(q, ip_2_ip4(&srcAddr.address()),
    //         ip_2_ip4(&dstAddr.address()),
    //         hl, IPH_TOS(ip4hdr), proto);
    // pbuf_free(q);

    /* Packet handled by NAT64 */
    /* Consume */
    return 1;
}

bool Nat64::filter(const IpTransportAddress& src, const IpTransportAddress& dst, netif* in) const {
    if (!rule_ || !pool_) {
        return true;
    }

    if ((in != rule_->outside() && rule_->outside()) && in != rule_->inside()) {
        LOG_DEBUG(TRACE, "Filtered because input interface does not match inside or outside");
        return true;
    }

    if (ip_addr_isbroadcast(&dst.address(), in)) {
        LOG_DEBUG(TRACE, "Filtered, broadcast");
        return true;
    }
    
    return false;
}

BibEntry* Nat64::lookupBib(const IpTransportAddress& addr, L4Protocol proto) {
    BibTable& tbl = proto == L4_PROTO_UDP ? udpBibTable_ : (proto == L4_PROTO_TCP ? tcpBibTable_ : icmpBibTable_);
    for (auto entry = tbl.front(); entry != nullptr; entry = entry->next) {
        if (entry->matches(addr)) {
            return entry;
        }
    }
    return nullptr;
}

BibEntry* Nat64::addBib(const IpTransportAddress& src, const IpTransportAddress& dst, L4Protocol proto, netif* in) {
    BibTable& tbl = proto == L4_PROTO_UDP ? udpBibTable_ : (proto == L4_PROTO_TCP ? tcpBibTable_ : icmpBibTable_);

    if (rule_ && rule_->inside() != in) {
        LOG_DEBUG(TRACE, "Not creating a new BIB for a connection initiated from outside side");
        return nullptr;
    }

    if (rule_) {
        netif* out = rule_->outside();
        if (!out) {
            ip4_addr_t addr = {};
            addr = *ip_2_ip4(&dst.address());
            out = ip4_route(&addr);
        }
        if (out && out != in) {
            LOG_DEBUG(TRACE, "Outside interface %c%c%u", out->name[0], out->name[1], netif_get_index(out));
            const ip4_addr_t* t = netif_ip4_addr(out);
            if (!ip4_addr_isany(t)) {
                Ip4TransportAddress src4;
                src4.setAddress(*t);
                if (findNextL4Id(src4, proto)) {
                    if (pool_) {
                        BibEntry* bib = static_cast<BibEntry*>(pool_->alloc(NAT64_ENTRY_SIZE));
                        if (bib) {
                            new (bib) BibEntry(src, src4);
                            tbl.pushFront(bib);
                            return bib;
                        } else {
                            LOG_DEBUG(ERROR, "Failed to allocate new BIB");
                        }
                    }
                }
                LOG_DEBUG(ERROR, "Failed to find next l4 id");
                dump();
            } else {
                LOG_DEBUG(ERROR, "Failed to allocate new BIB, because there is no IPv4 address on the outside iface");
            }
        }
    }

    return nullptr;
}

bool Nat64::findNextL4Id(Ip4TransportAddress& src, L4Protocol proto) {
    if (proto == L4_PROTO_UDP) {
        return findNextUdpPort(src);
    } else if (proto == L4_PROTO_TCP) {
        return findNextTcpPort(src);
    } else if (proto == L4_PROTO_ICMP) {
        return findNextIcmpId(src);
    }

    return false;
}

bool Nat64::findNextUdpPort(Ip4TransportAddress& src) {
    uint16_t port = udpNextPort_;
    do {
        src.setPort(port);
        if (!lookupBib(src, L4_PROTO_UDP)) {
            udpNextPort_ = nextBoundId(port, DEFAULT_UDP_NAT_MIN_PORT, DEFAULT_UDP_NAT_MAX_PORT);
            return true;
        }

        port = nextBoundId(port, DEFAULT_UDP_NAT_MIN_PORT, DEFAULT_UDP_NAT_MAX_PORT);
    } while(port != udpNextPort_);

    return false;
}

bool Nat64::findNextTcpPort(Ip4TransportAddress& src) {
    uint16_t port = tcpNextPort_;
    do {
        src.setPort(port);
        if (!lookupBib(src, L4_PROTO_TCP)) {
            tcpNextPort_ = nextBoundId(port, DEFAULT_TCP_NAT_MIN_PORT, DEFAULT_TCP_NAT_MAX_PORT);
            return true;
        }

        port = nextBoundId(port, DEFAULT_TCP_NAT_MIN_PORT, DEFAULT_TCP_NAT_MAX_PORT);
    } while(port != tcpNextPort_);

    return false;
}

bool Nat64::findNextIcmpId(Ip4TransportAddress& src) {
    uint16_t id = icmpNextId_;
    do {
        src.setIcmpId(id);
        if (!lookupBib(src, L4_PROTO_ICMP)) {
            icmpNextId_ = nextBoundId(id, DEFAULT_ICMP_NAT_MIN_ID, DEFAULT_ICMP_NAT_MAX_ID);
            return true;
        }

        id = nextBoundId(id, DEFAULT_ICMP_NAT_MIN_ID, DEFAULT_ICMP_NAT_MAX_ID);
    } while(id != icmpNextId_);

    return false;
}

void Nat64::timeout(uint32_t dt) {
    for (auto bib = udpBibTable_.front(), p = static_cast<BibEntry*>(nullptr); bib != nullptr;) {
        if (bib->timeout(dt, *pool_)) {
            LOG_DEBUG(TRACE, "UDP BIB %s#%u <-> %s#%u timed out",
                      IP4ADDR_NTOA(&bib->srcIn().address()), bib->srcIn().l4Id(),
                      IP4ADDR_NTOA(&bib->dstOut().address()), bib->dstOut().l4Id());

            auto popped = udpBibTable_.pop(bib, p);
            bib = popped->next;
            pool_->free(popped);
        } else {
            p = bib;
            bib = bib->next;
        }
    }

    for (auto bib = icmpBibTable_.front(), p = static_cast<BibEntry*>(nullptr); bib != nullptr;) {
        if (bib->timeout(dt, *pool_)) {
            LOG_DEBUG(TRACE, "ICMP BIB %s#%u <-> %s#%u timed out",
                      IP4ADDR_NTOA(&bib->srcIn().address()), bib->srcIn().l4Id(),
                      IP4ADDR_NTOA(&bib->dstOut().address()), bib->dstOut().l4Id());

            auto popped = icmpBibTable_.pop(bib, p);
            bib = popped->next;
            pool_->free(popped);
        } else {
            p = bib;
            bib = bib->next;
        }
    }

    for (auto bib = tcpBibTable_.front(), p = static_cast<BibEntry*>(nullptr); bib != nullptr;) {
        if (bib->timeout(dt, *pool_)) {
            LOG_DEBUG(TRACE, "TCP BIB %s#%u <-> %s#%u timed out",
                      IP4ADDR_NTOA(&bib->srcIn().address()), bib->srcIn().l4Id(),
                      IP4ADDR_NTOA(&bib->dstOut().address()), bib->dstOut().l4Id());

            auto popped = tcpBibTable_.pop(bib, p);
            bib = popped->next;
            pool_->free(popped);
        } else {
            p = bib;
            bib = bib->next;
        }
    }
}

void Nat64::dump() {
    for (auto bib = udpBibTable_.front(); bib != nullptr;) {
        LOG_DEBUG(TRACE, "UDP BIB %s#%u <-> %s#%u",
                    IP4ADDR_NTOA(&bib->srcIn().address()), bib->srcIn().l4Id(),
                    IP4ADDR_NTOA(&bib->dstOut().address()), bib->dstOut().l4Id());
        for (auto s = bib->sessions_.front(); s != nullptr;) {
            LOG_DEBUG(TRACE, "Session %s#%u <-> %s#%u, %s#%u <-> %s#%u lifetime=%u",
                      IP4ADDR_NTOA(&s->srcIn().address()), s->srcIn().l4Id(),
                      IP4ADDR_NTOA(&s->dstIn().address()), s->dstIn().l4Id(),
                      IP4ADDR_NTOA(&s->srcOut().address()), s->srcOut().l4Id(),
                      IP4ADDR_NTOA(&s->dstOut().address()), s->dstOut().l4Id(),
                      s->lifetime());
            s = s->next;
        }
        bib = bib->next;
    }

    for (auto bib = tcpBibTable_.front(); bib != nullptr;) {
        LOG_DEBUG(TRACE, "TCP BIB %s#%u <-> %s#%u",
                    IP4ADDR_NTOA(&bib->srcIn().address()), bib->srcIn().l4Id(),
                    IP4ADDR_NTOA(&bib->dstOut().address()), bib->dstOut().l4Id());
        for (auto s = bib->sessions_.front(); s != nullptr;) {
            LOG_DEBUG(TRACE, "Session %s#%u <-> %s#%u, %s#%u <-> %s#%u lifetime=%u",
                      IP4ADDR_NTOA(&s->srcIn().address()), s->srcIn().l4Id(),
                      IP4ADDR_NTOA(&s->dstIn().address()), s->dstIn().l4Id(),
                      IP4ADDR_NTOA(&s->srcOut().address()), s->srcOut().l4Id(),
                      IP4ADDR_NTOA(&s->dstOut().address()), s->dstOut().l4Id(),
                      s->lifetime());
            s = s->next;
        }
        bib = bib->next;
    }

    for (auto bib = icmpBibTable_.front(); bib != nullptr;) {
        LOG_DEBUG(TRACE, "ICMP BIB %s#%u <-> %s#%u",
                    IP4ADDR_NTOA(&bib->srcIn().address()), bib->srcIn().l4Id(),
                    IP4ADDR_NTOA(&bib->dstOut().address()), bib->dstOut().l4Id());
        for (auto s = bib->sessions_.front(); s != nullptr;) {
            LOG_DEBUG(TRACE, "Session %s#%u <-> %s#%u, %s#%u <-> %s#%u lifetime=%u",
                      IP4ADDR_NTOA(&s->srcIn().address()), s->srcIn().l4Id(),
                      IP4ADDR_NTOA(&s->dstIn().address()), s->dstIn().l4Id(),
                      IP4ADDR_NTOA(&s->srcOut().address()), s->srcOut().l4Id(),
                      IP4ADDR_NTOA(&s->dstOut().address()), s->dstOut().l4Id(),
                      s->lifetime());
            s = s->next;
        }
        bib = bib->next;
    }
}

void Nat64::enableSessionTimer() {
    LwipTcpIpCoreLock lock;
    timeoutHandlerCb(this);
}

void Nat64::disableSessionTimer() {
    LwipTcpIpCoreLock lock;
    sys_untimeout(&timeoutHandlerCb, this);
}

void Nat64::timeoutHandlerCb(void* arg) {
    auto self = static_cast<Nat64*>(arg);
    self->timeout(DEFAULT_SESSION_CLEANUP_TIMEOUT);
    sys_timeout(DEFAULT_SESSION_CLEANUP_TIMEOUT, &timeoutHandlerCb, self);
}
