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
 * Structure used by apps whose drivers access SDIO drivers.
 * Pulled out separately so dhdu and wlu can both use it.
 *
 * $Id: sdiovar.h 241182 2011-02-17 21:50:03Z gmo $
 */

#ifndef _sdiovar_h_
#define _sdiovar_h_

#include <typedefs.h>

/* require default structure packing */
#define BWL_DEFAULT_PACKING
#include <packed_section_start.h>

typedef struct sdreg {
	int func;
	int offset;
	int value;
} sdreg_t;

/* Common msglevel constants */
#define SDH_ERROR_VAL		0x0001	/* Error */
#define SDH_TRACE_VAL		0x0002	/* Trace */
#define SDH_INFO_VAL		0x0004	/* Info */
#define SDH_DEBUG_VAL		0x0008	/* Debug */
#define SDH_DATA_VAL		0x0010	/* Data */
#define SDH_CTRL_VAL		0x0020	/* Control Regs */
#define SDH_LOG_VAL		0x0040	/* Enable bcmlog */
#define SDH_DMA_VAL		0x0080	/* DMA */

#define NUM_PREV_TRANSACTIONS	16

#ifdef BCMSPI
/* Error statistics for gSPI */
struct spierrstats_t {
	uint32  dna;	/* The requested data is not available. */
	uint32  rdunderflow;	/* FIFO underflow happened due to current (F2, F3) rd command */
	uint32  wroverflow;	/* FIFO underflow happened due to current (F1, F2, F3) wr command */

	uint32  f2interrupt;	/* OR of all F2 related intr status bits. */
	uint32  f3interrupt;	/* OR of all F3 related intr status bits. */

	uint32  f2rxnotready;	/* F2 FIFO is not ready to receive data (FIFO empty) */
	uint32  f3rxnotready;	/* F3 FIFO is not ready to receive data (FIFO empty) */

	uint32  hostcmddataerr;	/* Error in command or host data, detected by CRC/checksum
	                         * (optional)
	                         */
	uint32  f2pktavailable;	/* Packet is available in F2 TX FIFO */
	uint32  f3pktavailable;	/* Packet is available in F2 TX FIFO */

	uint32	dstatus[NUM_PREV_TRANSACTIONS];	/* dstatus bits of last 16 gSPI transactions */
	uint32  spicmd[NUM_PREV_TRANSACTIONS];
};
#endif /* BCMSPI */

#include <packed_section_end.h>

#endif /* _sdiovar_h_ */
