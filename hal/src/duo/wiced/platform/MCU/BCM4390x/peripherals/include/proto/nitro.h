/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * Nintendo 802.11 Nitro protocol definitions
 *
 * $Id: nitro.h 382882 2013-02-04 23:24:31Z xwei $
 */

#ifndef _nitro_h_
#define _nitro_h_

#ifndef _TYPEDEFS_H_
#include <typedefs.h>
#endif

/* This marks the start of a packed structure section. */
#include <packed_section_start.h>


#ifdef WLNINTENDO

#define NINTENDO_OUI               "\x00\x09\xBF"  /* Nintendo OUI */
#define NINTENDO_OUI_LEN           3               /* OUI len */
#define NITRO_MP_MACADDR           "\x03\x09\xBF\x00\x00\x00"    /* DA of MP frame */
#define NITRO_MPACK_MACADDR        "\x03\x09\xBF\x00\x00\x03"    /* DA of MPACK frame */
#define NITRO_KEYDATA_MACADDR      "\x03\x09\xBF\x00\x00\x10"    /* DA of KEYDATA frame */

#define NITRO_MP_MAXLEN         516  /* Mp frame maxlen */
#define NITRO_KEYDATA_MAXLEN    516  /* Keydata frame maxlen */

/* Definition of Game IE advertised in the beacon frame */
BWL_PRE_PACKED_STRUCT struct nitro_gameIE {
	uint8         id;           /* IE ID, 221, DOT11_MNG_PROPR_ID */
	uint8         len;          /* IE length */
	uint8         oui[NINTENDO_OUI_LEN];    /* Nintendo OUI -00:09:BF */
	uint8         reserved;
	uint16        activeZone;   /* active zone value; 0xffff if PM not enabled */
	uint16        vblankTsf;    /* current Vblank TSF value */
	uint8         gameInfo[1];  /* Game info */
} BWL_POST_PACKED_STRUCT;
typedef struct nitro_gameIE nitro_gameIE_t;


#define NITRO_GAMEIE_FIXEDLEN 10 /* Game IE fixed field len */
/* Definition of fake IE to pass extra args in beacon Rx indication */
BWL_PRE_PACKED_STRUCT struct nitro_bcnphy_ie {
	uint8         id;           /* IE ID, 0, N.A. */
	uint8         len;          /* IE length */
	int8          rssi;         /* rssi */
	uint16        rate;         /* rate */
} BWL_POST_PACKED_STRUCT;
typedef struct nitro_bcnphy_ie nitro_bcnphy_ie_t;

#define NITRO_BCNPHY_LEN sizeof(nitro_bcnphy_ie_t)


/* Definition of MP frame */
BWL_PRE_PACKED_STRUCT struct nitro_mpfrm {
	uint16        txop;         /* time (us) alloted for child transmission */
	uint16        pollbitmap;   /* bitmap of children polled */
	uint16        wmheader;     /* wm header */
	uint8         data[1];      /* data */
} BWL_POST_PACKED_STRUCT;
typedef struct nitro_mpfrm nitro_mpfrm_t;

#define NITRO_MPFRM_HDRLEN	6   /* MP frame header len */

/* Definition of Keydata frame */
BWL_PRE_PACKED_STRUCT struct nitro_keydatafrm {
	uint16        wmheader;     /* wmheader */
	uint8         data[1];      /* data */
} BWL_POST_PACKED_STRUCT;
typedef struct nitro_keydatafrm nitro_keydatafrm_t;

#define NITRO_KEYDATA_FRM_HDRLEN  2    /* Keydata frame header len */

/* Definition of MPACK frame */
BWL_PRE_PACKED_STRUCT struct nitro_mpackfrm {
	uint16        tmptt;        /* time (10us) to start of next MP seq */
	uint16        pollbitmap;   /* bitmap of children polled */
} BWL_POST_PACKED_STRUCT;
typedef struct nitro_mpackfrm nitro_mpackfrm_t;

#define NITRO_MPACKFRM_HDRLEN	4   /* MP frame header len */

#endif /* WLNINTENDO */

/* This marks the end of a packed structure section. */
#include <packed_section_end.h>

#endif /* _nitro_h_ */
