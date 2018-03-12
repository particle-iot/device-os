/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef _et_wiced_h_
#define _et_wiced_h_

#include "platform_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* defaults */
#ifndef WICED_ETHERNET_DESCNUM_TX
#define WICED_ETHERNET_DESCNUM_TX 8
#endif
#ifndef WICED_ETHERNET_DESCNUM_RX
#define WICED_ETHERNET_DESCNUM_RX 4
#endif

/* tunables */
#define NTXD                  WICED_ETHERNET_DESCNUM_TX               /* # tx dma ring descriptors (must be ^2) */
#define NRXD                  WICED_ETHERNET_DESCNUM_RX               /* # rx dma ring descriptors (must be ^2) */
#define NRXBUFPOST            MAX((WICED_ETHERNET_DESCNUM_RX) / 2, 1) /* try to keep this # rbufs posted to the chip */
#define RXBUFSZ               WICED_ETHERNET_BUFFER_SIZE              /* receive buffer size */

/* dma tunables */
#ifndef TXMR
#define TXMR                  2 /* number of outstanding reads */
#endif

#ifndef TXPREFTHRESH
#define TXPREFTHRESH          8 /* prefetch threshold */
#endif

#ifndef TXPREFCTL
#define TXPREFCTL            16 /* max descr allowed in prefetch request */
#endif

#ifndef TXBURSTLEN
#define TXBURSTLEN          128 /* burst length for dma reads */
#endif

#ifndef RXPREFTHRESH
#define RXPREFTHRESH          1 /* prefetch threshold */
#endif

#ifndef RXPREFCTL
#define RXPREFCTL             8 /* max descr allowed in prefetch request */
#endif

#ifndef RXBURSTLEN
#define RXBURSTLEN          128 /* burst length for dma writes */
#endif


/****************************** internal **************************/

#define IS_POWER_OF_2(x)          (((x) != 0) && (((x) & (~(x) + 1)) == (x)))


static inline void wiced_osl_static_asserts(void)
{
    STATIC_ASSERT(IS_POWER_OF_2(NTXD));
    STATIC_ASSERT(IS_POWER_OF_2(NRXD));
}


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _et_wiced_h_ */
