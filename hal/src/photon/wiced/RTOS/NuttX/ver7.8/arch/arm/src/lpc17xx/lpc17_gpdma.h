/****************************************************************************
 * arch/arm/src/lpc17xx/lpc17_gpdma.h
 *
 *   Copyright (C) 2010, 2013 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_LPC17XX_LPC17_GPDMA_H
#define __ARCH_ARM_SRC_LPC17XX_LPC17_GPDMA_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include "chip/lpc17_gpdma.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

#ifdef CONFIG_LPC17_GPDMA
/* DMA_HANDLE is an opaque reference to an allocated DMA channel */

typedef FAR void *DMA_HANDLE;

/* dma_callback_t a function pointer provided to lpc17_dmastart.  This
 * function is called at the completion of the DMA transfer.  'arg' is the
 * same 'arg' value that was provided when lpc17_dmastart() was called and
 * result indicates the result of the transfer:  Zero indicates a successful
 * tranfers.  On failure, a negated errno is returned indicating the general
 * nature of the DMA faiure.
 */

typedef void (*dma_callback_t)(DMA_HANDLE handle, void *arg, int result);

/* The following is used for sampling DMA registers when CONFIG DEBUG_DMA is selected */

#ifdef CONFIG_DEBUG_DMA
struct lpc17_dmaglobalregs_s
{
  /* Global Registers */

  uint32_t intst;       /* DMA Interrupt Status Register */
  uint32_t inttcst;     /* DMA Interrupt Terminal Count Request Status Register */
  uint32_t interrst;    /* DMA Interrupt Error Status Register */
  uint32_t rawinttcst;  /* DMA Raw Interrupt Terminal Count Status Register */
  uint32_t rawinterrst; /* DMA Raw Error Interrupt Status Register */
  uint32_t enbldchns;   /* DMA Enabled Channel Register */
  uint32_t softbreq;    /* DMA Software Burst Request Register */
  uint32_t softsreq;    /* DMA Software Single Request Register */
  uint32_t softlbreq;   /* DMA Software Last Burst Request Register */
  uint32_t softlsreq;   /* DMA Software Last Single Request Register */
  uint32_t config;      /* DMA Configuration Register */
  uint32_t sync;        /* DMA Synchronization Register */
};

struct lpc17_dmachanregs_s
{
  /* Channel Registers */

  uint32_t srcaddr;  /* DMA Channel Source Address Register */
  uint32_t destaddr; /* DMA Channel Destination Address Register */
  uint32_t lli;      /* DMA Channel Linked List Item Register */
  uint32_t control;  /* DMA Channel Control Register */
  uint32_t config;   /* DMA Channel Configuration Register */
};

struct lpc17_dmaregs_s
{
  /* Global Registers */

  struct lpc17_dmaglobalregs_s gbl;

  /* Channel Registers */

  struct lpc17_dmachanregs_s   ch;
};

#endif /* CONFIG_DEBUG_DMA */

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef __ASSEMBLY__
#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/* If the following value is zero, then there is no DMA in progress. This
 * value is needed in the IDLE loop to determine if the IDLE loop should
 * go into lower power power consumption modes.  According to the LPC17xx
 * User Manual: "The DMA controller can continue to work in Sleep mode, and
 * has access to the peripheral SRAMs and all peripheral registers. The
 * flash memory and the Main SRAM are not available in Sleep mode, they are
 * disabled in order to save power."
 */

EXTERN volatile uint8_t g_dma_inprogress;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: up_dmainitialize
 *
 * Description:
 *   Initialize the GPDMA subsystem (also prototyped in up_internal.h).
 *
 * Returned Value:
 *   Zero on success; A negated errno value on failure.
 *
 ****************************************************************************/

void weak_function up_dmainitialize(void);

/****************************************************************************
 * Name: lpc17_dmaconfigure
 *
 * Description:
 *   Configure a DMA request.  Each DMA request may have two different DMA
 *   request sources.  This associates one of the sources with a DMA request.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void lpc17_dmaconfigure(uint8_t dmarequest, bool alternate);

/****************************************************************************
 * Name: lpc17_dmachannel
 *
 * Description:
 *   Allocate a DMA channel.  This function sets aside a DMA channel and
 *   gives the caller exclusive access to the DMA channel.
 *
 * Returned Value:
 *   One success, this function returns a non-NULL, void* DMA channel
 *   handle.  NULL is returned on any failure.  This function can fail only
 *   if no DMA channel is available.
 *
 ****************************************************************************/

DMA_HANDLE lpc17_dmachannel(void);

/****************************************************************************
 * Name: lpc17_dmafree
 *
 * Description:
 *   Release a DMA channel.  NOTE:  The 'handle' used in this argument must
 *   NEVER be used again until lpc17_dmachannel() is called again to re-gain
 *   a valid handle.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void lpc17_dmafree(DMA_HANDLE handle);

/****************************************************************************
 * Name: lpc17_dmasetup
 *
 * Description:
 *   Configure DMA for one transfer.
 *
 ****************************************************************************/

int lpc17_dmasetup(DMA_HANDLE handle, uint32_t control, uint32_t config,
                   uint32_t srcaddr, uint32_t destaddr, size_t nxfrs);

/****************************************************************************
 * Name: lpc17_dmastart
 *
 * Description:
 *   Start the DMA transfer
 *
 ****************************************************************************/

int lpc17_dmastart(DMA_HANDLE handle, dma_callback_t callback, void *arg);

/****************************************************************************
 * Name: lpc17_dmastop
 *
 * Description:
 *   Cancel the DMA.  After lpc17_dmastop() is called, the DMA channel is
 *   reset and lpc17_dmasetup() must be called before lpc17_dmastart() can be
 *   called again
 *
 ****************************************************************************/

void lpc17_dmastop(DMA_HANDLE handle);

/****************************************************************************
 * Name: lpc17_dmasample
 *
 * Description:
 *   Sample DMA register contents
 *
 ****************************************************************************/

#ifdef CONFIG_DEBUG_DMA
void lpc17_dmasample(DMA_HANDLE handle, struct lpc17_dmaregs_s *regs);
#else
#  define lpc17_dmasample(handle,regs)
#endif

/****************************************************************************
 * Name: lpc17_dmadump
 *
 * Description:
 *   Dump previously sampled DMA register contents
 *
 ****************************************************************************/

#ifdef CONFIG_DEBUG_DMA
void lpc17_dmadump(DMA_HANDLE handle, const struct lpc17_dmaregs_s *regs,
                   const char *msg);
#else
#  define lpc17_dmadump(handle,regs,msg)
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* CONFIG_LPC17_GPDMA */
#endif /* __ARCH_ARM_SRC_LPC17XX_LPC17_GPDMA_H */
