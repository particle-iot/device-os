/* 
 * File:   dns_hal.h
 * Author: mat
 *
 * Created on 17 September 2014, 21:02
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
 * @param out_ip_addr   The ip address 
 * @return 
 */
int inet_gethostbyname(char* hostname, uint16_t hostnameLen, uint32_t* out_ip_addr);


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

