/**
 ******************************************************************************
 * @file    inet_hal.h
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    25-Sept-2014
 * @brief   Internet APIs
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

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

#ifndef DNS_HAL_H
#define	DNS_HAL_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>

#if PLATFORM_ID>=4 && PLATFORM_ID<=8
#define HAL_IPv6 1
#else    
#define HAL_IPv6 0
#endif

#if HAL_IPv6
typedef struct _HAL_IPAddress_t {
    uint8_t v;              // 4 for Ipv4, 6 for Ipv6
    union {
        uint8_t ipv4[4];    // in network order (big-endian)        
        uint32_t u32;       // convenient access (is reversed)
        uint8_t ipv6[16];  
    };
} HAL_IPAddress;
#else
typedef struct _HAL_IPAddress_t {
    union {
        uint8_t ipv4[4];    
        uint32_t u32;
    };
} HAL_IPAddress;
#endif

typedef struct _NetworkConfig_t {
    HAL_IPAddress aucIP;             // byte 0 is MSB, byte 3 is LSB
    HAL_IPAddress aucSubnetMask;     // byte 0 is MSB, byte 3 is LSB
    HAL_IPAddress aucDefaultGateway; // byte 0 is MSB, byte 3 is LSB
    HAL_IPAddress aucDHCPServer;     // byte 0 is MSB, byte 3 is LSB
    HAL_IPAddress aucDNSServer;      // byte 0 is MSB, byte 3 is LSB
    uint8_t uaMacAddr[6];
} NetworkConfig;

typedef uint32_t network_interface_t;

/**
 * 
 * @param hostname      buffer to receive the hostname
 * @param hostnameLen   length of the hostname buffer 
 * @param out_ip_addr   The ip address in network byte order.
 * @return 
 */
int inet_gethostbyname(const char* hostname, uint16_t hostnameLen, HAL_IPAddress* out_ip_addr, 
        network_interface_t nif, void* reserved);


/**
 * 
 * @param remoteIP  The IP address. MSB..LSB [0..3]
 * @param nTries
 * @return >0 on success. 0 on timeout? <0 on error.
 */
int inet_ping(const HAL_IPAddress* address, network_interface_t nif, uint8_t nTries,
        void* reserved);


#ifdef	__cplusplus
}
#endif

#endif	/* DNS_HAL_H */

