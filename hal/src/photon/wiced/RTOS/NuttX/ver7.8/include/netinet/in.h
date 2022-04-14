/****************************************************************************
 * include/netinet/in.h
 *
 *   Copyright (C) 2007, 2009-2010 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __INCLUDE_NETINET_IN_H
#define __INCLUDE_NETINET_IN_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Values for protocol argument to socket() */

#define IPPROTO_IP            0    /* Dummy protocol for TCP */
#define IPPROTO_HOPOPTS       0    /* IPv6 Hop-by-Hop options.  */
#define IPPROTO_ICMP          1    /* Internet Control Message Protocol */
#define IPPROTO_IGMP          2    /* Internet Group Management Protocol */
#define IPPROTO_IPIP          4    /* IPIP tunnels (older KA9Q tunnels use 94) */
#define IPPROTO_TCP           6    /* Transmission Control Protocol */
#define IPPROTO_EGP           8    /* Exterior Gateway Protocol */
#define IPPROTO_PUP           12   /* PUP protocol */
#define IPPROTO_UDP           17   /* User Datagram Protocol */
#define IPPROTO_IDP           22   /* XNS IDP protocol */
#define IPPROTO_TP            29   /* SO Transport Protocol Class 4.  */
#define IPPROTO_DCCP          33   /* Datagram Congestion Control Protocol */
#define IPPROTO_IPV6          41   /* IPv6-in-IPv4 tunnelling */
#define IPPROTO_ROUTING       43   /* IPv6 routing header. */
#define IPPROTO_FRAGMENT      44   /* IPv6 fragmentation header.  */
#define IPPROTO_RSVP          46   /* Reservation Protocol. */
#define IPPROTO_GRE           47   /* General Routing Encapsulation. */
#define IPPROTO_ESP           50   /* Encapsulation Security Payload protocol */
#define IPPROTO_AH            51   /* Authentication Header protocol */
#define IPPROTO_ICMPV6        58   /* ICMPv6 */
#define IPPROTO_NONE          59   /* IPv6 no next header. */
#define IPPROTO_DSTOPTS       60   /* IPv6 destination options. */
#define IPPROTO_MTP           92   /* Multicast Transport Protocol.  */
#define IPPROTO_ENCAP         98   /* Encapsulation Header. */
#define IPPROTO_BEETPH        94   /* IP option pseudo header for BEET */
#define IPPROTO_PIM           103  /* Protocol Independent Multicast */
#define IPPROTO_COMP          108  /* Compression Header protocol */
#define IPPROTO_SCTP          132  /* Stream Control Transport Protocol */
#define IPPROTO_UDPLITE       136  /* UDP-Lite (RFC 3828) */
#define IPPROTO_RAW           255  /* Raw IP packets */

/* Values used with SIOCSIFMCFILTER and SIOCGIFMCFILTER ioctl's */

#define MCAST_EXCLUDE         0
#define MCAST_INCLUDE         1

/* Test if an IPv4 address is a multicast address */

#define IN_CLASSD(i)          (((uint32_t)(i) & 0xf0000000) == 0xe0000000)
#define IN_MULTICAST(i)       IN_CLASSD(i)

/* Special values of in_addr_t */

#define INADDR_ANY            ((in_addr_t)0x00000000) /* Address to accept any incoming messages */
#define INADDR_BROADCAST      ((in_addr_t)0xffffffff) /* Address to send to all hosts */
#define INADDR_NONE           ((in_addr_t)0xffffffff) /* Address indicating an error return */
#define INADDR_LOOPBACK       ((in_addr_t)0x7f000001) /* Inet 127.0.0.1.  */

/* Special initializer for in6_addr_t */

#define IN6ADDR_ANY_INIT      {{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}}
#define IN6ADDR_LOOPBACK_INIT {{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}}}

/* struct in6_addr union selectors */

#define s6_addr               in6_u.u6_addr8
#define s6_addr16             in6_u.u6_addr16
#define s6_addr32             in6_u.u6_addr32

#ifdef CONFIG_NET_PKTINFO
/* Options for use with [gs]etsockopt at the IP level. */
//#define IP_TOS          1
#define IP_TTL          2
//#define IP_HDRINCL      3
//#define IP_OPTIONS      4
//#define IP_ROUTER_ALERT 5
//#define IP_RECVOPTS     6
//#define IP_RETOPTS      7
#define IP_PKTINFO      8
//#define IP_PKTOPTIONS   9
//#define IP_MTU_DISCOVER 10
//#define IP_RECVERR      11
//#define IP_RECVTTL      12
//#define IP_RECVTOS      13
//#define IP_MTU          14

#define IP_MULTICAST_IF     32
#define IP_MULTICAST_TTL    33
#define IP_MULTICAST_LOOP   34
#define IP_ADD_MEMBERSHIP   35
#define IP_DROP_MEMBERSHIP  36

#define IPV6_PKTINFO        50

/* control message flag for PKTINFO */
#define IP_CMSG_PKTINFO         1

/* flags for msg_flags in msghdr */
#endif /* CONFIG_NET_PKTINFO */

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

typedef uint16_t in_port_t;

/* IPv4 Internet address */

typedef uint32_t in_addr_t;

struct in_addr
{
  in_addr_t       s_addr;      /* Address (network byte order) */
};

struct sockaddr_in
{
  sa_family_t     sin_family;  /* Address family: AF_INET */
  uint16_t        sin_port;    /* Port in network byte order */
  struct in_addr  sin_addr;    /* Internet address */
};

/* IPv6 Internet address */

struct in6_addr
{
  union
  {
    uint8_t       u6_addr8[16];
    uint16_t      u6_addr16[8];
    uint32_t      u6_addr32[4];
  } in6_u;
};

struct sockaddr_in6
{
  sa_family_t     sin6_family; /* Address family: AF_INET */
  uint16_t        sin6_port;   /* Port in network byte order */
  uint32_t      sin6_flowinfo; /* IPv6 flow information */
  struct in6_addr sin6_addr;   /* IPv6 internet address */
  uint32_t      sin6_scope_id; /* IPv6 scope-id */
};

struct ip_mreq
{
  struct in_addr imr_multiaddr;   /* IP multicast address of group */
  struct in_addr imr_interface;   /* local IP address of interface */
};

struct in_pktinfo
{
  int             ipi_ifindex;
  struct in_addr  ipi_spec_dst;
  struct in_addr  ipi_addr;
};

struct in6_pktinfo
{
  struct in6_addr ipi6_addr;
  int             ipi6_ifindex;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/* Global IPv6 in6addr_any */

EXTERN const struct in6_addr in6addr_any;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __INCLUDE_NETINET_IN_H */
