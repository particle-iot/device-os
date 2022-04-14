/************************************************************************************
 * arch/arm/src/lpc17xx/lpc176x_gpio.h
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
 ************************************************************************************/

#ifndef __ARCH_ARM_SRC_LPC17XX_LPC176X_GPIO_H
#define __ARCH_ARM_SRC_LPC17XX_LPC176X_GPIO_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

/************************************************************************************
 * Pre-processor Definitions
 ************************************************************************************/
/* Bit-encoded input to lpc17_configgpio() ******************************************/

/* Encoding: FFFx MMOV PPPN NNNN
 *
 *   Pin Function:   FFF
 *   Pin Mode bits:  MM
 *   Open drain:     O (output pins)
 *   Initial value:  V (output pins)
 *   Port number:    PPP (0-4)
 *   Pin number:     NNNNN (0-31)
 */

/* Pin Function bits: FFF
 * Only meaningful when the GPIO function is GPIO_PIN
 */

#define GPIO_FUNC_SHIFT        (13)       /* Bits 13-15: GPIO mode */
#define GPIO_FUNC_MASK         (7 << GPIO_FUNC_SHIFT)
#  define GPIO_INPUT           (0 << GPIO_FUNC_SHIFT) /* 000 GPIO input pin */
#  define GPIO_INTFE           (1 << GPIO_FUNC_SHIFT) /* 001 GPIO interrupt falling edge */
#  define GPIO_INTRE           (2 << GPIO_FUNC_SHIFT) /* 010 GPIO interrupt rising edge */
#  define GPIO_INTBOTH         (3 << GPIO_FUNC_SHIFT) /* 011 GPIO interrupt both edges */
#  define GPIO_OUTPUT          (4 << GPIO_FUNC_SHIFT) /* 100 GPIO outpout pin */
#  define GPIO_ALT1            (5 << GPIO_FUNC_SHIFT) /* 101 Alternate function 1 */
#  define GPIO_ALT2            (6 << GPIO_FUNC_SHIFT) /* 110 Alternate function 2 */
#  define GPIO_ALT3            (7 << GPIO_FUNC_SHIFT) /* 111 Alternate function 3 */

#define GPIO_EDGE_SHIFT        (13)       /* Bits 13-14: Interrupt edge bits */
#define GPIO_EDGE_MASK         (3 << GPIO_EDGE_SHIFT)

#define GPIO_INOUT_MASK        GPIO_OUTPUT
#define GPIO_FE_MASK           GPIO_INTFE
#define GPIO_RE_MASK           GPIO_INTRE

#define GPIO_ISGPIO(ps)        ((uint16_t(ps) & GPIO_FUNC_MASK) <= GPIO_OUTPUT)
#define GPIO_ISALT(ps)         ((uint16_t(ps) & GPIO_FUNC_MASK) > GPIO_OUTPUT)
#define GPIO_ISINPUT(ps)       (((ps) & GPIO_FUNC_MASK) == GPIO_INPUT)
#define GPIO_ISOUTPUT(ps)      (((ps) & GPIO_FUNC_MASK) == GPIO_OUTPUT)
#define GPIO_ISINORINT(ps)     (((ps) & GPIO_INOUT_MASK) == 0)
#define GPIO_ISOUTORALT(ps)    (((ps) & GPIO_INOUT_MASK) != 0)
#define GPIO_ISINTERRUPT(ps)   (GPIO_ISOUTPUT(ps) && !GPIO_ISINPUT(ps))
#define GPIO_ISFE(ps)          (((ps) & GPIO_FE_MASK) != 0)
#define GPIO_ISRE(ps)          (((ps) & GPIO_RE_MASK) != 0)

/* Pin Mode: MM */

#define GPIO_PUMODE_SHIFT      (10)      /* Bits 10-11: Pin pull-up mode */
#define GPIO_PUMODE_MASK       (3 << GPIO_PUMODE_SHIFT)
#  define GPIO_PULLUP          (0 << GPIO_PUMODE_SHIFT) /* Pull-up resistor enabled */
#  define GPIO_REPEATER        (1 << GPIO_PUMODE_SHIFT) /* Repeater mode enabled */
#  define GPIO_FLOAT           (2 << GPIO_PUMODE_SHIFT) /* Neither pull-up nor -down */
#  define GPIO_PULLDN          (3 << GPIO_PUMODE_SHIFT) /* Pull-down resistor enabled */

/* Open drain: O */

#define GPIO_OPEN_DRAIN        (1 << 9)  /* Bit 9:  Open drain mode */

/* Initial value: V */

#define GPIO_VALUE             (1 << 8)  /* Bit 8:  Initial GPIO output value */
#define GPIO_VALUE_ONE         GPIO_VALUE
#define GPIO_VALUE_ZERO        (0)

/* Port number:    PPP (0-4) */

#define GPIO_PORT_SHIFT        (5)         /* Bit 5-7:  Port number */
#define GPIO_PORT_MASK         (7 << GPIO_PORT_SHIFT)
#  define GPIO_PORT0           (0 << GPIO_PORT_SHIFT)
#  define GPIO_PORT1           (1 << GPIO_PORT_SHIFT)
#  define GPIO_PORT2           (2 << GPIO_PORT_SHIFT)
#  define GPIO_PORT3           (3 << GPIO_PORT_SHIFT)
#  define GPIO_PORT4           (4 << GPIO_PORT_SHIFT)

#define GPIO_NPORTS            5

/* Pin number:     NNNNN (0-31) */

#define GPIO_PIN_SHIFT         0        /* Bits 0-4: GPIO number: 0-31 */
#define GPIO_PIN_MASK          (31 << GPIO_PIN_SHIFT)
#  define GPIO_PIN0            (0  << GPIO_PIN_SHIFT)
#  define GPIO_PIN1            (1  << GPIO_PIN_SHIFT)
#  define GPIO_PIN2            (2  << GPIO_PIN_SHIFT)
#  define GPIO_PIN3            (3  << GPIO_PIN_SHIFT)
#  define GPIO_PIN4            (4  << GPIO_PIN_SHIFT)
#  define GPIO_PIN5            (5  << GPIO_PIN_SHIFT)
#  define GPIO_PIN6            (6  << GPIO_PIN_SHIFT)
#  define GPIO_PIN7            (7  << GPIO_PIN_SHIFT)
#  define GPIO_PIN8            (8  << GPIO_PIN_SHIFT)
#  define GPIO_PIN9            (9  << GPIO_PIN_SHIFT)
#  define GPIO_PIN10           (10 << GPIO_PIN_SHIFT)
#  define GPIO_PIN11           (11 << GPIO_PIN_SHIFT)
#  define GPIO_PIN12           (12 << GPIO_PIN_SHIFT)
#  define GPIO_PIN13           (13 << GPIO_PIN_SHIFT)
#  define GPIO_PIN14           (14 << GPIO_PIN_SHIFT)
#  define GPIO_PIN15           (15 << GPIO_PIN_SHIFT)
#  define GPIO_PIN16           (16 << GPIO_PIN_SHIFT)
#  define GPIO_PIN17           (17 << GPIO_PIN_SHIFT)
#  define GPIO_PIN18           (18 << GPIO_PIN_SHIFT)
#  define GPIO_PIN19           (19 << GPIO_PIN_SHIFT)
#  define GPIO_PIN20           (20 << GPIO_PIN_SHIFT)
#  define GPIO_PIN21           (21 << GPIO_PIN_SHIFT)
#  define GPIO_PIN22           (22 << GPIO_PIN_SHIFT)
#  define GPIO_PIN23           (23 << GPIO_PIN_SHIFT)
#  define GPIO_PIN24           (24 << GPIO_PIN_SHIFT)
#  define GPIO_PIN25           (25 << GPIO_PIN_SHIFT)
#  define GPIO_PIN26           (26 << GPIO_PIN_SHIFT)
#  define GPIO_PIN27           (27 << GPIO_PIN_SHIFT)
#  define GPIO_PIN28           (28 << GPIO_PIN_SHIFT)
#  define GPIO_PIN29           (29 << GPIO_PIN_SHIFT)
#  define GPIO_PIN30           (30 << GPIO_PIN_SHIFT)
#  define GPIO_PIN31           (31 << GPIO_PIN_SHIFT)

/************************************************************************************
 * Public Types
 ************************************************************************************/

typedef uint16_t lpc17_pinset_t;

/************************************************************************************
 * Public Data
 ************************************************************************************/

#ifndef __ASSEMBLY__
#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/* These tables have global scope only because they are shared between lpc17_gpio.c,
 * lpc17_gpioint.c, and lpc17_gpiodbg.c
 */

EXTERN const uint32_t g_lopinsel[GPIO_NPORTS];
EXTERN const uint32_t g_hipinsel[GPIO_NPORTS];
EXTERN const uint32_t g_lopinmode[GPIO_NPORTS];
EXTERN const uint32_t g_hipinmode[GPIO_NPORTS];
EXTERN const uint32_t g_odmode[GPIO_NPORTS];

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef __cplusplus
}
#endif
#endif /* __ASSEMBLY__ */

#endif /* __ARCH_ARM_SRC_LPC17XX_LPC176X_GPIO_H */
