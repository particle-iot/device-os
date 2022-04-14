/************************************************************************************
 * arch/arm/include/efm32s/irq.h
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
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
 ************************************************************************************/

/* This file should never be included directed but, rather, only indirectly through
 * nuttx/irq.h
 */

#ifndef __ARCH_ARM_INCLUDE_EFM32_IRQ_H
#define __ARCH_ARM_INCLUDE_EFM32_IRQ_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>

#include <nuttx/irq.h>
#include <arch/efm32/chip.h>

/************************************************************************************
 * Pre-processor Definitions
 ************************************************************************************/

/* IRQ numbers.  The IRQ number corresponds vector number and hence map directly to
 * bits in the NVIC.  This does, however, waste several words of memory in the IRQ
 * to handle mapping tables.
 */

/* Processor Exceptions (vectors 0-15) */

#define EFM32_IRQ_RESERVED     (0) /* Reserved vector (only used with CONFIG_DEBUG) */
                                   /* Vector  0: Reset stack pointer value */
                                   /* Vector  1: Reset (not handler as an IRQ) */
#define EFM32_IRQ_NMI          (2) /* Vector  2: Non-Maskable Interrupt (NMI) */
#define EFM32_IRQ_HARDFAULT    (3) /* Vector  3: Hard fault */
#define EFM32_IRQ_MEMFAULT     (4) /* Vector  4: Memory management (MPU) */
#define EFM32_IRQ_BUSFAULT     (5) /* Vector  5: Bus fault */
#define EFM32_IRQ_USAGEFAULT   (6) /* Vector  6: Usage fault */
#define EFM32_IRQ_SVCALL      (11) /* Vector 11: SVC call */
#define EFM32_IRQ_DBGMONITOR  (12) /* Vector 12: Debug Monitor */
                                   /* Vector 13: Reserved */
#define EFM32_IRQ_PENDSV      (14) /* Vector 14: Pendable system service request */
#define EFM32_IRQ_SYSTICK     (15) /* Vector 15: System tick */

/* External interrupts (vectors >= 16).  These definitions are chip-specific */

#define EFM32_IRQ_INTERRUPTS  (16) /* Vector number of the first external interrupt */

#if defined(CONFIG_EFM32_EFM32TG)
#  include <arch/efm32/efm32tg_irq.h>
#elif defined(CONFIG_EFM32_EFM32G)
#  include <arch/efm32/efm32g_irq.h>
#elif defined(CONFIG_EFM32_EFM32GG)
#  include <arch/efm32/efm32gg_irq.h>
#else
#  error "Unsupported EFM32 chip"
#endif

#ifdef CONFIG_EFM32_GPIO_IRQ
/* If GPIO interrupt support is enabled then up to 16 additional GPIO interrupt
 * sources are available.  There are actually only two physical interrupt lines:
 * GPIO_EVEN and GPIO_ODD.  However, from the software point of view, there are
 * 16-additional interrupts generated from a second level of decoding.
 */

#  define EFM32_IRQ_EXTI0     (NR_VECTORS+0)  /* Port[n], pin0 external interrupt */
#  define EFM32_IRQ_EXTI1     (NR_VECTORS+1)  /* Port[n], pin1 external interrupt */
#  define EFM32_IRQ_EXTI2     (NR_VECTORS+2)  /* Port[n], pin2 external interrupt */
#  define EFM32_IRQ_EXTI3     (NR_VECTORS+3)  /* Port[n], pin3 external interrupt */
#  define EFM32_IRQ_EXTI4     (NR_VECTORS+4)  /* Port[n], pin4 external interrupt */
#  define EFM32_IRQ_EXTI5     (NR_VECTORS+5)  /* Port[n], pin5 external interrupt */
#  define EFM32_IRQ_EXTI6     (NR_VECTORS+6)  /* Port[n], pin6 external interrupt */
#  define EFM32_IRQ_EXTI7     (NR_VECTORS+7)  /* Port[n], pin7 external interrupt */
#  define EFM32_IRQ_EXTI8     (NR_VECTORS+8)  /* Port[n], pin8 external interrupt */
#  define EFM32_IRQ_EXTI9     (NR_VECTORS+9)  /* Port[n], pin9 external interrupt */
#  define EFM32_IRQ_EXTI10    (NR_VECTORS+10) /* Port[n], pin10 external interrupt */
#  define EFM32_IRQ_EXTI11    (NR_VECTORS+11) /* Port[n], pin11 external interrupt */
#  define EFM32_IRQ_EXTI12    (NR_VECTORS+12) /* Port[n], pin12 external interrupt */
#  define EFM32_IRQ_EXTI13    (NR_VECTORS+13) /* Port[n], pin13 external interrupt */
#  define EFM32_IRQ_EXTI14    (NR_VECTORS+14) /* Port[n], pin14 external interrupt */
#  define EFM32_IRQ_EXTI15    (NR_VECTORS+15) /* Port[n], pin15 external interrupt */

#  define NR_IRQS             (NR_VECTORS+16) /* Total number of interrupts */
#else
#  define NR_IRQS             NR_VECTORS      /* Total number of interrupts */
#endif

/************************************************************************************
 * Public Types
 ************************************************************************************/

/************************************************************************************
 * Public Data
 ************************************************************************************/

#ifndef __ASSEMBLY__
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/************************************************************************************
 * Public Functions
 ************************************************************************************/

#undef EXTERN
#ifdef __cplusplus
}
#endif
#endif

#endif /* __ARCH_ARM_INCLUDE_EFM32_IRQ_H */
