/*
 * File:   parse_server_address.h
 * Author: mat1
 *
 * Created on April 20, 2015, 8:42 PM
 */

#ifndef PARSE_SERVER_ADDRESS_H
#define	PARSE_SERVER_ADDRESS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "ota_flash_hal.h"
#include <stdint.h>

void parseServerAddressData(ServerAddress* server_addr, const uint8_t* buf, int maxLength)
{
  // Internet address stored on external flash may be
  // either a domain name or an IP address.
  // It's stored in a type-length-value encoding.
  // First byte is type, second byte is length, the rest is value.

  switch (buf[0])
  {
    case IP_ADDRESS:
      server_addr->addr_type = IP_ADDRESS;
      server_addr->ip = (buf[2] << 24) | (buf[3] << 16) |
                        (buf[4] << 8)  |  buf[5];
      break;

    case DOMAIN_NAME:
      if (buf[1] <= maxLength - 2)
      {
        server_addr->addr_type = DOMAIN_NAME;
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

}


#ifdef	__cplusplus
}
#endif

#endif	/* PARSE_SERVER_ADDRESS_H */

