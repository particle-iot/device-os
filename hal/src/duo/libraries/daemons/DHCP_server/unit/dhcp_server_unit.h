/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *  Unit Tester for DHCP server
 *
 *  Runs a suite of tests on the DHCP server to attempt
 *  to discover bugs
 */
#ifndef INCLUDED_DHCP_SERVER_UNIT_H_
#define INCLUDED_DHCP_SERVER_UNIT_H_

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#pragma pack(1)
typedef struct
{
    uint8_t  opcode;                     /* packet opcode type */
    uint8_t  hardware_type;              /* hardware addr type */
    uint8_t  hardware_addr_len;          /* hardware addr length */
    uint8_t  hops;                       /* gateway hops */
    uint32_t transaction_id;             /* transaction ID */
    uint16_t second_elapsed;             /* seconds since boot began */
    uint16_t flags;
    uint8_t  client_ip_addr[4];          /* client IP address */
    uint8_t  your_ip_addr[4];            /* 'your' IP address */
    uint8_t  server_ip_addr[4];          /* server IP address */
    uint8_t  gateway_ip_addr[4];         /* gateway IP address */
    uint8_t  client_hardware_addr[16];   /* client hardware address */
    uint8_t  legacy[192];
    /* as of RFC2131 it is variable length */
} dhcp_header_t;



#pragma pack()

#define BOOTP_OP_REQUEST                (1)
#define BOOTP_OP_REPLY                  (2)

#define BOOTP_HTYPE_ETHERNET            (1)

/* DHCP commands */
#define DHCPDISCOVER                    (1)
#define DHCPOFFER                       (2)
#define DHCPREQUEST                     (3)
#define DHCPDECLINE                     (4)
#define DHCPACK                         (5)
#define DHCPNAK                         (6)
#define DHCPRELEASE                     (7)
#define DHCPINFORM                      (8)

#define DHCP_MAGIC_COOKIE               0x63, 0x82, 0x53, 0x63

#define DHCP_OPTION_CODE_REQUEST_IP_ADDRESS (50)
#define DHCP_OPTION_CODE_LEASE_TIME         (51)
#define DHCP_OPTION_CODE_DHCP_MESSAGE_TYPE  (53)
#define DHCP_OPTION_CODE_SERVER_ID          (54)
#define DHCP_OPTION_CODE_PARAM_REQ_LIST     (55)
#define DHCP_OPTION_CODE_MAX_MSG_SIZE       (57)
#define DHCP_OPTION_CODE_CLIENT_ID          (61)
#define DHCP_OPTION_END                     (255)


typedef struct
{
        const char* bytes;
        int size;
} dhcp_option_t;

#define MAKE_DHCP_OPTION( var ) { (var), sizeof(var) }

extern const dhcp_header_t normal_dhcp_discover_header;
extern const dhcp_option_t normal_dhcp_discover_options[];
extern const dhcp_option_t dhcp_discover_w_request_options[];

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* ifndef INCLUDED_DHCP_SERVER_UNIT_H_ */
