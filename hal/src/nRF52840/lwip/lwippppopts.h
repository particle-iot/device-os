/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HAL_LWIP_LWIPPPPOPTS_H
#define HAL_LWIP_LWIPPPPOPTS_H

/**
 * PPP_SUPPORT==1: Enable PPP.
 */
#define PPP_SUPPORT                     1

/**
 * PPPOE_SUPPORT==1: Enable PPP Over Ethernet
 */
#define PPPOE_SUPPORT                   0

/**
 * PPPOL2TP_SUPPORT==1: Enable PPP Over L2TP
 */
#define PPPOL2TP_SUPPORT                0

/**
 * PPPOL2TP_AUTH_SUPPORT==1: Enable PPP Over L2TP Auth (enable MD5 support)
 */
#define PPPOL2TP_AUTH_SUPPORT           PPPOL2TP_SUPPORT

/**
 * PPPOS_SUPPORT==1: Enable PPP Over Serial
 */
#define PPPOS_SUPPORT                   PPP_SUPPORT

/**
 * LWIP_PPP_API==1: Enable PPP API (in pppapi.c)
 */
#define LWIP_PPP_API                    (PPP_SUPPORT && (NO_SYS == 0))

/**
 * MEMP_NUM_PPP_PCB: the number of simultaneously active PPP
 * connections (requires the PPP_SUPPORT option)
 */
#define MEMP_NUM_PPP_PCB                2

/**
 * PPP_NUM_TIMEOUTS_PER_PCB: the number of sys_timeouts running in parallel per
 * ppp_pcb. This is a conservative default which needs to be checked...
 */
#define PPP_NUM_TIMEOUTS_PER_PCB        6

/* The number of sys_timeouts required for the PPP module */
// #define PPP_NUM_TIMEOUTS                (PPP_SUPPORT * PPP_NUM_TIMEOUTS_PER_PCB * MEMP_NUM_PPP_PCB)

#if PPP_SUPPORT

/**
 * MEMP_NUM_PPPOS_INTERFACES: the number of concurrently active PPPoS
 * interfaces (only used with PPPOS_SUPPORT==1)
 */
#define MEMP_NUM_PPPOS_INTERFACES       MEMP_NUM_PPP_PCB

/**
 * MEMP_NUM_PPPOE_INTERFACES: the number of concurrently active PPPoE
 * interfaces (only used with PPPOE_SUPPORT==1)
 */
#define MEMP_NUM_PPPOE_INTERFACES       1

/**
 * MEMP_NUM_PPPOL2TP_INTERFACES: the number of concurrently active PPPoL2TP
 * interfaces (only used with PPPOL2TP_SUPPORT==1)
 */
#define MEMP_NUM_PPPOL2TP_INTERFACES       1

/**
 * MEMP_NUM_PPP_API_MSG: Number of concurrent PPP API messages (in pppapi.c)
 */
#define MEMP_NUM_PPP_API_MSG 5

/**
 * PPP_DEBUG: Enable debugging for PPP.
 */
#define PPP_DEBUG                       LWIP_DBG_ON

/**
 * PPP_INPROC_IRQ_SAFE==1 call pppos_input() using tcpip_callback().
 *
 * Please read the "PPPoS input path" chapter in the PPP documentation about this option.
 */
#define PPP_INPROC_IRQ_SAFE             0

/**
 * PRINTPKT_SUPPORT==1: Enable PPP print packet support
 *
 * Mandatory for debugging, it displays exchanged packet content in debug trace.
 */
#define PRINTPKT_SUPPORT                1

/**
 * PPP_IPV4_SUPPORT==1: Enable PPP IPv4 support
 */
#define PPP_IPV4_SUPPORT                (LWIP_IPV4)

/**
 * PPP_IPCP_OVERRIDE==1: third-party IPCP implementation
 */
#define PPP_IPCP_OVERRIDE               1

/**
 * PPP_IPV6_SUPPORT==1: Enable PPP IPv6 support
 */
#define PPP_IPV6_SUPPORT                (LWIP_IPV6)

/**
 * PPP_IPV6CP_OVERRIDE==1: third-party IPV6CP implementation
 */
#define PPP_IPV6CP_OVERRIDE             0

/**
 * PPP_NOTIFY_PHASE==1: Support PPP notify phase support
 *
 * PPP notify phase support allows you to set a callback which is
 * called on change of the internal PPP state machine.
 *
 * This can be used for example to set a LED pattern depending on the
 * current phase of the PPP session.
 */
#define PPP_NOTIFY_PHASE                1

/**
 * pbuf_type PPP is using for LCP, PAP, CHAP, EAP, CCP, IPCP and IP6CP packets.
 *
 * Memory allocated must be single buffered for PPP to works, it requires pbuf
 * that are not going to be chained when allocated. This requires setting
 * PBUF_POOL_BUFSIZE to at least 512 bytes, which is quite huge for small systems.
 *
 * Setting PPP_USE_PBUF_RAM to 1 makes PPP use memory from heap where continuous
 * buffers are required, allowing you to use a smaller PBUF_POOL_BUFSIZE.
 */
#define PPP_USE_PBUF_RAM                0

/**
 * PPP_FCS_TABLE: Keep a 256*2 byte table to speed up FCS calculation for PPPoS
 */
#define PPP_FCS_TABLE                   1

/**
 * PAP_SUPPORT==1: Support PAP.
 */
#define PAP_SUPPORT                     1

/**
 * CHAP_SUPPORT==1: Support CHAP.
 */
#define CHAP_SUPPORT                    1

/**
 * MSCHAP_SUPPORT==1: Support MSCHAP.
 */
#define MSCHAP_SUPPORT                  0
#if MSCHAP_SUPPORT
/* MSCHAP requires CHAP support */
#undef CHAP_SUPPORT
#define CHAP_SUPPORT                    1
#endif /* MSCHAP_SUPPORT */

/**
 * EAP_SUPPORT==1: Support EAP.
 */
#define EAP_SUPPORT                     0

/**
 * CCP_SUPPORT==1: Support CCP.
 */
#define CCP_SUPPORT                     0

/**
 * MPPE_SUPPORT==1: Support MPPE.
 */
#define MPPE_SUPPORT                    0
#if MPPE_SUPPORT
/* MPPE requires CCP support */
#undef CCP_SUPPORT
#define CCP_SUPPORT                     1
/* MPPE requires MSCHAP support */
#undef MSCHAP_SUPPORT
#define MSCHAP_SUPPORT                  1
/* MSCHAP requires CHAP support */
#undef CHAP_SUPPORT
#define CHAP_SUPPORT                    1
#endif /* MPPE_SUPPORT */

/**
 * CBCP_SUPPORT==1: Support CBCP. CURRENTLY NOT SUPPORTED! DO NOT SET!
 */
#define CBCP_SUPPORT                    0

/**
 * ECP_SUPPORT==1: Support ECP. CURRENTLY NOT SUPPORTED! DO NOT SET!
 */
#define ECP_SUPPORT                     0

/**
 * DEMAND_SUPPORT==1: Support dial on demand. CURRENTLY NOT SUPPORTED! DO NOT SET!
 */
#define DEMAND_SUPPORT                  0

/**
 * LQR_SUPPORT==1: Support Link Quality Report. Do nothing except exchanging some LCP packets.
 */
#define LQR_SUPPORT                     0

/**
 * PPP_SERVER==1: Enable PPP server support (waiting for incoming PPP session).
 *
 * Currently only supported for PPPoS.
 */
#define PPP_SERVER                      1

#if PPP_SERVER
/*
 * PPP_OUR_NAME: Our name for authentication purposes
 */
#define PPP_OUR_NAME                    "lwIP"
#endif /* PPP_SERVER */

/**
 * VJ_SUPPORT==1: Support VJ header compression.
 */
#define VJ_SUPPORT                      0
/* VJ compression is only supported for TCP over IPv4 over PPPoS. */
#if !PPPOS_SUPPORT || !PPP_IPV4_SUPPORT || !LWIP_TCP
#undef VJ_SUPPORT
#define VJ_SUPPORT                      0
#endif /* !PPPOS_SUPPORT */

/**
 * PPP_MD5_RANDM==1: Use MD5 for better randomness.
 * Enabled by default if CHAP, EAP, or L2TP AUTH support is enabled.
 */
#define PPP_MD5_RANDM                   (CHAP_SUPPORT || EAP_SUPPORT || PPPOL2TP_AUTH_SUPPORT)

/**
 * PolarSSL embedded library
 *
 *
 * lwIP contains some files fetched from the latest BSD release of
 * the PolarSSL project (PolarSSL 0.10.1-bsd) for ciphers and encryption
 * methods we need for lwIP PPP support.
 *
 * The PolarSSL files were cleaned to contain only the necessary struct
 * fields and functions needed for lwIP.
 *
 * The PolarSSL API was not changed at all, so if you are already using
 * PolarSSL you can choose to skip the compilation of the included PolarSSL
 * library into lwIP.
 *
 * If you are not using the embedded copy you must include external
 * libraries into your arch/cc.h port file.
 *
 * Beware of the stack requirements which can be a lot larger if you are not
 * using our cleaned PolarSSL library.
 */

/**
 * LWIP_USE_EXTERNAL_POLARSSL: Use external PolarSSL library
 */
#define LWIP_USE_EXTERNAL_POLARSSL      0

/**
 * LWIP_USE_EXTERNAL_MBEDTLS: Use external mbed TLS library
 */
#define LWIP_USE_EXTERNAL_MBEDTLS       0

/*
 * PPP Timeouts
 */

/**
 * FSM_DEFTIMEOUT: Timeout time in seconds
 */
#define FSM_DEFTIMEOUT                  6

/**
 * FSM_DEFMAXTERMREQS: Maximum Terminate-Request transmissions
 */
#define FSM_DEFMAXTERMREQS              2

/**
 * FSM_DEFMAXCONFREQS: Maximum Configure-Request transmissions
 */
#define FSM_DEFMAXCONFREQS              10

/**
 * FSM_DEFMAXNAKLOOPS: Maximum number of nak loops
 */
#define FSM_DEFMAXNAKLOOPS              5

/**
 * UPAP_DEFTIMEOUT: Timeout (seconds) for retransmitting req
 */
#define UPAP_DEFTIMEOUT                 6

/**
 * UPAP_DEFTRANSMITS: Maximum number of auth-reqs to send
 */
#define UPAP_DEFTRANSMITS               10

#if PPP_SERVER
/**
 * UPAP_DEFREQTIME: Time to wait for auth-req from peer
 */
#define UPAP_DEFREQTIME                 30
#endif /* PPP_SERVER */

/**
 * CHAP_DEFTIMEOUT: Timeout (seconds) for retransmitting req
 */
#define CHAP_DEFTIMEOUT                 6

/**
 * CHAP_DEFTRANSMITS: max # times to send challenge
 */
#define CHAP_DEFTRANSMITS               10

#if PPP_SERVER
/**
 * CHAP_DEFRECHALLENGETIME: If this option is > 0, rechallenge the peer every n seconds
 */
#define CHAP_DEFRECHALLENGETIME         0
#endif /* PPP_SERVER */

/**
 * EAP_DEFREQTIME: Time to wait for peer request
 */
#define EAP_DEFREQTIME                  6

/**
 * EAP_DEFALLOWREQ: max # times to accept requests
 */
#define EAP_DEFALLOWREQ                 10

#if PPP_SERVER
/**
 * EAP_DEFTIMEOUT: Timeout (seconds) for rexmit
 */
#define EAP_DEFTIMEOUT                  6

/**
 * EAP_DEFTRANSMITS: max # times to transmit
 */
#define EAP_DEFTRANSMITS                10
#endif /* PPP_SERVER */

/**
 * LCP_DEFLOOPBACKFAIL: Default number of times we receive our magic number from the peer
 * before deciding the link is looped-back.
 */
#define LCP_DEFLOOPBACKFAIL             10

/**
 * LCP_ECHOINTERVAL: Interval in seconds between keepalive echo requests, 0 to disable.
 */
#define LCP_ECHOINTERVAL                0

/**
 * LCP_MAXECHOFAILS: Number of unanswered echo requests before failure.
 */
#define LCP_MAXECHOFAILS                3

/**
 * PPP_MAXIDLEFLAG: Max Xmit idle time (in ms) before resend flag char.
 */
#define PPP_MAXIDLEFLAG                 100

/**
 * PPP Packet sizes
 */

/**
 * PPP_MRU: Default MRU
 */
#define PPP_MRU                         1500

/**
 * PPP_DEFMRU: Default MRU to try
 */
#define PPP_DEFMRU                      1500

/**
 * PPP_MAXMRU: Normally limit MRU to this (pppd default = 16384)
 */
#define PPP_MAXMRU                      1500

/**
 * PPP_MINMRU: No MRUs below this
 */
#define PPP_MINMRU                      128

/**
 * PPPOL2TP_DEFMRU: Default MTU and MRU for L2TP
 * Default = 1500 - PPPoE(6) - PPP Protocol(2) - IPv4 header(20) - UDP Header(8)
 * - L2TP Header(6) - HDLC Header(2) - PPP Protocol(2) - MPPE Header(2) - PPP Protocol(2)
 */
#if PPPOL2TP_SUPPORT
#ifndef PPPOL2TP_DEFMRU
#define PPPOL2TP_DEFMRU                 1450
#endif
#endif /* PPPOL2TP_SUPPORT */

/**
 * MAXNAMELEN: max length of hostname or name for auth
 */
#define MAXNAMELEN                      256

/**
 * MAXSECRETLEN: max length of password or secret
 */
#define MAXSECRETLEN                    256

/* ------------------------------------------------------------------------- */

#endif /* PPP_SUPPORT */

#endif /* HAL_LWIP_LWIPPPPOPTS_H */
