/*
 * Extensible Authentication Protocol (EAP) definitions
 *
 * See
 * RFC 2284: PPP Extensible Authentication Protocol (EAP)
 *
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: eap.h 405837 2013-06-05 00:13:20Z harveysm $
 */

#ifndef _eap_h_
#define _eap_h_

/* This marks the start of a packed structure section. */
#include <packed_section_start.h>

/* EAP packet format */
typedef BWL_PRE_PACKED_STRUCT struct {
	unsigned char code;	/* EAP code */
	unsigned char id;	/* Current request ID */
	unsigned short length;	/* Length including header */
	unsigned char type;	/* EAP type (optional) */
	unsigned char data[1];	/* Type data (optional) */
} BWL_POST_PACKED_STRUCT eap_header_t;

#define EAP_HEADER_LEN 4

/* EAP codes */
#define EAP_REQUEST	1
#define EAP_RESPONSE	2
#define EAP_SUCCESS	3
#define EAP_FAILURE	4

/* EAP types */
#define EAP_IDENTITY		1
#define EAP_NOTIFICATION	2
#define EAP_NAK			3
#define EAP_MD5			4
#define EAP_OTP			5
#define EAP_GTC			6
#define EAP_TLS			13
#define EAP_EXPANDED		254
#define BCM_EAP_SES		10
#define BCM_EAP_EXP_LEN		12  /* EAP_LEN 5 + 3 bytes for SMI ID + 4 bytes for ven type */
#define BCM_SMI_ID		0x113d
#define WFA_VENDOR_SMI	0x009F68

#ifdef  BCMCCX
#define EAP_LEAP		17

#define LEAP_VERSION		1
#define LEAP_CHALLENGE_LEN	8
#define LEAP_RESPONSE_LEN	24

/* LEAP challenge */
typedef struct {
	unsigned char version;		/* should be value of LEAP_VERSION */
	unsigned char reserved;		/* not used */
	unsigned char chall_len;	/* always value of LEAP_CHALLENGE_LEN */
	unsigned char challenge[LEAP_CHALLENGE_LEN]; /* random */
	unsigned char username[1];
} leap_challenge_t;

#define LEAP_CHALLENGE_HDR_LEN	12

/* LEAP challenge reponse */
typedef struct {
	unsigned char version;	/* should be value of LEAP_VERSION */
	unsigned char reserved;	/* not used */
	unsigned char resp_len;	/* always value of LEAP_RESPONSE_LEN */
	/* MS-CHAP hash of challenge and user's password */
	unsigned char response[LEAP_RESPONSE_LEN];
	unsigned char username[1];
} leap_response_t;

#define LEAP_RESPONSE_HDR_LEN	28

#endif /* BCMCCX */

/* This marks the end of a packed structure section. */
#include <packed_section_end.h>

#endif /* _eap_h_ */
