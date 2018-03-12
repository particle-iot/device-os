/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * Generic Broadcom Home Networking Division (HND) DMA engine SW interface
 * This supports the following chips: BCM42xx, 44xx, 47xx .
 *
 * $Id: hnddma.h 321146 2012-03-14 08:27:23Z deepakd $
 */

#ifndef    _m2m_hnddma_h_
#define    _m2m_hnddma_h_

#include <stdint.h>
#include "bcmdefs.h"
#include "typedefs.h"
#include "siutils.h"

#ifndef _hnddma_pub_
#define _hnddma_pub_
typedef const struct m2m_hnddma_pub m2m_hnddma_t;
//#include "hnddma.h"
#endif /* _hnddma_pub_ */

/* range param for dma_getnexttxp() and dma_txreclaim */
typedef enum {
    HNDDMA_RANGE_ALL        = 1,
    HNDDMA_RANGE_TRANSMITTED,
    HNDDMA_RANGE_TRANSFERED
} txd_range_t;





void m2m_dma_detach(m2m_hnddma_t *di);
void m2m_dma_txinit(m2m_hnddma_t *di);
int m2m_dma_txactive(m2m_hnddma_t *di_in);
int BCMFASTPATH m2m_dma_txfast(m2m_hnddma_t *di, void *p0, bool commit);
void BCMFASTPATH m2m_dma_txreclaim(m2m_hnddma_t *di, txd_range_t range);
void * BCMFASTPATH m2m_dma_getnexttxp(m2m_hnddma_t *di, txd_range_t range);
void m2m_dma_rxinit(m2m_hnddma_t *di);
bool m2m_dma_txreset(m2m_hnddma_t *di);
bool m2m_dma_rxreset(m2m_hnddma_t *di);
void * BCMFASTPATH m2m_dma_rx(m2m_hnddma_t *di);
bool BCMFASTPATH m2m_dma_rxfill(m2m_hnddma_t *di);
int m2m_dma_rxactive(m2m_hnddma_t *di_in);
void m2m_dma_rxreclaim(m2m_hnddma_t *di);
void m2m_dma_fifoloopbackenable(m2m_hnddma_t *di);
uint m2m_dma_ctrlflags(m2m_hnddma_t *di, uint mask, uint flags);
bool m2m_dma_txreset_inner(void *osh, void *txregs);
bool m2m_dma_rxreset_inner(void *osh, void *rxregs);

/*
 * Exported data structure (read-only)
 */
/* export structure */
struct m2m_hnddma_pub {
    uint        txavail;    /* # free tx descriptors */
    uint        dmactrlflags;    /* dma control flags */

    /* rx error counters */
    uint        rxgiants;    /* rx giant frames */
    uint        rxnobuf;    /* rx out of dma descriptors */
    /* tx error counters */
    uint        txnobuf;    /* tx out of dma descriptors */
    uint        txnodesc;    /* tx out of dma descriptors running count */
};


extern m2m_hnddma_t * m2m_dma_attach(osl_t *osh, const char *name, si_t *sih,
    volatile void *dmaregstx, volatile void *dmaregsrx,
    uint ntxd, uint nrxd, uint rxbufsize, int rxextheadroom, uint nrxpost,
    uint rxoffset, uint *msg_level);

#endif    /* _m2m_hnddma_h_ */
