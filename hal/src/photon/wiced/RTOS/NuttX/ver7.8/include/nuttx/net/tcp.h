/****************************************************************************
 * include/nuttx/net/tcp.h
 * Header file for the uIP TCP/IP stack.
 *
 * The uIP TCP/IP stack header file contains definitions for a number
 * of C macros that are used by uIP programs as well as internal uIP
 * structures, TCP/IP header structures and function declarations.
 *
 *   Copyright (C) 2007, 2009-2010, 2012-2014 Gregory Nutt. All rights
 *      reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * This logic was leveraged from uIP which also has a BSD-style license:
 *
 *   Author Adam Dunkels <adam@dunkels.com>
 *   Copyright (c) 2001-2003, Adam Dunkels.
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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
 *
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_NET_TCP_H
#define __INCLUDE_NUTTX_NET_TCP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#ifdef CONFIG_NET_TCP

#include <stdint.h>

#include <nuttx/net/netconfig.h>
#include <nuttx/net/arp.h>
#include <nuttx/net/ip.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* TCP definitions */

#define TCP_FIN           0x01
#define TCP_SYN           0x02
#define TCP_RST           0x04
#define TCP_PSH           0x08
#define TCP_ACK           0x10
#define TCP_URG           0x20
#define TCP_CTL           0x3f

#define TCP_OPT_END       0   /* End of TCP options list */
#define TCP_OPT_NOOP      1   /* "No-operation" TCP option */
#define TCP_OPT_MSS       2   /* Maximum segment size TCP option */

#define TCP_OPT_MSS_LEN   4   /* Length of TCP MSS option. */

/* The TCP states used in the struct tcp_conn_s tcpstateflags field */

#define TCP_STATE_MASK    0x0f /* Bits 0-3: TCP state */
#  define TCP_CLOSED      0x00 /* The connection is not in use and available */
#  define TCP_ALLOCATED   0x01 /* The connection is allocated, but not yet initialized */
#  define TCP_SYN_RCVD    0x02
#  define TCP_SYN_SENT    0x03
#  define TCP_ESTABLISHED 0x04
#  define TCP_FIN_WAIT_1  0x05
#  define TCP_FIN_WAIT_2  0x06
#  define TCP_CLOSING     0x07
#  define TCP_TIME_WAIT   0x08
#  define TCP_LAST_ACK    0x09
#  define TCP_STOPPED     0x10 /* Bit 4: stopped */
                               /* Bit 5-7: Unused, but not available */

/* TCP header sizes */

#define TCP_HDRLEN        20                         /* Size of TCP header */

#ifdef CONFIG_NET_IPv4
#  define IPv4TCP_HDRLEN (TCP_HDRLEN + IPv4_HDRLEN) /* Size of IPv4 + TCP header */
#endif

#ifdef CONFIG_NET_IPv6
#  define IPv6TCP_HDRLEN (TCP_HDRLEN + IPv6_HDRLEN) /* Size of IPv4 + TCP header */
#endif

/* Initial minimum MSS according to RFC 879
 *
 * There have been some assumptions made about using other than the
 * default size for datagrams with some unfortunate results.
 *
 *     HOSTS MUST NOT SEND DATAGRAMS LARGER THAN 576 OCTETS UNLESS THEY
 *     HAVE SPECIFIC KNOWLEDGE THAT THE DESTINATION HOST IS PREPARED TO
 *     ACCEPT LARGER DATAGRAMS.
 *
 * This is a long established rule.
 */

#define TCP_INITIAL_MSS(d,h)       (TCP_MSS(d,h) > 576 ? 576 : TCP_MSS(d,h))

#define MIN_TCP_INITIAL_MSS(h)     (__MIN_TCP_MSS(h) > 576 ? 576 : __MIN_TCP_MSS(h))
#define MAX_TCP_INITIAL_MSS(h)     (__MAX_TCP_MSS(h) > 576 ? 576 : __MAX_TCP_MSS(h))

#ifdef CONFIG_NET_IPv4
#  define TCP_IPv4_INITIAL_MSS(d)  TCP_INITIAL_MSS(d,IPv4_HDRLEN)
#  define MIN_IPv4_TCP_INITIAL_MSS MIN_TCP_INITIAL_MSS(IPv4_HDRLEN)
#  define MAX_IPv4_TCP_INITIAL_MSS MAX_TCP_INITIAL_MSS(IPv4_HDRLEN)
#endif

#ifdef CONFIG_NET_IPv6
#  define TCP_IPv6_INITIAL_MSS(d)  TCP_INITIAL_MSS(d,IPv6_HDRLEN)
#  define MIN_IPv6_TCP_INITIAL_MSS MIN_TCP_INITIAL_MSS(IPv6_HDRLEN)
#  define MAX_IPv6_TCP_INITIAL_MSS MAX_TCP_INITIAL_MSS(IPv6_HDRLEN)
#endif

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/* TCP header */

struct tcp_hdr_s
{
  uint16_t srcport;
  uint16_t destport;
  uint8_t  seqno[4];
  uint8_t  ackno[4];
  uint8_t  tcpoffset;
  uint8_t  flags;
  uint8_t  wnd[2];
  uint16_t tcpchksum;
  uint8_t  urgp[2];
  uint8_t  optdata[4];
};

/* The structure holding the TCP/IP statistics that are gathered if
 * CONFIG_NET_STATISTICS is defined.
 */

#ifdef CONFIG_NET_STATISTICS
struct tcp_stats_s
{
  net_stats_t drop;       /* Number of dropped TCP segments */
  net_stats_t recv;       /* Number of received TCP segments */
  net_stats_t sent;       /* Number of sent TCP segments */
  net_stats_t chkerr;     /* Number of TCP segments with a bad checksum */
  net_stats_t ackerr;     /* Number of TCP segments with a bad ACK number */
  net_stats_t rst;        /* Number of received TCP RST (reset) segments */
  net_stats_t rexmit;     /* Number of retransmitted TCP segments */
  net_stats_t syndrop;    /* Number of dropped SYNs due to too few
                             available connections */
  net_stats_t synrst;     /* Number of SYNs for closed ports triggering a RST */
};
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#endif /* CONFIG_NET_TCP */
#endif /* __INCLUDE_NUTTX_NET_TCP_H */
