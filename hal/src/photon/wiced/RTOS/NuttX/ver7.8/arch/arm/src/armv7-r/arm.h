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
 * arch/arm/src/armv7-r/arm.h
 * Non-CP15 Registers
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * References:
 *
 *  "Cortex-R4 and Cortex-R4F. Technical Reference Manual", Revision: r1p3,
 *   Copyright � 2009 ARM Limited. All rights reserved. ARM DDI 0363E (ID013010)
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

#ifndef __ARCH_ARM_SRC_ARMV7_R_H
#define __ARCH_ARM_SRC_ARMV7_R_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* ARMv7-R ******************************************************************/

/* PSR bits */

#define PSR_MODE_SHIFT    (0)       /* Bits 0-4: Mode fields */
#define PSR_MODE_MASK     (31 << PSR_MODE_SHIFT)
#  define PSR_MODE_USR    (16 << PSR_MODE_SHIFT) /* User mode */
#  define PSR_MODE_FIQ    (17 << PSR_MODE_SHIFT) /* FIQ mode */
#  define PSR_MODE_IRQ    (18 << PSR_MODE_SHIFT) /* IRQ mode */
#  define PSR_MODE_SVC    (19 << PSR_MODE_SHIFT) /* Supervisor mode */
#  define PSR_MODE_MON    (22 << PSR_MODE_SHIFT) /* Monitor mode */
#  define PSR_MODE_ABT    (23 << PSR_MODE_SHIFT) /* Abort mode */
#  define PSR_MODE_HYP    (26 << PSR_MODE_SHIFT) /* Hyp mode */
#  define PSR_MODE_UND    (27 << PSR_MODE_SHIFT) /* Undefined mode */
#  define PSR_MODE_SYS    (31 << PSR_MODE_SHIFT) /* System mode */
#define PSR_T_SHIFT       (5)  /* Bit 5: Thumb execution state bit shift */
#define PSR_T_BIT         (1 << PSR_T_SHIFT)  /* Bit 5: Thumb execution state bit */
#define PSR_MASK_SHIFT    (6)       /* Bits 6-8: Mask Bits */
#define PSR_MASK_MASK     (7 << PSR_GE_SHIFT)
#  define PSR_F_BIT       (1 << 6)  /* Bit 6: FIQ mask bit */
#  define PSR_I_BIT       (1 << 7)  /* Bit 7: IRQ mask bit */
#  define PSR_A_BIT       (1 << 8)  /* Bit 8: Asynchronous abort mask */
#define PSR_E_BIT         (1 << 9)  /* Bit 9:  Endianness execution state bit */
#define PSR_GE_SHIFT      (16)      /* Bits 16-19: Greater than or Equal flags */
#define PSR_GE_MASK       (15 << PSR_GE_SHIFT)
                                    /* Bits 20-23: Reserved. RAZ/SBZP */
#define PSR_J_BIT         (1 << 24) /* Bit 24: Jazelle state bit */
#define PSR_IT01_SHIFT    (25)      /* Bits 25-26:  If-Then execution state bits IT[0:1] */
#define PSR_IT01_MASK     (3 << PSR_IT01_SHIFT)
#define PSR_Q_BIT         (1 << 27) /* Bit 27: Cumulative saturation bit */
#define PSR_V_BIT         (1 << 28) /* Bit 28: Overflow condition flag */
#define PSR_C_BIT         (1 << 29) /* Bit 29: Carry condition flag */
#define PSR_Z_BIT         (1 << 30) /* Bit 30: Zero condition flag */
#define PSR_N_BIT         (1 << 31) /* Bit 31: Negative condition flag */

/****************************************************************************
 * Inline Functions
 ****************************************************************************/

#ifndef __ASSEMBLY__

#endif /* __ASSEMBLY__ */

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifndef __ASSEMBLY__
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif
#endif /* __ASSEMBLY__ */

#endif  /* __ARCH_ARM_SRC_ARMV7_R_H */
