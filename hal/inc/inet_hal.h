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

/**
 * 
 * @param hostname      buffer to receive the hostname
 * @param hostnameLen   length of the hostname buffer 
 * @param out_ip_addr   The ip address in network byte order.
 * @return 
 */
int inet_gethostbyname(const char* hostname, uint16_t hostnameLen, uint32_t* out_ip_addr);


/**
 * 
 * @param remoteIP  The IP address. MSB..LSB [0..3]
 * @param nTries
 * @return >0 on success. 0 on timeout? <0 on error.
 */
int inet_ping(uint8_t remoteIP[4], uint8_t nTries);


#ifdef	__cplusplus
}
#endif

#endif	/* DNS_HAL_H */

