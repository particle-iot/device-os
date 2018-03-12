/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * Broadcom Gigabit Ethernet MAC defines.
 *
 * $Id: etcgmac.h 459261 2014-03-02 22:57:00Z fgomes $
 */
#ifndef _etcgmac_h_
#define _etcgmac_h_

/* chip interrupt bit error summary */
#define	I_ERRORS		(I_PDEE | I_PDE | I_DE | I_RDU | I_RFO | I_XFU)

#if defined(BCM_GMAC3)
#define	DEF_INTMASK		(I_XI0 | I_XI1 | I_XI2 | I_XI3 | I_RI | I_ERRORS | I_TO)
#else  /* ! BCM_GMAC3 */
#define DEF_INTMASK     (I_XI0 | I_XI1 | I_XI2 | I_XI3 | I_RI | I_ERRORS)
#endif /* ! BCM_GMAC3 */

#define GMAC_RESET_DELAY 	2

#define GMAC_MIN_FRAMESIZE	17	/* gmac can only send frames of
	                                 * size above 17 octetes.
	                                 */

#define DMAREG(ch, dir, qnum)	((dir == DMA_TX) ? \
	                         (void *)(uintptr)&(ch->regs->dmaregs[qnum].dmaxmt) : \
	                         (void *)(uintptr)&(ch->regs->dmaregs[qnum].dmarcv))

/*
 * Add multicast address to the list. Multicast address are maintained as
 * hash table with chaining.
 */
typedef struct mclist {
	struct ether_addr mc_addr;	/* multicast address to allow */
	struct mclist *next;		/* next entry */
} mflist_t;

#define GMAC_HASHT_SIZE		16	/* hash table size */
#define GMAC_MCADDR_HASH(m)	((((uint8 *)(m))[3] + ((uint8 *)(m))[4] + \
	                         ((uint8 *)(m))[5]) & (GMAC_HASHT_SIZE - 1))

#define ETHER_MCADDR_CMP(x, y) ((((uint16 *)(x))[0] ^ ((uint16 *)(y))[0]) | \
				(((uint16 *)(x))[1] ^ ((uint16 *)(y))[1]) | \
				(((uint16 *)(x))[2] ^ ((uint16 *)(y))[2]))

#define SUCCESS			0
#define FAILURE			-1

typedef struct mcfilter {
					/* hash table for multicast filtering */
	mflist_t *bucket[GMAC_HASHT_SIZE];
} mcfilter_t;

extern uint32 find_priq(uint32 pri_map);

#endif /* _etcgmac_h_ */
