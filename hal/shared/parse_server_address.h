/**
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef PARSE_SERVER_ADDRESS_H
#define	PARSE_SERVER_ADDRESS_H

#include "ota_flash_hal.h" // For ServerAddress
#include "system_error.h"

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cctype>

namespace particle {

inline void parseServerAddressData(ServerAddress* addr, const uint8_t* buf, size_t size) {
    // Internet address stored on external flash may be
    // either a domain name or an IP address.
    // It's stored in a type-length-value encoding.
    // First byte is type, second byte is length, the rest is value.
    if (size < 2) {
        addr->addr_type = INVALID_INTERNET_ADDRESS;
        return;
    }
    switch (buf[0]) {
    case IP_ADDRESS: {
        size_t addr_len = buf[1];
        if (addr_len != 4 || size < 6) {
            addr->addr_type = INVALID_INTERNET_ADDRESS;
            return;
        }
        auto ip = ((unsigned)buf[2] << 24) | ((unsigned)buf[3] << 16) | ((unsigned)buf[4] << 8) | (unsigned)buf[5];
        if (!ip || ip == 0xffffffffu) { // 0.0.0.0 or 255.255.255.255
            addr->addr_type = INVALID_INTERNET_ADDRESS;
            return;
        }
        addr->addr_type = IP_ADDRESS;
        addr->length = addr_len;
        addr->ip = ip;
        break;
    }
    case DOMAIN_NAME: {
        size_t name_len = buf[1];
        if (name_len + 2 > size || name_len > sizeof(addr->domain) - 1) { // Reserve 1 byte for '\0'
            addr->addr_type = INVALID_INTERNET_ADDRESS;
            return;
        }
        auto name = buf + 2;
        for (size_t i = 0; i < name_len; ++i) {
            if (!isprint(name[i])) {
                addr->addr_type = INVALID_INTERNET_ADDRESS;
                return;
            }
        }
        addr->addr_type = DOMAIN_NAME;
        addr->length = name_len;
        memcpy(addr->domain, name, name_len);
        addr->domain[name_len] = '\0';
        break;
    }
    default:
        addr->addr_type = INVALID_INTERNET_ADDRESS;
        return;
    }
    if (size < 68) {
        addr->addr_type = INVALID_INTERNET_ADDRESS;
        return;
    }
    addr->port = (buf[66] << 8) | buf[67];
}

inline int encodeServerAddressData(const ServerAddress* addr, uint8_t* buf, size_t bufSize) {
    // Size of the ServerAddress structure without the padding bytes
    const size_t size = sizeof(ServerAddress) - sizeof(ServerAddress::padding);
    if (bufSize < size) {
        return SYSTEM_ERROR_TOO_LARGE;
    }
    if (addr->addr_type == IP_ADDRESS || addr->addr_type == DOMAIN_NAME) {
        if (addr->length > size - 4) { // 4 bytes for the type, length and port fields
            return SYSTEM_ERROR_TOO_LARGE;
        }
        // ServerAddress is serialized as a packed structure, but the parsing code expects its integer
        // fields to be encoded in the network order
        memcpy(buf, addr, size);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        if (addr->addr_type == IP_ADDRESS) {
            uint32_t* const ip = (uint32_t*)(buf + offsetof(ServerAddress, ip));
            *ip = __builtin_bswap32(*ip);
        }
        uint16_t* const port = (uint16_t*)(buf + offsetof(ServerAddress, port));
        *port = __builtin_bswap16(*port);
#endif
    } else {
        *buf = INVALID_INTERNET_ADDRESS;
    }
    return 0;
}

// Note: This function doesn't parse the port number
inline int parseServerAddressString(ServerAddress* addr, const char* str) {
    unsigned n1 = 0, n2 = 0, n3 = 0, n4 = 0;
    if (sscanf(str, "%u.%u.%u.%u", &n1, &n2, &n3, &n4) == 4) {
        if (n1 > 255 || n2 > 255 || n3 > 255 || n4 > 255) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        addr->addr_type = IP_ADDRESS;
        addr->length = 4;
        addr->ip = (n1 << 24) | (n2 << 16) | (n3 << 8) | n4;
    } else {
        size_t len = strlen(str);
        if (len > sizeof(addr->domain) - 1) { // Reserve 1 byte for '\0'
            return SYSTEM_ERROR_TOO_LARGE;
        }
        addr->addr_type = DOMAIN_NAME;
        addr->length = len;
        memcpy(addr->domain, str, len + 1); // Include '\0'
    }
    return 0;
}

} // namespace particle

#endif	/* PARSE_SERVER_ADDRESS_H */

