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

#include "dns64.h"

#include "socket_hal_posix.h"

#include "system_error.h"
#include "logging.h"

#include "lwiplock.h"
#include "lwip_util.h"

#include "lwip/dns.h"

LOG_SOURCE_CATEGORY("net.dns64")

#ifndef DEBUG_DNS64
#define DEBUG_DNS64 0
#endif

// TODO: Move this macro to a global header
#define CHECK(_expr) \
        ({ \
            const auto _ret = _expr; \
            if (_ret < 0) { \
                return _ret; \
            } \
            _ret; \
        })

#define CHECK_SOCKET(_expr) \
        ({ \
            const auto _ret = _expr; \
            if (_ret < 0) { \
                LOG_DEBUG(ERROR, #_expr " failed: %d", errno); \
                return socketToSystemError(errno); \
            } \
            _ret; \
        })

#undef DEBUG // Legacy logging macro

#if DEBUG_DNS64
#define DEBUG(...) LOG_DEBUG(TRACE, __VA_ARGS__)
#else
#define DEBUG(...)
#endif

namespace particle {

namespace net {

namespace {

// Message header flags
enum HeaderFlag { // RFC 1035, 4.1.1
    QR = 0x8000,
    OPCODE_MASK = 0x7800,
    OPCODE_QUERY = 0x0000,
    OPCODE_IQUERY = 0x0800,
    OPCODE_STATUS = 0x1000,
    AA = 0x0400,
    TC = 0x0200,
    RD = 0x0100,
    RA = 0x0080,
    Z_MASK = 0x0070,
    RCODE_MASK = 0x000f,
    RCODE_NO_ERROR = 0x0000,
    RCODE_FORMAT_ERROR = 0x0001,
    RCODE_SERVER_FAILURE = 0x0002,
    RCODE_NAME_ERROR = 0x0003, // aka NXDOMAIN
    RCODE_NOT_IMPLEMENTED = 0x0004,
    RCODE_REFUSED = 0x0005
};

// TYPE/QTYPE values
enum Type {
    A = 1, // RFC 1035, 3.2.2
    AAAA = 28, // RFC 3596, 2.1
    TYPE_ANY = 255 // RFC 1035, 3.2.3
};

// CLASS/QCLASS values
enum Class {
    IN = 1, // RFC 1035, 3.2.4
    CLASS_ANY = 255 // RFC 1035, 3.2.5
};

// Message header
struct Header { // RFC 1035, 4.1.1
    uint16_t id;
    uint16_t flags; // See HeaderFlag
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} __attribute__((packed));

// Fixed-size part of a question entry
struct Question { // RFC 1035, 4.1.2
    uint16_t qtype;
    uint16_t qcls;
} __attribute__((packed));

// Fixed-size part of a resource record
struct Record { // RFC 1035, 4.1.3
    uint16_t type;
    uint16_t cls;
    uint32_t ttl;
    uint16_t rdlength;
} __attribute__((packed));

// Maximum size of a UDP message
const size_t MAX_MESSAGE_SIZE = 512; // RFC 1035, 2.3.4

// Value of the TTL field sent in all response messages
const uint32_t DEFAULT_TTL = 0; // Do not cache (RFC 1035, 4.1.3)

// Timeout for select() in milliseconds
const unsigned SOCKET_RECV_TIMEOUT = 1000;

ssize_t readHeader(const char* data, size_t size, Header* h) {
    if (size < sizeof(Header)) {
        LOG_DEBUG(ERROR, "Unexpected end of message");
        return SYSTEM_ERROR_BAD_DATA;
    }
    memcpy(h, data, sizeof(Header));
    h->id = lwip_ntohs(h->id);
    h->flags = lwip_ntohs(h->flags);
    h->qdcount = lwip_ntohs(h->qdcount);
    h->ancount = lwip_ntohs(h->ancount);
    h->nscount = lwip_ntohs(h->nscount);
    h->arcount = lwip_ntohs(h->arcount);
    return sizeof(Header);
}

ssize_t writeHeader(char* data, size_t size, const Header& h) {
    if (size < sizeof(Header)) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    Header hh = {};
    hh.id = lwip_htons(h.id);
    hh.flags = lwip_htons(h.flags);
    hh.qdcount = lwip_htons(h.qdcount);
    hh.ancount = lwip_htons(h.ancount);
    hh.nscount = lwip_htons(h.nscount);
    hh.arcount = lwip_htons(h.arcount);
    memcpy(data, &hh, sizeof(Header));
    return sizeof(Header);
}

ssize_t readQuestion(const char* data, size_t size, Question* q) {
    if (size < sizeof(Question)) {
        LOG_DEBUG(ERROR, "Unexpected end of message");
        return SYSTEM_ERROR_BAD_DATA;
    }
    memcpy(q, data, sizeof(Question));
    q->qtype = lwip_ntohs(q->qtype);
    q->qcls = lwip_ntohs(q->qcls);
    return sizeof(Question);
}

ssize_t writeQuestion(char* data, size_t size, const Question& q) {
    if (size < sizeof(Question)) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    Question qq = {};
    qq.qtype = lwip_htons(q.qtype);
    qq.qcls = lwip_htons(q.qcls);
    memcpy(data, &qq, sizeof(Question));
    return sizeof(Question);
}

ssize_t writeRecord(char* data, size_t size, const Record& r) {
    if (size < sizeof(Record)) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    Record rr = {};
    rr.type = lwip_htons(r.type);
    rr.cls = lwip_htons(r.cls);
    rr.ttl = lwip_htons(r.ttl);
    rr.rdlength = lwip_htons(r.rdlength);
    memcpy(data, &rr, sizeof(Record));
    return sizeof(Record);
}

// Note: This function modifies the source buffer
ssize_t readName(char* data, size_t size, const char** name) {
    const auto src = data;
    for (;;) {
        if (size < 1) {
            LOG_DEBUG(ERROR, "Unexpected end of message");
            return SYSTEM_ERROR_BAD_DATA;
        }
        const size_t n = (uint8_t)*data; // Label length
        if (!n) {
            ++data;
            break;
        }
        if ((n & 0xc0) != 0) { // RFC 1035, 4.1.4
            LOG_DEBUG(ERROR, "Message compression is not supported");
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
        if (size < n + 1) {
            LOG_DEBUG(ERROR, "Unexpected end of message");
            return SYSTEM_ERROR_BAD_DATA;
        }
        *data = '.';
        data += n + 1;
        size -= n + 1;
    }
    *name = src + 1;
    return data - src;
}

ssize_t writeName(char* data, size_t size, const char* name) {
    const auto dest = data;
    auto label = name;
    for (;;) {
        const char c = *name;
        if (c == '.' || c == '\0') {
            const size_t n = name - label; // Label length
            if (n == 0 || n > 63) { // RFC 1035, 2.3.4
                return SYSTEM_ERROR_BAD_DATA;
            }
            if (size < n + 1) {
                return SYSTEM_ERROR_TOO_LARGE;
            }
            *data = n;
            memcpy(data + 1, label, n);
            data += n + 1;
            size -= n + 1;
            if (c == '\0') {
                if (size < 1) {
                    return SYSTEM_ERROR_TOO_LARGE;
                }
                *data++ = 0;
                break;
            }
            label = name + 1;
        }
        ++name;
    }
    return data - dest;
}

ssize_t writeCompressedName(char* data, size_t size, size_t offs) {
    if (size < 2) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    data[0] = ((offs >> 8) & 0xff) | 0xc0; // RFC 1035, 4.1.4
    data[1] = offs & 0xff;
    return 2;
}

ip6_addr_t transformAddress(const ip4_addr_t& src, const ip6_addr_t& prefix) {
    ip6_addr_t dest = {};
    ip6_addr_copy(dest, prefix);
    dest.addr[3] = src.addr;
    return dest;
}

int socketToSystemError(int error) {
    return SYSTEM_ERROR_IO; // TODO
}

int lwipToSystemError(int error) {
    return SYSTEM_ERROR_NETWORK; // TODO
}

uint16_t systemToDnsError(int error) {
    switch (error) {
    case SYSTEM_ERROR_NOT_FOUND:
        return RCODE_NAME_ERROR;
    case SYSTEM_ERROR_BAD_DATA:
        return HeaderFlag::RCODE_FORMAT_ERROR;
    case SYSTEM_ERROR_NOT_ALLOWED:
        return HeaderFlag::RCODE_REFUSED;
    case SYSTEM_ERROR_NOT_SUPPORTED:
        return HeaderFlag::RCODE_NOT_IMPLEMENTED;
    default:
        return HeaderFlag::RCODE_SERVER_FAILURE;
    }
}

} // particle::net::

struct Dns64::Context {
    ip6_addr_t prefix;
    int sock;

    Context() :
            sock(-1) {
    }

    ~Context() {
        if (sock >= 0 && sock_close(sock) != 0) {
            LOG(ERROR, "Unable to close socket");
        }
    }
};

struct Dns64::Query {
    std::weak_ptr<Context> ctx;
    sockaddr_in6 srcAddr;
    Header h;
    Question q;
    uint16_t type;
};

int Dns64::init(if_t iface, const ip6_addr_t& prefix, uint16_t port) {
    // Initialize the context
    ctx_.reset(new(std::nothrow) Context);
    if (!ctx_) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    ctx_->prefix = prefix;
    // Allocate a buffer for query data
    buf_.reset(new(std::nothrow) char[MAX_MESSAGE_SIZE]);
    if (!buf_) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    // Create a socket
    ctx_->sock = CHECK_SOCKET(sock_socket(AF_INET6, SOCK_DGRAM, 0));
    // Set socket options
    int opt = 1;
    CHECK_SOCKET(sock_setsockopt(ctx_->sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)));
    // Bind the socket
    char name[IF_NAMESIZE] = {};
    CHECK(if_get_name(iface, name));
    CHECK_SOCKET(sock_setsockopt(ctx_->sock, SOL_SOCKET, SO_BINDTODEVICE, name, sizeof(name)));
    sockaddr_in6 addr = {};
    addr.sin6_len = sizeof(sockaddr_in6);
    addr.sin6_family = AF_INET6;
    addr.sin6_port = lwip_htons(port);
    addr.sin6_flowinfo = 0;
    addr.sin6_addr = in6addr_any;
    addr.sin6_scope_id = 0;
    CHECK_SOCKET(sock_bind(ctx_->sock, (const struct sockaddr*)&addr, sizeof(addr)));
    return 0;
}

void Dns64::destroy() {
    buf_.reset();
    ctx_.reset();
}

int Dns64::run() {
    if (!ctx_) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    fd_set fd;
    FD_ZERO(&fd);
    FD_SET(ctx_->sock, &fd);
    timeval tv = {};
    tv.tv_usec = SOCKET_RECV_TIMEOUT * 1000;
    int r = lwip_select(ctx_->sock + 1, &fd, nullptr, nullptr, &tv);
    if (r < 0) {
        LOG(ERROR, "select() failed: %d", errno);
        destroy();
        return socketToSystemError(errno);
    }
    if (r == 0) {
        return 0;
    }
    const LwipTcpIpCoreLock lock;
    sockaddr_in6 srcAddr = {};
    socklen_t addrSize = sizeof(srcAddr);
    const ssize_t n = sock_recvfrom(ctx_->sock, buf_.get(), MAX_MESSAGE_SIZE, 0, (sockaddr*)&srcAddr, &addrSize);
    if (n < 0) {
        if (errno == EWOULDBLOCK) {
            return 0; // Timeout
        }
        LOG(ERROR, "sock_recvfrom() failed: %d", errno);
        destroy();
        return socketToSystemError(errno);
    }
    r = processQuery(buf_.get(), n, srcAddr);
    if (r < 0) {
        LOG_DEBUG(ERROR, "Unable to process query: %d", r);
    }
    return 0;
}

int Dns64::processQuery(char* data, size_t size, const sockaddr_in6& srcAddr) {
    // Allocate a query object
    std::unique_ptr<Query> q(new(std::nothrow) Query());
    if (!q) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    q->srcAddr = srcAddr;
    q->ctx = ctx_;
    // Parse the query
    const char* name = nullptr;
    int ret = parseQuery(data, size, q.get(), &name);
    if (ret == 0) {
        // Perform a DNS lookup
        q->type = q->q.qtype; // Try getting an address of the requested type first
        ip_addr_t addr = {};
        ret = getHostByName(name, &addr, q.get());
        if (ret == GetHostByNameResult::DONE) {
            ret = sendResponse(addr, name, *q, ctx_.get());
            if (ret < 0) {
                LOG_DEBUG(ERROR, "Unable to send response: %d", ret);
            }
        } else if (ret == GetHostByNameResult::PENDING) {
            q.release(); // The query is being processed asynchronously
        } else {
            LOG_DEBUG(ERROR, "Unable to resolve hostname: %d", ret);
        }
    }
    if (ret < 0) {
        const int r = sendErrorResponse(ret, name, *q, ctx_.get());
        if (r < 0) {
            LOG_DEBUG(WARN, "Unable to send error response: %d", r);
        }
    }
    return ret;
}

int Dns64::parseQuery(char* data, size_t size, Query* q, const char** name) {
    const auto end = data + size;
    // Parse the header section
    data += CHECK(readHeader(data, end - data, &q->h));
    if ((q->h.flags & HeaderFlag::QR) != 0) {
        LOG_DEBUG(ERROR, "Invalid message type (QR)");
        return SYSTEM_ERROR_BAD_DATA;
    }
    if ((q->h.flags & HeaderFlag::OPCODE_MASK) != HeaderFlag::OPCODE_QUERY) {
        LOG_DEBUG(ERROR, "Unsupported query type (OPCODE)");
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    if (q->h.qdcount != 1) {
        LOG_DEBUG(ERROR, "Queries with multiple entries in the question section are not supported (QDCOUNT)");
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    // Parse the question section
    data += CHECK(readName(data, end - data, name));
    CHECK(readQuestion(data, end - data, &q->q));
    if (q->q.qtype == Type::TYPE_ANY) {
        q->q.qtype = Type::AAAA; // Prefer IPv6 by default
    } else if (q->q.qtype != Type::A && q->q.qtype != Type::AAAA) {
        LOG_DEBUG(ERROR, "Unsupported query type (QTYPE)");
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    if (q->q.qcls != Class::IN && q->q.qcls != Class::CLASS_ANY) {
        LOG_DEBUG(ERROR, "Unsupported query class (QCLASS)");
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    DEBUG("Received query: QNAME: %s, QTYPE: %d", *name, (int)q->q.qtype);
    return 0;
}

int Dns64::sendResponse(const ip_addr_t& addr, const char* name, const Query& q, Context* ctx) {
    ip_addr_t raddr = {}; // Resolved address
    if (q.q.qtype == Type::AAAA && IP_IS_V4(&addr)) {
        const auto addr6 = transformAddress(*ip_2_ip4(&addr), ctx->prefix);
        ip_addr_copy_from_ip6(raddr, addr6);
    } else {
        ip_addr_copy(raddr, addr);
    }
    const size_t addrSize = IPADDR_SIZE(&raddr); // Size of the serialized IP address
    // Allocate a buffer for response data
    const size_t qnameSize = strlen(name) + 2; // Including the length and term. null bytes
    const size_t size = sizeof(Header) /* ID ... ARCOUNT */ + qnameSize /* QNAME */ +
            sizeof(Question) /* QTYPE ... QCLASS */ + 2 /* NAME (compressed) */ +
            sizeof(Record) /* TYPE ... RDLENGTH */ + addrSize /* RDATA */ ;
    std::unique_ptr<char[]> buf(new(std::nothrow) char[size]);
    if (!buf) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    char* data = buf.get();
    char* const end = data + size;
    // Serialize the header section
    Header h = {};
    h.id = q.h.id;
    h.flags = HeaderFlag::QR | (q.h.flags & HeaderFlag::OPCODE_MASK) | (q.h.flags & HeaderFlag::RD) |
            HeaderFlag::RCODE_NO_ERROR;
    h.qdcount = 1;
    h.ancount = 1;
    data += CHECK(writeHeader(data, end - data, h));
    // Serialize the question section
    const size_t nameOffs = data - buf.get();
    data += CHECK(writeName(data, end - data, name));
    data += CHECK(writeQuestion(data, end - data, q.q));
    // Serialize the answer section
    data += CHECK(writeCompressedName(data, end - data, nameOffs));
    Record r = {};
    r.type = IP_IS_V6(&raddr) ? Type::AAAA : Type::A;
    r.cls = Class::IN;
    r.ttl = DEFAULT_TTL;
    r.rdlength = addrSize;
    data += CHECK(writeRecord(data, end - data, r));
    if (end - data < (ptrdiff_t)addrSize) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    memcpy(data, IPADDR_DATA(&raddr), addrSize);
    // Send the response
    CHECK_SOCKET(sock_sendto(ctx->sock, buf.get(), size, MSG_DONTWAIT, (const sockaddr*)&q.srcAddr, sizeof(q.srcAddr)));
    return 0;
}

int Dns64::sendErrorResponse(int error, const char* name, const Query& q, Context* ctx) {
    // Allocate a buffer for response data
    size_t qsize = 0; // Size of the question section
    if (name) {
        const size_t qnameSize = strlen(name) + 2; // Including the length and term. null bytes
        qsize = qnameSize /* QNAME */ + sizeof(Question) /* QTYPE ... QCLASS */ ;
    }
    const size_t size = sizeof(Header) /* ID ... ARCOUNT */ + qsize;
    std::unique_ptr<char[]> buf(new(std::nothrow) char[size]);
    if (!buf) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    char* data = buf.get();
    char* const end = data + size;
    // Serialize the header section
    const auto rcode = systemToDnsError(error);
    Header h = {};
    h.id = q.h.id;
    h.flags = HeaderFlag::QR | (q.h.flags & HeaderFlag::OPCODE_MASK) | (q.h.flags & HeaderFlag::RD) | rcode;
    h.qdcount = name ? 1 : 0;
    data += CHECK(writeHeader(data, end - data, h));
    if (name) {
        // Serialize the question section
        data += CHECK(writeName(data, end - data, name));
        data += CHECK(writeQuestion(data, end - data, q.q));
    }
    // Send the response
    CHECK_SOCKET(sock_sendto(ctx->sock, buf.get(), size, MSG_DONTWAIT, (const sockaddr*)&q.srcAddr, sizeof(q.srcAddr)));
    return 0;
}

int Dns64::getHostByName(const char* name, ip_addr_t* addr, Query* q) {
    const uint8_t addrType = (q->type == Type::A) ? LWIP_DNS_ADDRTYPE_IPV4 : LWIP_DNS_ADDRTYPE_IPV6;
    LwipTcpIpCoreLock lock; // LwIP's DNS client API is not thread-safe
    const auto lwipRet = dns_gethostbyname_addrtype(name, addr, Dns64::dnsCallback, q, addrType);
    lock.unlock();
    if (lwipRet == ERR_INPROGRESS) {
        return GetHostByNameResult::PENDING;
    } else if (lwipRet != ERR_OK) {
        return lwipToSystemError(lwipRet);
    }
    return GetHostByNameResult::DONE;
}

void Dns64::dnsCallback(const char* name, const ip_addr_t* addr, void* data) {
    DEBUG("dns_found_callback: name: %s, address: %s", name ? name : "NULL", addr ? IPADDR_NTOA(addr) : "NULL");
    std::unique_ptr<Query> q(static_cast<Query*>(data));
    if (!name) {
        return;
    }
    const auto ctx = q->ctx.lock();
    if (!ctx) {
        return;
    }
    int ret = 0;
    if (addr) {
        ret = sendResponse(*addr, name, *q, ctx.get());
    } else if (q->type == Type::AAAA) {
        q->type = Type::A; // Try getting an IPv4 address
        ip_addr_t addr = {};
        ret = getHostByName(name, &addr, q.get());
        if (ret == GetHostByNameResult::DONE) {
            ret = sendResponse(addr, name, *q, ctx.get());
            if (ret < 0) {
                LOG_DEBUG(ERROR, "Unable to send response: %d", ret);
            }
        } else if (ret == GetHostByNameResult::PENDING) {
            q.release(); // The query is being processed asynchronously
        } else {
            LOG_DEBUG(ERROR, "Unable to resolve hostname: %d", ret);
        }
    } else {
        ret = SYSTEM_ERROR_NOT_FOUND;
    }
    if (ret < 0) {
        ret = sendErrorResponse(ret, name, *q, ctx.get());
        if (ret < 0) {
            LOG_DEBUG(WARN, "Unable to send error response: %d", ret);
        }
    }
}

} // particle::net

} // particle
