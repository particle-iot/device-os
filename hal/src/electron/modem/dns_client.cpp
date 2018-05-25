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

// This code is based on LwIP's DNS client implementation

/*
 * Port to lwIP from uIP
 * by Jim Pettinato April 2007
 *
 * security fixes and more by Simon Goldschmidt
 *
 * uIP version Copyright (c) 2002-2003, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HAL_CELLULAR_EXCLUDE

#include "dns_client.h"

#include "mdm_hal.h"
#include <stdio.h>
#include "timer_hal.h"
#include "delay_hal.h"
#include "system_error.h"

#include <memory>

extern MDMElectronSerial electronMDM;

namespace particle {

namespace {

// DNS server address
const MDM_IP SERVER_ADDRESS = IPADR(8, 8, 8, 8);
const uint16_t SERVER_PORT = 53;
// Maximum host name length
const size_t MAX_NAME_LENGTH = 255;
// Maximum number of retries
const unsigned MAX_RETRIES = 3;
// Buffer size
const size_t BUFFER_SIZE = 512; // See RFC 1035 – 4.2.1. UDP usage

// DNS protocol flags
const unsigned DNS_FLAG1_RD = 0x01;
const unsigned DNS_FLAG1_RESPONSE = 0x80;
const unsigned DNS_FLAG2_ERR_MASK = 0x0f;

// TYPE and CLASS fields
const unsigned DNS_RRTYPE_A = 1; // A host address
const unsigned DNS_RRCLASS_IN = 1; // The Internet

// Last used query ID
uint16_t g_queryId = 0;

struct DnsHeader {
  uint16_t id;
  uint8_t flags1;
  uint8_t flags2;
  uint16_t numquestions;
  uint16_t numanswers;
  uint16_t numauthrr;
  uint16_t numextrarr;
} __attribute__((packed));

struct DnsTypeClass {
  uint16_t type;
  uint16_t cls;
} __attribute__((packed));

struct DnsAnswer {
    uint16_t type;
    uint16_t cls;
    uint32_t ttl;
    uint16_t len;
} __attribute__((packed));

struct DnsQuery {
    std::unique_ptr<char[]> buf;
    const char* name;
    MDM_IP addr;
    uint16_t id;

    explicit DnsQuery(const char* name) :
            name(name),
            addr(NOIP),
            id(0) {
    }
};

class UdpSocket {
public:
    UdpSocket() :
            sock_(-1) {
    }

    ~UdpSocket() {
        close();
    }

    int open() {
        close();
        const int sock = electronMDM.socketSocket(MDM_IPPROTO_UDP);
        if (sock < 0) {
            return sock;
        }
        if (!electronMDM.socketSetBlocking(sock, 0)) {
            electronMDM.socketFree(sock);
            return MDM_SOCKET_ERROR;
        }
        sock_ = sock;
        return 0;
    }

    void close() {
        if (sock_ >= 0) {
            electronMDM.socketFree(sock_);
            sock_ = -1;
        }
    }

    int sendTo(MDM_IP addr, int port, const char* data, size_t size) {
        return electronMDM.socketSendTo(sock_, addr, port, data, size);
    }

    int recvFrom(MDM_IP* addr, int* port, char* data, size_t size) {
        return electronMDM.socketRecvFrom(sock_, addr, port, data, size);
    }

private:
    int sock_;
};

inline uint16_t htons(uint16_t val) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return val;
#else
    return __builtin_bswap16(val);
#endif
}

inline uint16_t ntohs(uint16_t val) {
    return htons(val);
}

inline uint32_t htonl(uint32_t val) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return val;
#else
    return __builtin_bswap32(val);
#endif
}

inline uint32_t ntohl(uint32_t val) {
    return htonl(val);
}

int dnsCompareName(const char* name, const char* data, size_t size) {
    size_t offs = 0;
    uint8_t n = 0;
    do {
        if (offs >= size) {
            return -1;
        }
        n = data[offs++];
        // See RFC 1035 - 4.1.4. Message compression
        if ((n & 0xc0) == 0xc0) {
            // Compressed name: cannot be equal since we don't send them
            return -1;
        } else {
            // Not compressed name
            while (n > 0) {
                if (offs >= size) {
                    return -1;
                }
                const char c = data[offs];
                if (*name != c) {
                    return -1;
                }
                ++offs;
                ++name;
                --n;
            }
            ++name;
        }
        if (offs >= size) {
            return -1;
        }
        n = data[offs];
    } while (n != 0);
    return offs + 1;
}

int dnsSkipName(const char* data, size_t size) {
    size_t offs = 0;
    uint8_t n = 0;
    do {
        if (offs >= size) {
            return -1;
        }
        n = data[offs++];
        // See RFC 1035 - 4.1.4. Message compression
        if ((n & 0xc0) == 0xc0) {
            // Compressed name: since we only want to skip it (not check it), stop here
            break;
        } else {
            // Not compressed name
            offs += n;
            if (offs >= size) {
                return -1;
            }
        }
        if (offs >= size) {
            return -1;
        }
        n = data[offs];
    } while (n != 0);
    return offs + 1;
}

int recvDnsResponse(UdpSocket* sock, DnsQuery* q) {
    MDM_IP addr = NOIP;
    int port = 0;
    int ret = sock->recvFrom(&addr, &port, q->buf.get(), BUFFER_SIZE);
    if (ret <= 0) {
        return ret;
    }
    const size_t packetSize = ret;
    if (packetSize > BUFFER_SIZE) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    if (addr != SERVER_ADDRESS) {
        return 0; // Ignore packet
    }
    // Is the DNS message big enough?
    if (packetSize < sizeof(DnsHeader)) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    DnsHeader hdr = {};
    memcpy(&hdr, q->buf.get(), sizeof(DnsHeader));
    hdr.id = ntohs(hdr.id);
    if (hdr.id != q->id) {
        return 0; // Ignore packet
    }
    // Check for errors
    if (hdr.flags2 & DNS_FLAG2_ERR_MASK) {
        return SYSTEM_ERROR_UNKNOWN;
    }
    hdr.numquestions = ntohs(hdr.numquestions);
    if (!(hdr.flags1 & DNS_FLAG1_RESPONSE) || hdr.numquestions != 1) {
        return SYSTEM_ERROR_BAD_DATA; // Unexpected response data
    }
    // Check if the question section matches the query
    size_t offs = sizeof(DnsHeader);
    ret = dnsCompareName(q->name, q->buf.get() + offs, packetSize - offs);
    if (ret < 0) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    offs += ret;
    if (packetSize - offs < sizeof(DnsTypeClass)) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    DnsTypeClass tc = {};
    memcpy(&tc, q->buf.get() + offs, sizeof(DnsTypeClass));
    tc.cls = ntohs(tc.cls);
    tc.type = ntohs(tc.type);
    if (tc.cls != DNS_RRCLASS_IN || tc.type != DNS_RRTYPE_A) {
        return SYSTEM_ERROR_BAD_DATA;
    }
    offs += sizeof(DnsTypeClass);
    // Process the answer section
    uint16_t nanswers = ntohs(hdr.numanswers);
    while (nanswers > 0 && offs < packetSize) {
        // Skip resource record's host name
        int ret = dnsSkipName(q->buf.get() + offs, packetSize - offs);
        if (ret < 0) {
            return SYSTEM_ERROR_BAD_DATA;
        }
        offs += ret;
        if (packetSize - offs < sizeof(DnsAnswer)) {
            return SYSTEM_ERROR_BAD_DATA;
        }
        DnsAnswer answer = {};
        memcpy(&answer, q->buf.get() + offs, sizeof(DnsAnswer));
        offs += sizeof(DnsAnswer);
        answer.cls = ntohs(answer.cls);
        answer.type = ntohs(answer.type);
        answer.len = ntohs(answer.len);
        if (answer.cls == DNS_RRCLASS_IN && answer.type == DNS_RRTYPE_A && answer.len == 4) {
            if (packetSize - offs < 4) {
                return SYSTEM_ERROR_BAD_DATA;
            }
            uint32_t addr = 0;
            memcpy(&addr, q->buf.get() + offs, 4);
            q->addr = ntohl(addr);
            return 0;
        }
        // Skip this answer
        offs += answer.len;
        --nanswers;
    }
    return SYSTEM_ERROR_NOT_FOUND;
}

int sendDnsQuery(UdpSocket* sock, DnsQuery* q) {
    SPARK_ASSERT(sock && q);
    const size_t packetSize = sizeof(DnsHeader) + strlen(q->name) + 2 + sizeof(DnsTypeClass);
    if (packetSize > BUFFER_SIZE) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    q->id = ++g_queryId; // FIXME: Use random IDs
    // Fill the header section
    DnsHeader hdr = {};
    hdr.id = htons(q->id);
    hdr.flags1 = DNS_FLAG1_RD; // Recursion Desired
    hdr.numquestions = htons(1);
    memcpy(q->buf.get(), &hdr, sizeof(hdr));
    // Fill the question section
    const char* name = q->name - 1;
    size_t offs = sizeof(DnsHeader);
    do {
        ++name;
        const char* namePart = name;
        size_t n = 0;
        for (; *name != '.' && *name != '\0'; ++name) {
            ++n;
        }
        q->buf[offs] = n;
        memcpy(q->buf.get() + offs + 1, namePart, name - namePart);
        offs += n + 1;
    } while (*name != '\0');
    q->buf[offs] = '\0';
    ++offs;
    DnsTypeClass tc = {};
    tc.type = htons(DNS_RRTYPE_A);
    tc.cls = htons(DNS_RRCLASS_IN);
    memcpy(q->buf.get() + offs, &tc, sizeof(DnsTypeClass));
    const int ret = sock->sendTo(SERVER_ADDRESS, SERVER_PORT, q->buf.get(), packetSize);
    if (ret < 0) {
        return ret;
    }
    return 0;
}

} // namespace particle::

int getHostByName(const char* name, MDM_IP* addr) {
    SPARK_ASSERT(name && addr);
    LOG_DEBUG(TRACE, "Resolving domain name: %s", name);
    if (!name[0]) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    const size_t nameLen = strlen(name);
    if (nameLen > MAX_NAME_LENGTH) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    if (strcmp(name, "localhost") == 0) {
        return IPADR(127, 0, 0, 1);
    }
    unsigned a = 0, b = 0, c = 0, d = 0;
    if (sscanf(name, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
        *addr = IPADR(a, b, c, d);
        return 0;
    }
    DnsQuery q(name);
    q.buf.reset(new(std::nothrow) char[BUFFER_SIZE]);
    if (!q.buf) {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    UdpSocket sock;
    int ret = sock.open();
    if (ret < 0) {
        LOG(ERROR, "Unable to create socket");
        return ret;
    }
    unsigned retries = 0;
    do {
        const system_tick_t timeout = (retries + 1) * 1000;
        LOG_DEBUG(TRACE, "Sending DNS query, timeout: %u", (unsigned)timeout);
        ret = sendDnsQuery(&sock, &q);
        if (ret < 0) {
            return ret;
        }
        const system_tick_t timeStart = HAL_Timer_Get_Milli_Seconds();
        do {
            HAL_Delay_Milliseconds(200);
            ret = recvDnsResponse(&sock, &q);
            if (ret < 0) {
                return ret;
            }
            if (q.addr != NOIP) {
                LOG_DEBUG(TRACE, "Received DNS response, address: " IPSTR, IPNUM(q.addr));
                *addr = q.addr;
                return 0; // OK
            }
        } while (HAL_Timer_Get_Milli_Seconds() - timeStart < timeout);
        ++retries;
    } while (retries <= MAX_RETRIES);
    return SYSTEM_ERROR_TIMEOUT;
}

} // namespace particle

#endif // !defined(HAL_CELLULAR_EXCLUDE)
