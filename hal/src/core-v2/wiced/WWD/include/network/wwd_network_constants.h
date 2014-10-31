/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  Provides packet size constants which are useful for implementations of the
 *  network interface and buffer interface.
 */


#ifndef INCLUDED_WWD_NETWORK_CONSTANTS_H_
#define INCLUDED_WWD_NETWORK_CONSTANTS_H_

#include "wwd_buffer_interface.h"
#include "wwd_bus_protocol.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 * @cond       Constants
 ******************************************************/

/**
 * The maximum size of the wwd_buffer_header_t structure (i.e. largest bus implementation)
 */
#define MAX_BUS_HEADER_LENGTH (12)

/**
 * The maximum size of the SDPCM + BDC header, including offsets and reserved space
 * 12 bytes - SDPCM header
 * 2 bytes  - Extra offset for SDPCM headers that come as 14 bytes
 * 4 bytes  - BDC header
 * 4 bytes  - offset for one extra BDC reserved long int.
 */
#define MAX_SDPCM_HEADER_LENGTH (22)

/**
 * The space in bytes required for headers in front of the Ethernet header.
 */
#define WICED_LINK_OVERHEAD_BELOW_ETHERNET_FRAME ( sizeof(wwd_buffer_header_t) + WWD_SDPCM_HEADER_RESERVED_LENGTH )

/**
 * The maximum space in bytes required for headers in front of the Ethernet header.
 * This definition allows WICED to use a pre-built bus-generic network stack library regardless of choice of bus.
 * Note: adjust accordingly if a new bus is added.
 */
#define WICED_LINK_OVERHEAD_BELOW_ETHERNET_FRAME_MAX ( MAX_BUS_HEADER_LENGTH + MAX_SDPCM_HEADER_LENGTH )

/**
 * The space in bytes required after the end of an Ethernet packet
 */
#define WICED_LINK_TAIL_AFTER_ETHERNET_FRAME     ( 0 )

/**
 * The size of an Ethernet header
 */
#define WICED_ETHERNET_SIZE         (14)

/**
 * The size in bytes of the Link layer header i.e. the Wiced specific headers and the Ethernet header
 */
#define WICED_PHYSICAL_HEADER       (WICED_LINK_OVERHEAD_BELOW_ETHERNET_FRAME_MAX + WICED_ETHERNET_SIZE)

/**
 * The size in bytes of the data after Link layer packet
 */
#define WICED_PHYSICAL_TRAILER      (WICED_LINK_TAIL_AFTER_ETHERNET_FRAME)

/**
 * The maximum size in bytes of the data part of an Ethernet frame
 */
#ifndef WICED_PAYLOAD_MTU
#define WICED_PAYLOAD_MTU           (1500)
#endif

/**
 * The maximum size in bytes of a packet used within Wiced
 */
#define WICED_LINK_MTU              (WICED_PAYLOAD_MTU + WICED_PHYSICAL_HEADER + WICED_PHYSICAL_TRAILER)


/**
 * Ethernet Ethertypes
 */
#define WICED_ETHERTYPE_IPv4    0x0800
#define WICED_ETHERTYPE_IPv6    0x86DD
#define WICED_ETHERTYPE_ARP     0x0806
#define WICED_ETHERTYPE_RARP    0x8035
#define WICED_ETHERTYPE_EAPOL   0x888E
#define WICED_ETHERTYPE_8021Q   0x8100

/** @endcond */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ifndef INCLUDED_WWD_NETWORK_CONSTANTS_H_ */
