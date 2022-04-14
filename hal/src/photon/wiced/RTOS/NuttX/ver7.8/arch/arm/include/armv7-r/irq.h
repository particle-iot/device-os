/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
/****************************************************************************
 * arch/arm/include/armv7-r/irq.h
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
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

/* This file should never be included directed but, rather, only indirectly
 * through nuttx/irq.h
 */

#ifndef __ARCH_ARM_INCLUDE_ARMV7_R_IRQ_H
#define __ARCH_ARM_INCLUDE_ARMV7_R_IRQ_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/irq.h>

#ifndef __ASSEMBLY__
#  include <stdint.h>
#  include <arch/arch.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* IRQ Stack Frame Format:
 *
 * Context is always saved/restored in the same way:
 *
 *   (1) stmia rx, {r0-r14}
 *   (2) then the PC and CPSR
 *
 * This results in the following set of indices that can be used to access
 * individual registers in the xcp.regs array:
 */

#define REG_R0              (0)
#define REG_R1              (1)
#define REG_R2              (2)
#define REG_R3              (3)
#define REG_R4              (4)
#define REG_R5              (5)
#define REG_R6              (6)
#define REG_R7              (7)
#define REG_R8              (8)
#define REG_R9              (9)
#define REG_R10             (10)
#define REG_R11             (11)
#define REG_R12             (12)
#define REG_R13             (13)
#define REG_R14             (14)
#define REG_R15             (15)
#define REG_CPSR            (16)

#define ARM_CONTEXT_REGS    (17)

/* The total number of registers saved by software */

#define XCPTCONTEXT_REGS    (ARM_CONTEXT_REGS)
#define XCPTCONTEXT_SIZE    (4 * XCPTCONTEXT_REGS)

/* Friendly register names */

#define REG_A1              REG_R0
#define REG_A2              REG_R1
#define REG_A3              REG_R2
#define REG_A4              REG_R3
#define REG_V1              REG_R4
#define REG_V2              REG_R5
#define REG_V3              REG_R6
#define REG_V4              REG_R7
#define REG_V5              REG_R8
#define REG_V6              REG_R9
#define REG_V7              REG_R10
#define REG_SB              REG_R9
#define REG_SL              REG_R10
#define REG_FP              REG_R11
#define REG_IP              REG_R12
#define REG_SP              REG_R13
#define REG_LR              REG_R14
#define REG_PC              REG_R15

/****************************************************************************
 * Public Types
 ****************************************************************************/

#ifndef __ASSEMBLY__

/* This struct defines the way the registers are stored.  We need to save:
 *
 *  1  CPSR
 *  7  Static registers, v1-v7 (aka r4-r10)
 *  1  Frame pointer, fp (aka r11)
 *  1  Stack pointer, sp (aka r13)
 *  1  Return address, lr (aka r14)
 * ---
 * 11  (XCPTCONTEXT_USER_REG)
 *
 * On interrupts, we also need to save:
 *  4  Volatile registers, a1-a4 (aka r0-r3)
 *  1  Scratch Register, ip (aka r12)
 *---
 *  5  (XCPTCONTEXT_IRQ_REGS)
 *
 * For a total of 17 (XCPTCONTEXT_REGS)
 */
struct xcptcontext
{
  /* The following function pointer is non-zero if there are pending signals
   * to be processed.
   */

#ifndef CONFIG_DISABLE_SIGNALS
  void *sigdeliver; /* Actual type is sig_deliver_t */

  /* These are saved copies of LR and CPSR used during signal processing. */

  uint32_t saved_pc;
  uint32_t saved_cpsr;
#endif

  /* Register save area */

  uint32_t regs[XCPTCONTEXT_REGS];
};

#endif /* __ASSEMBLY__ */

/****************************************************************************
 * Inline functions
 ****************************************************************************/

#ifndef __ASSEMBLY__

/* Return the current IRQ state */

static inline irqstate_t irqstate(void)
{
  unsigned int cpsr;

  __asm__ __volatile__
    (
      "\tmrs    %0, cpsr\n"
      : "=r" (cpsr)
      :
      : "memory"
    );

  return cpsr;
}

/* Disable IRQs and return the previous IRQ state */

static inline irqstate_t irqsave(void)
{
  unsigned int cpsr;

  __asm__ __volatile__
    (
      "\tmrs    %0, cpsr\n"
      "\tcpsid  i\n"
      : "=r" (cpsr)
      :
      : "memory"
    );

  return cpsr;
}

/* Enable IRQs and return the previous IRQ state */

static inline irqstate_t irqenable(void)
{
  unsigned int cpsr;

  __asm__ __volatile__
    (
      "\tmrs    %0, cpsr\n"
      "\tcpsie  i\n"
      : "=r" (cpsr)
      :
      : "memory"
    );

  return cpsr;
}

/* Restore saved IRQ state */

static inline void irqrestore(irqstate_t flags)
{
  __asm__ __volatile__
    (
      "msr    cpsr_c, %0"
      :
      : "r" (flags)
      : "memory"
    );
}

#endif /* __ASSEMBLY__ */

/****************************************************************************
 * Public Variables
 ****************************************************************************/

#ifndef __ASSEMBLY__
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#undef EXTERN
#ifdef __cplusplus
}
#endif
#endif

#endif /* __ARCH_ARM_INCLUDE_ARMV7_R_IRQ_H */
