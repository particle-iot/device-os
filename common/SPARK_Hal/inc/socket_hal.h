/* 
 * File:   socket.h
 * Author: mat
 *
 * Created on 09 September 2014, 01:01
 */

#ifndef SOCKET_H
#define	SOCKET_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "debug.h"
#include <stdint.h>
        
typedef struct _sockaddr_t
{
    uint16_t   sa_family;
    uint8_t     sa_data[14];
} sockaddr;

typedef uint32_t socket_handle_t;
typedef uint32_t socklen_t;


const uint8_t SOCKET_STATUS_INACTIVE = 1;
const uint8_t SOCKET_STATUS_ACTIVE = 0;

uint8_t get_socket_active_status(socket_handle_t socket);


int32_t socket_connect(socket_handle_t sd, const sockaddr *addr, long addrlen);

/**
 * Reads from a socket to a buffer, upto a given number of bytes, with timeout.
 * Returns the number of bytes read, which can be less than the number requested.
 * Returns <0 on error or if the stream is closed.
 */
int32_t socket_receive(socket_handle_t sd, void* buffer, int len, system_tick_t _timeout);

int32_t socket_receivefrom(socket_handle_t sd, void* buffer, int len, sockaddr* address, socklen_t* addr_size);

int32_t socket_send(socket_handle_t sd, const void* buffer, socklen_t len);

int32_t socket_sendto(socket_handle_t sd, const void* buffer, socklen_t len, sockaddr* addr, socklen_t addr_size);

int32_t socket_close(socket_handle_t sd);

int32_t socket_reset_blocking_call();

socket_handle_t socket_create(uint8_t family, uint8_t type, uint8_t protocol);

int32_t socket_create_nonblocking_server(socket_handle_t handle);
int32_t socket_accept(socket_handle_t sd);

/**
 * Binds a client socket.
 */
int32_t socket_bind(socket_handle_t sd, uint16_t port);



//int32_t bind(socket_handle_t sd, const sockaddr *addr, int32_t addrlen);
//int32_t listen(int32_t sd, int32_t backlog);
//int32_t setsockopt(socket_handle_t sd, int32_t level, int32_t optname, const void *optval, socklen_t optlen);


// CC3000
const socket_handle_t MAX_SOCK_NUM = (socket_handle_t)8;


/**
 * 
 * @param hostname      buffer to receive the hostname
 * @param hostnameLen   length of the hostname buffer 
 * @param out_ip_addr   The ip address 
 * @return 
 */
int gethostbyname(char* hostname, uint16_t hostnameLen, uint32_t* out_ip_addr);


//--------- Address Families --------

#define  AF_INET                2
#define  AF_INET6               23

//------------ Socket Types ------------

#define  SOCK_STREAM            1
#define  SOCK_DGRAM             2
#define  SOCK_RAW               3           // Raw sockets allow new IPv4 protocols to be implemented in user space. A raw socket receives or sends the raw datagram not including link level headers
#define  SOCK_RDM               4
#define  SOCK_SEQPACKET         5

//----------- Socket Protocol ----------

#define IPPROTO_IP              0           // dummy for IP
#define IPPROTO_ICMP            1           // control message protocol
#define IPPROTO_IPV4            IPPROTO_IP  // IP inside IP
#define IPPROTO_TCP             6           // tcp
#define IPPROTO_UDP             17          // user datagram protocol
#define IPPROTO_IPV6            41          // IPv6 in IPv6
#define IPPROTO_NONE            59          // No next header
#define IPPROTO_RAW             255         // raw IP packet
#define IPPROTO_MAX             256


#ifdef	__cplusplus
}
#endif

#endif	/* SOCKET_H */

