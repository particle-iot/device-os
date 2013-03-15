/*****************************************************************************
*
*  socket.h  - CC3000 Host Driver Implementation.
*  Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the   
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*****************************************************************************/
#ifndef __SOCKET_H__
#define __SOCKET_H__


//*****************************************************************************
//
//! \addtogroup socket_api
//! @{
//
//*****************************************************************************


//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef  __cplusplus
extern "C" {
#endif

#define HOSTNAME_MAX_LENGTH (230)  // 230 bytes + header shouldn't exceed 8 bit value

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

//----------- Socket retunr codes  -----------

#define SOC_ERROR				(-1)		// error 
#define SOC_IN_PROGRESS			(-2)		// socket in progress

//----------- Socket Options -----------
#define  SOL_SOCKET             0xffff		//  socket level
#define  SOCKOPT_RECV_TIMEOUT	1			//  optname to configure recv and recvfromtimeout
#define  SOCKOPT_NONBLOCK          2			// accept non block mode set SOCK_ON or SOCK_OFF (default block mode )
#define  SOCK_ON                0			// socket non-blocking mode	is enabled		
#define  SOCK_OFF               1			// socket blocking mode is enabled

#define  TCP_NODELAY            0x0001
#define  TCP_BSDURGENT          0x7000

#define  MAX_PACKET_SIZE        1500
#define  MAX_LISTEN_QUEUE       4

#define  IOCTL_SOCKET_EVENTMASK

#define ENOBUFS                 55          // No buffer space available

#define __FD_SETSIZE            32

#define  ASIC_ADDR_LEN          8
	
#define NO_QUERY_RECIVED        -3
	
	
typedef struct _in_addr_t
{
    unsigned long s_addr;                   // load with inet_aton()
} in_addr;

typedef struct _sockaddr_t
{
    unsigned short int    sa_family;
    unsigned char     sa_data[14];
} sockaddr;

typedef struct _sockaddr_in_t
{
    short            sin_family;            // e.g. AF_INET
    unsigned short   sin_port;              // e.g. htons(3490)
    in_addr          sin_addr;              // see struct in_addr, below
    char             sin_zero[8];           // zero this if you want to
} sockaddr_in;

typedef unsigned long socklen_t;

// The fd_set member is required to be an array of longs.
typedef long int __fd_mask;

// It's easier to assume 8-bit bytes than to get CHAR_BIT.
#define __NFDBITS               (8 * sizeof (__fd_mask))
#define __FDELT(d)              ((d) / __NFDBITS)
#define __FDMASK(d)             ((__fd_mask) 1 << ((d) % __NFDBITS))

/* Zac changed fd_set definition on 2013-02-18 to avoid conflicts with sys/types.h */
// fd_set for select and pselect.
typedef struct
{
    __fd_mask fds_bits[__FD_SETSIZE / __NFDBITS];
#define __FDS_BITS(set)        ((set)->fds_bits)
} _types_fd_set_cc3000;
#define fd_set _types_fd_set_cc3000

// We don't use `memset' because this would require a prototype and
//   the array isn't too big.
#define __FD_ZERO(set)                               \
  do {                                                \
    unsigned int __i;                                 \
    fd_set *__arr = (set);                            \
    for (__i = 0; __i < sizeof (fd_set) / sizeof (__fd_mask); ++__i) \
      __FDS_BITS (__arr)[__i] = 0;                    \
  } while (0)
#define __FD_SET(d, set)       (__FDS_BITS (set)[__FDELT (d)] |= __FDMASK (d))
#define __FD_CLR(d, set)       (__FDS_BITS (set)[__FDELT (d)] &= ~__FDMASK (d))
#define __FD_ISSET(d, set)     (__FDS_BITS (set)[__FDELT (d)] & __FDMASK (d))

// Access macros for 'fd_set'.
#define FD_SET(fd, fdsetp)      __FD_SET (fd, fdsetp)
#define FD_CLR(fd, fdsetp)      __FD_CLR (fd, fdsetp)
#define FD_ISSET(fd, fdsetp)    __FD_ISSET (fd, fdsetp)
#define FD_ZERO(fdsetp)         __FD_ZERO (fdsetp)

//Use in case of Big Endian only
  
#define htonl(A)    ((((unsigned long)(A) & 0xff000000) >> 24) | \
                     (((unsigned long)(A) & 0x00ff0000) >> 8) | \
                     (((unsigned long)(A) & 0x0000ff00) << 8) | \
                     (((unsigned long)(A) & 0x000000ff) << 24))

#define ntohl                   htonl

//Use in case of Big Endian only
#define htons(A)     ((((unsigned long)(A) & 0xff00) >> 8) | \
                      (((unsigned long)(A) & 0x00ff) << 8))


#define ntohs                   htons

// mDNS port - 5353    mDNS multicast address - 224.0.0.251 
#define SET_mDNS_ADD(sockaddr)     	   	sockaddr.sa_data[0] = 0x14; \
																								sockaddr.sa_data[1] = 0xe9; \
																								sockaddr.sa_data[2] = 0xe0; \
																								sockaddr.sa_data[3] = 0x0; \
																								sockaddr.sa_data[4] = 0x0; \
																								sockaddr.sa_data[5] = 0xfb; 


//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************

/**
 * \brief create an endpoint for communication
 *
 * The socket function creates a socket that is bound to a specific transport service provider.
 * This function is called by the application layer to obtain a socket handle.
 *
 * \param[in] domain            selects the protocol family which will be used for communication. On this version only AF_INET is supported
 * \param[in] type  			specifies the communication semantics. On this version only SOCK_STREAM, SOCK_DGRAM,  SOCK_RAW are supported
 * \param[in] protocol          specifies a particular protocol to be used with the socket IPPROTO_TCP, IPPROTO_UDP or IPPROTO_RAW are supported
 *
 * \return						On success, socket handle that 
 *              is used for consequent socket operations. On
 *         error, -1 is returned.
 *
 * \sa
 * \note
 * \warning
 */


extern int socket(long domain, long type, long protocol);

/**
 * \brief gracefully close socket
 *
 * The socket function closes a created socket.
 *
 * \param[in] sd                socket handle.
 *
 * \return	On success, zero is returned. On error, -1 is 
 *         returned.
 *
 * \sa socket
 * \note
 * \warning
 */
extern long closesocket(long sd);

/**
 * \brief accept a connection on a socket
 *
 * This function is used with connection-based socket types (SOCK_STREAM).
 * It extracts the first connection request on the queue of pending connections, creates a
 * new connected socket, and returns a new file descriptor referring to that socket.
 * The newly created socket is not in the listening state. The 
 * original socket sd is unaffected by this call. 
 * The argument sd is a socket that has been created with 
 * socket(), bound to a local address with bind(), and is 
 * listening for connections after a listen(). The argument \b 
 * \e addr is a pointer to a sockaddr structure. This structure 
 * is filled in with the address of the peer socket, as known to 
 * the communications layer. The exact format of the address 
 * returned addr is determined by the socket's address family. 
 * The \b \e addrlen argument is a value-result argument: it 
 * should initially contain the size of the structure pointed to 
 * by addr, on return it will contain the actual length (in 
 * bytes) of the address returned.
 *
 * \param[in] sd                socket descriptor (handle)
 * \param[out] addr             the argument addr is a pointer 
 *                              to a sockaddr structure. This
 *                              structure is filled in with the
 *                              address of the peer socket, as
 *                              known to the communications
 *                              layer. The exact format of the
 *                              address returned addr is
 *                              determined by the socket's
 *                              address\n
 *                              sockaddr:\n - code for the
 *                              address format. On this version
 *                              only AF_INET is supported.\n -
 *                              socket address, the length
 *                              depends on the code format
 * \param[out] addrlen          the addrlen argument is a 
 *       value-result argument: it should initially contain the
 *       size of the structure pointed to by addr
 *
 * \return	For socket in blocking mode:
 *				On success, socket handle. on failure negative \n
 *			For socket in non-blocking mode:
 *				- On connection esatblishment, socket handle
 *				- On connection pending, SOC_IN_PROGRESS (-2)
 *				- On failure, SOC_ERROR	(-1)
 *				
 *
 *
 * \sa socket ; bind ; listen
 * \note 
 * \warning
 */
extern long accept(long sd, sockaddr *addr, socklen_t *addrlen);

/**
 * \brief assign a name to a socket
 *
 * This function gives the socket the local address addr.
 * addr is addrlen bytes long. Traditionally, this is called
 * When a socket is created with socket, it exists in a name
 * space (address family) but has no name assigned.
 * It is necessary to assign a local address before a SOCK_STREAM
 * socket may receive connections.
 *
 * \param[in] sd                socket descriptor (handle)
 * \param[in] addr              specifies the destination 
 *                              addrs\n sockaddr:\n - code for
 *                              the address format. On this
 *                              version only AF_INET is
 *                              supported.\n - socket address,
 *                              the length depends on the code
 *                              format
 * \param[in] addrlen           contains the size of the 
 *       structure pointed to by addr
 *
 * \return						On success, zero is returned. On error, -1 is returned.
 *
 * \sa socket   accept   listen
 * \note
 * \warning
 */
extern long bind(long sd, const sockaddr *addr, long addrlen);

/**
 * \brief listen for connections on a socket
 *
 * The willingness to accept incoming connections and a queue
 * limit for incoming connections are specified with listen(),
 * and then the connections are accepted with accept.
 * The listen() call applies only to sockets of type SOCK_STREAM
 * The backlog parameter defines the maximum length the queue of
 * pending connections may grow to. 
 *
 * \param[in] sd                socket descriptor (handle)
 * \param[in] backlog        pecifies the listen queue 
 *       depth. On this version backlog is not supported
 *
 * \return	On success, zero is returned. On 
 *             error, -1 is returned.
 *
 * \sa socket  accept  bind
 * \note On this version, backlog is not supported
 * \warning
 */
extern long listen(long sd, long backlog);


/**
 * \brief initiate a connection on a socket 
 *  
 * Function connects the socket referred to by the socket 
 * descriptor sd, to the address specified by addr. The addrlen 
 * argument specifies the size of addr. The format of the 
 * address in addr is determined by the address space of the 
 * socket. If it is of type SOCK_DGRAM, this call specifies the 
 * peer with which the socket is to be associated; this address 
 * is that to which datagrams are to be sent, and the only 
 * address from which datagrams are to be received.  If the 
 * socket is of type SOCK_STREAM, this call attempts to make a 
 * connection to another socket. The other socket is specified 
 * by address, which is an address in the com- munications space 
 * of the socket. Note that the function implements only blocking
 * bheavior thus the caller will be waiting either for the connection 
 * establishement or for the connection establishement failure.
 *  
 *  
 * \param[in] sd                socket descriptor (handle)
 * \param[in] addr              specifies the destination addr\n
 *                              sockaddr:\n - code for the
 *                              address format. On this version
 *                              only AF_INET is supported.\n -
 *                              socket address, the length
 *                              depends on the code format
 *  
 * \param[in] addrlen           contains the size of the 
 *       structure pointed to by addr
 *
 * \return   On success, zero is returned. On error, -1 is 
 *              returned
 *
 * \sa socket
 * \note
 * \warning
 */
extern long connect(long sd, const sockaddr *addr, long addrlen);

/**
 * \brief Monitor socket activity
 *  
 * Select allow a program to monitor multiple file descriptors,
 * waiting until one or more of the file descriptors become 
 * "ready" for some class of I/O operation 
 *  
 *  
 * \param[in] nfds           the highest-numbered file descriptor in any of the
 *                           three sets, plus 1.
 * \param[out] writesds      socket descriptors list for write 
 *       monitoring
 * \param[out] readsds       socket descriptors list for read 
 *       monitoring
 * \param[out] exceptsds     socket descriptors list for 
 *       exception monitoring
 * \param[in] timeout        is an upper bound on the amount of time elapsed
 *                           before select() returns. Null means infinity 
 *                           timeout. The minimum timeout is 5 milliseconds,
 *                           less than 5 milliseconds will be set
 *                           automatically to 5 milliseconds.
 *  
 * \return						On success, select()  returns the number of
 *                      file descriptors contained in the three returned
 *                      descriptor sets (that is, the total number of bits that
 *                      are set in readfds, writefds, exceptfds) which may be
 *                      zero if the timeout expires before anything interesting
 *                      happens. On error, -1 is returned.
 *                      readsds - return the sockets on which Read request will
 *                                return without delay with valid data.
 *                      writesds - return the sockets on which Write request 
 *                                 will return without delay.
 *                      exceptsds - return the sockets wich closed recently. 
 *
 * \sa socket
 * \note  If the timeout value set to less than 5ms it will 
 *  automatically set to 5ms to prevent overload of the system
 *  
 * \warning
 */
extern int select(long nfds, fd_set *readsds, fd_set *writesds,
                  fd_set *exceptsds, struct timeval *timeout);

/**
 * \brief set socket options
 *
 * This function manipulate the options associated with a socket.
 * Options may exist at multiple protocol levels; they are always
 * present at the uppermost socket level.
 *
 * When manipulating socket options the level at which the option resides
 * and the name of the option must be specified.  To manipulate options at
 * the socket level, level is specified as SOL_SOCKET.  To manipulate
 * options at any other level the protocol number of the appropriate proto-
 * col controlling the option is supplied.  For example, to indicate that an
 * option is to be interpreted by the TCP protocol, level should be set to
 * the protocol number of TCP; 
 *
 * The parameters optval and optlen are used to access optval - 
 * ues for setsockopt().  For getsockopt() they identify a 
 * buffer in which the value for the requested option(s) are to 
 * be returned.  For getsockopt(), optlen is a value-result 
 * parameter, initially contain- ing the size of the buffer 
 * pointed to by option_value, and modified on return to 
 * indicate the actual size of the value returned. This value returns in network order. 
 * If no option value is to be supplied or returned, option_value may be  NULL. 
 *
 *  
 * \param[in] sd                socket handle
 * \param[in] level             defines the protocol level for this option
 * \param[in] optname           defines the option name to interogate
 * \param[in] optval            specifies a value for the option
 * \param[in] optlen            specifies the length of the 
 *       option value
 *
 * \return   On success, zero is returned. On error, -1 is 
 *            returned 
 * \sa getsockopt
 * \note On this version the following socket options are enabled:
 *			- SOL_SOCKET (optname). SOL_SOCKET configures socket level
 *			- SOCKOPT_RECV_TIMEOUT (optname)
 *			  SOCKOPT_RECV_TIMEOUT configures recv and recvfrom timeout. 
 *			  In that case optval should be pointer to unsigned long
 *			- SOCK_NONBLOCK (optname). set socket non-blocking mode is on or off.
 *			  SOCK_ON or SOCK_OFF (optval)
 * \warning
 */
#ifndef CC3000_TINY_DRIVER 
extern int setsockopt(long sd, long level, long optname, const void *optval,
                      socklen_t optlen);
#endif
/**
 * \brief get socket options
 *
 * This function manipulate the options associated with a socket.
 * Options may exist at multiple protocol levels; they are always
 * present at the uppermost socket level.
 *
 * When manipulating socket options, the level at which the option resides
 * and the name of the option must be specified.  To manipulate options at
 * the socket level, level is specified as SOL_SOCKET.  To manipulate
 * options at any other level the protocol number of the appropriate proto-
 * col controlling the option is supplied.  For example, to indicate that an
 * option is to be interpreted by the TCP protocol, level should be set to
 * the protocol number of TCP; 
 *
 * The parameters optval and optlen are used to access optval - 
 * ues for setsockopt().  For getsockopt() they identify a 
 * buffer in which the value for the requested option(s) are to 
 * be returned.  For getsockopt(), optlen is a value-result 
 * parameter, initially contain- ing the size of the buffer 
 * pointed to by option_value, and modified on return to 
 * indicate the actual size of the value returned.  If no option 
 * value is to be supplied or returned, option_value may be 
 * NULL. 
 *
 *
 * \param[in] sd                socket handle
 * \param[in] level             defines the protocol level for this option
 * \param[in] optname           defines the option name to interogate
 * \param[out] optval           specifies a value for the option
 * \param[out] optlen           specifies the length of the 
 *       option value
 *
 * \return		On success, zero is returned. On error, -1 is 
 *            returned 
 * \sa setsockopt
 *
 * \note On this version only SOL_SOCKET 
 *         (level) and SOCKOPT_RECV_TIMEOUT (optname) is
 *         enabled. SOCKOPT_RECV_TIMEOUT configure recv and
 *         recvfrom timeout. In that case optval should be
 *         pointer to unsigned long
 *        
 * \warning
 */
extern int getsockopt(long sd, long level, long optname, void *optval,
                      socklen_t *optlen);

/**
 * \brief read data from TCP socket
 *  
 * function receives a message from a connection-mode socket
 *  
 * \param[in] sd                socket handle
 * \param[out] buf              Points to the buffer where the 
 *       message should be stored.
 * \param[in] len               Specifies the length in bytes of 
*       the buffer pointed to by the buffer argument. Range: 1-1460 bytes
 * \param[in] flags             Specifies the type of message 
 *       reception. On this version, this parameter is not
 *       supported.
 *
 * \return   return the number of bytes received, or -1 if an 
 *           error occurred
 *
 * \sa   recvfrom
 * \note   On this version, only blocking mode is supported.
 * \warning
 */
extern int recv(long sd, void *buf, long len, long flags);

/**
 * \brief read data from socket
 *
 * function receives a message from a connection-mode or
 * connectionless-mode socket. Note that raw sockets are not
 * supported.
 * 
 * \param[in] sd                socket handle 
 * \param[out] buf              Points to the buffer where the message should be stored.
 * \param[in] len               Specifies the length in bytes of the buffer pointed to by the buffer argument. 
 * \param[in] flags             Specifies the type of message
 *       reception. On this version, this parameter is not
 *       supported.
 * \param[in] from              pointer to an address structure 
 *                              indicating the source
 *                              address.\n sockaddr:\n - code
 *                              for the address format. On this
 *                              version only AF_INET is
 *                              supported.\n - socket address,
 *                              the length depends on the code
 *                              format
 * \param[in] fromlen           source address strcutre
 *       size
 * 
 *
 * \return   return the number of bytes received, 0 if timeout 
 *           occurred or 1 if an error occurred
 *
 * \sa   recv
 * \note   On this version, only blocking mode is supported.
 * \warning
 */
extern int recvfrom(long sd, void *buf, long len, long flags, sockaddr *from, 
                    socklen_t *fromlen);

/**
 * \brief write data to TCP socket
 * 
 * This function is used to transmit a message to another socket.
 *  
 * \param[in] sd                socket handle
 * \param[in] buf               Points to a buffer containing 
 *       the message to be sent
 * \param[in] len               message size in bytes. Range: 1-1460 bytes
 * \param[in] flags             Specifies the type of message 
 *       transmission. On this version, this parameter is not
 *       supported
 * 
 *
 * \return   Return the number of bytes transmited, or -1 if an 
 *           error occurred
 *
 * \sa   sendto 
 * \note   On this version, only blocking mode is supported.
 * \warning   
 */

extern int send(long sd, const void *buf, long len, long flags);

/**
 * \brief write data to socket
 *
 * This function is used to transmit a message to another socket
 * (connection less socket SOCK_DGRAM,  SOCK_RAW). 
 *
 * \param[in] sd                socket handle
 * \param[in] buf               Points to a buffer containing 
 *       the message to be sent
 * \param[in] len               message size in bytes. Range: 1-1460 bytes
 * \param[in] flags             Specifies the type of message 
 *       transmission. On this version, this parameter is not
 *       supported 
 * \param[in] to                pointer to an address structure 
 *                              indicating the destination
 *                              address.\n sockaddr:\n - code
 *                              for the address format. On this
 *                              version only AF_INET is
 *                              supported.\n - socket address,
 *                              the length depends on the code
 *                              format
 * \param[in] tolen             destination address strcutre size 
 *
 * \return   Return the number of transmitted bytes, or
 *           -1 if an error occurred
 *
 * \sa   send write
 * \note   On this version only, blocking mode is supported.
 * \warning
 */

extern int sendto(long sd, const void *buf, long len, long flags, 
                  const sockaddr *to, socklen_t tolen);


/**
 * \brief get host IP by name
 *
 * Obtain the IP Address of machine on network, by its name.
 *  
 * \param[in]  hostname       host name            
 * \param[in]  usNameLen      name length      
 * \param[out] out_ip_addr    This parameter is filled in with 
 *       host IP address. In case that host name is not
 *       resolved, out_ip_addr is zero.
 *
 * \return   On success, positive is returned. On error, 
 *           negative is returned
 *  
 * \sa
 * \note   On this version only, blocking mode is supported. Also note that
 *		the function requires DNS server to be configured prior to its usage.
 * \warning
 */
#ifndef CC3000_TINY_DRIVER 
extern int gethostbyname(char * hostname, unsigned short usNameLen, unsigned long* out_ip_addr);
#endif

/**
 * \Set CC3000 in mDNS advertiser mode in order to advertise itself
 * 
 * This function is used to make the CC3000 seen by mDNS browsers
 * \param[in] mdnsEnabled          		flag to enable/disable the mDNS feature
 * \param[in] deviceServiceName    		the service name as part of the published canonical domain name
 * \param[in] deviceServiceNameLength     the length of the service name
 * \return   On success, zero is returned,    return SOC_ERROR if socket was not opened successfully, or if an error occurred.
 *
 * \sa   
 * \note 
 * \warning   
 */
extern int mdnsAdvertiser(unsigned short mdnsEnabled, char * deviceServiceName, unsigned short deviceServiceNameLength);
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************


//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef  __cplusplus
}
#endif // __cplusplus

#endif // __SOCKET_H__
