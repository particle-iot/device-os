/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * BCM43XX M2M DMA core hardware definitions.
 *
 * $Id:m2mdma _core.h 421139 2013-08-30 17:56:15Z kiranm $
 */
#ifndef	_M2MDMA_CORE_H
#define	_M2MDMA_CORE_H

#include <typedefs.h>
#include <sbhnddma.h>
/* cpp contortions to concatenate w/arg prescan */
#ifndef PAD
#define	_PADLINE(line)	pad ## line
#define	_XSTR(line)	_PADLINE(line)
#define	PAD		_XSTR(__LINE__)
#endif


/* dma regs to control the flow between host2dev and dev2host  */
typedef struct m2m_devdmaregs {
	dma64regs_t	tx;
	uint32 		PAD[2];
	dma64regs_t	rx;
	uint32 		PAD[2];
} m2m_devdmaregs_t;


typedef struct dmaintregs {
	uint32 intstatus;
	uint32 intmask;
} dmaintregs_t;

#define I_DE            (1 << 10) /* descriptor read error */
#define I_DA            (1 << 11) /* errors while transferring data to or from memory */
#define I_DP            (1 << 12) /* descriptor programming errors */
#define I_RU            (1 << 13) /* channel cannot process an incoming frame because no descriptors are available */
#define I_RI            (1 << 16) /* interrupt from the RX channel */
#define I_XI            (1 << 24) /* interrupt from the TX error */

#define I_ERRORS        (I_DE | I_DA | I_DP)
#define I_DMA           (I_ERRORS | I_RI)

/* SB side: M2M DMA core registers */
typedef struct sbpm2mregs {
	uint32 control;		/* Core control 0x0 */
	uint32 capabilities;	/* Core capabilities 0x4 */
	uint32 intcontrol;	/* Interrupt control 0x8 */
	uint32 PAD[5];
	uint32 intstatus;	/* Interrupt Status  0x20 */
	uint32 PAD[3];
	dmaintregs_t intregs[8]; /* 0x30 - 0x6c */
	uint32 PAD[36];
	uint32 intrxlazy[8];	/* 0x100 - 0x11c */
	uint32 PAD[48];
	uint32 clockctlstatus;  /* 0x1e0 */
	uint32 workaround;	/* 0x1e4 */
	uint32 powercontrol;	/* 0x1e8 */
	uint32 PAD[5];
	m2m_devdmaregs_t dmaregs[8]; /* 0x200 - 0x3f4 */
} sbm2mregs_t;


#endif	/* _M2MDMA_CORE_H */
