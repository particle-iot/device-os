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

#include "ota_flash_hal.h"

#include "system_error.h"

#include <stdint.h>
#include <stddef.h>

#ifdef	__cplusplus
extern "C" {
#endif

inline void parseServerAddressData(ServerAddress* server_addr, const uint8_t* buf, int maxLength)
{
  // Internet address stored on external flash may be
  // either a domain name or an IP address.
  // It's stored in a type-length-value encoding.
  // First byte is type, second byte is length, the rest is value.

  switch (buf[0])
  {
    case IP_ADDRESS:
      server_addr->addr_type = IP_ADDRESS;
      server_addr->length = 4;
      server_addr->ip = (buf[2] << 24) | (buf[3] << 16) |
                        (buf[4] << 8)  |  buf[5];
      break;

    case DOMAIN_NAME:
      if (buf[1] <= maxLength - 2)
      {
        server_addr->addr_type = DOMAIN_NAME;
        server_addr->length = buf[1];
        memcpy(server_addr->domain, buf + 2, buf[1]);

        // null terminate string
        char *p = server_addr->domain + buf[1];
        *p = 0;
        break;
      }
      // else fall through to default

    default:
      server_addr->addr_type = INVALID_INTERNET_ADDRESS;
  }
  if (server_addr->addr_type!=INVALID_INTERNET_ADDRESS)
  {
	  server_addr->port =  buf[66]<<8 | buf[67];

  }

}

inline int encodeServerAddressData(const ServerAddress* addr, uint8_t* buf, size_t bufSize) {
    // Size of the ServerAddress structure without the padding bytes
    const size_t size = sizeof(ServerAddress) - sizeof(((ServerAddress){0}).padding);
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

#ifdef	__cplusplus
}
#endif

#endif	/* PARSE_SERVER_ADDRESS_H */

