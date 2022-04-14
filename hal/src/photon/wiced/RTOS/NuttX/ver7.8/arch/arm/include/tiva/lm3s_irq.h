/************************************************************************************
 * arch/arm/include/tiva/lm3s_irq.h
 *
 *   Copyright (C) 2013 Gregory Nutt. All rights reserved.
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

#ifndef __ARCH_ARM_INCLUDE_TIVA_LM3S_IRQ_H
#define __ARCH_ARM_INCLUDE_TIVA_LM3S_IRQ_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <nuttx/config.h>

/************************************************************************************
 * Definitions
 ************************************************************************************/

/* IRQ numbers.  The IRQ number corresponds vector number and hence map directly to
 * bits in the NVIC.  This does, however, waste several words of memory in the IRQ
 * to handle mapping tables.
 */

/* External interrupts (vectors >= 16) */

#define TIVA_IRQ_INTERRUPTS   (16) /* Vector number of the first external interrupt */
#if defined(CONFIG_ARCH_CHIP_LM3S6918)

#  define TIVA_IRQ_GPIOA      (16) /* Vector 16: GPIO Port A */
#  define TIVA_IRQ_GPIOB      (17) /* Vector 17: GPIO Port B */
#  define TIVA_IRQ_GPIOC      (18) /* Vector 18: GPIO Port C */
#  define TIVA_IRQ_GPIOD      (19) /* Vector 19: GPIO Port D */

#  define TIVA_IRQ_GPIOE      (20) /* Vector 20: GPIO Port E */
#  define TIVA_IRQ_UART0      (21) /* Vector 21: UART 0 */
#  define TIVA_IRQ_UART1      (22) /* Vector 22: UART 1 */
#  define TIVA_IRQ_SSI0       (23) /* Vector 23: SSI 0 */
#  define TIVA_IRQ_I2C0       (24) /* Vector 24: I2C 0 */
#  define TIVA_RESERVED_25    (25) /* Vector 25: Reserved */
#  define TIVA_RESERVED_26    (26) /* Vector 26: Reserved */
#  define TIVA_RESERVED_27    (27) /* Vector 27: Reserved */
#  define TIVA_RESERVED_28    (28) /* Vector 28: Reserved */
#  define TIVA_RESERVED_29    (29) /* Vector 29: Reserved */

#  define TIVA_IRQ_ADC0       (30) /* Vector 30: ADC Sequence 0 */
#  define TIVA_IRQ_ADC1       (31) /* Vector 31: ADC Sequence 1 */
#  define TIVA_IRQ_ADC2       (32) /* Vector 32: ADC Sequence 2 */
#  define TIVA_IRQ_ADC3       (33) /* Vector 33: ADC Sequence 3 */
#  define TIVA_IRQ_WDOG       (34) /* Vector 34: Watchdog Timer */
#  define TIVA_IRQ_TIMER0A    (35) /* Vector 35: Timer 0 A */
#  define TIVA_IRQ_TIMER0B    (36) /* Vector 36: Timer 0 B */
#  define TIVA_IRQ_TIMER1A    (37) /* Vector 37: Timer 1 A */
#  define TIVA_IRQ_TIMER1B    (38) /* Vector 38: Timer 1 B */
#  define TIVA_IRQ_TIMER2A    (39) /* Vector 39: Timer 2 A */

#  define TIVA_IRQ_TIMER2B    (40) /* Vector 40: Timer 2 B */
#  define TIVA_IRQ_COMPARE0   (41) /* Vector 41: Analog Comparator 0 */
#  define TIVA_IRQ_COMPARE1   (42) /* Vector 42: Analog Comparator 1 */
#  define TIVA_RESERVED_43    (43) /* Vector 43: Reserved */
#  define TIVA_IRQ_SYSCON     (44) /* Vector 44: System Control */
#  define TIVA_IRQ_FLASHCON   (45) /* Vector 45: FLASH Control */
#  define TIVA_IRQ_GPIOF      (46) /* Vector 46: GPIO Port F */
#  define TIVA_IRQ_GPIOG      (47) /* Vector 47: GPIO Port G */
#  define TIVA_IRQ_GPIOH      (48) /* Vector 48: GPIO Port H */
#  define TIVA_RESERVED_49    (49) /* Vector 49: Reserved */

#  define TIVA_IRQ_SSI1       (50) /* Vector 50: SSI 1 */
#  define TIVA_IRQ_TIMER3A    (51) /* Vector 51: Timer 3 A */
#  define TIVA_IRQ_TIMER3B    (52) /* Vector 52: Timer 3 B */
#  define TIVA_IRQ_I2C1       (53) /* Vector 53: I2C 1 */
#  define TIVA_RESERVED_54    (54) /* Vector 54: Reserved */
#  define TIVA_RESERVED_55    (55) /* Vector 55: Reserved */
#  define TIVA_RESERVED_56    (56) /* Vector 56: Reserved */
#  define TIVA_RESERVED_57    (57) /* Vector 57: Reserved */
#  define TIVA_IRQ_ETHCON     (58) /* Vector 58: Ethernet Controller */
#  define TIVA_IRQ_HIBERNATE  (59) /* Vector 59: Hibernation Module */

#  define TIVA_RESERVED_60    (60) /* Vector 60: Reserved */
#  define TIVA_RESERVED_61    (61) /* Vector 61: Reserved */
#  define TIVA_RESERVED_62    (62) /* Vector 62: Reserved */
#  define TIVA_RESERVED_63    (63) /* Vector 63: Reserved */
#  define TIVA_RESERVED_64    (64) /* Vector 64: Reserved */
#  define TIVA_RESERVED_65    (65) /* Vector 65: Reserved */
#  define TIVA_RESERVED_66    (66) /* Vector 66: Reserved */
#  define TIVA_RESERVED_67    (67) /* Vector 67: Reserved */
#  define TIVA_RESERVED_68    (68) /* Vector 68: Reserved */
#  define TIVA_RESERVED_69    (69) /* Vector 69: Reserved */

#  define TIVA_RESERVED_70    (70) /* Vector 70: Reserved */

#  define NR_IRQS             (71) /* (Really less because of reserved vectors) */

#elif defined(CONFIG_ARCH_CHIP_LM3S6432)
#  define TIVA_IRQ_GPIOA      (16) /* Vector 16: GPIO Port A */
#  define TIVA_IRQ_GPIOB      (17) /* Vector 17: GPIO Port B */
#  define TIVA_IRQ_GPIOC      (18) /* Vector 18: GPIO Port C */
#  define TIVA_IRQ_GPIOD      (19) /* Vector 19: GPIO Port D */

#  define TIVA_IRQ_GPIOE      (20) /* Vector 20: GPIO Port E */
#  define TIVA_IRQ_UART0      (21) /* Vector 21: UART 0 */
#  define TIVA_IRQ_UART1      (22) /* Vector 22: UART 1 */
#  define TIVA_IRQ_SSI0       (23) /* Vector 23: SSI 0 */
#  define TIVA_IRQ_I2C0       (24) /* Vector 24: I2C 0 */
#  define TIVA_RESERVED_25    (25) /* Vector 25: Reserved */
#  define TIVA_IRQ_PWM0       (26) /* Vector 26: PWM Generator 0 */
#  define TIVA_RESERVED_27    (27) /* Vector 27: Reserved */
#  define TIVA_RESERVED_28    (28) /* Vector 28: Reserved */
#  define TIVA_RESERVED_29    (29) /* Vector 29: Reserved */

#  define TIVA_IRQ_ADC0       (30) /* Vector 30: ADC Sequence 0 */
#  define TIVA_IRQ_ADC1       (31) /* Vector 31: ADC Sequence 1 */
#  define TIVA_IRQ_ADC2       (32) /* Vector 32: ADC Sequence 2 */
#  define TIVA_IRQ_ADC3       (33) /* Vector 33: ADC Sequence 3 */
#  define TIVA_IRQ_WDOG       (34) /* Vector 34: Watchdog Timer */
#  define TIVA_IRQ_TIMER0A    (35) /* Vector 35: Timer 0 A */
#  define TIVA_IRQ_TIMER0B    (36) /* Vector 36: Timer 0 B */
#  define TIVA_IRQ_TIMER1A    (37) /* Vector 37: Timer 1 A */
#  define TIVA_IRQ_TIMER1B    (38) /* Vector 38: Timer 1 B */
#  define TIVA_IRQ_TIMER2A    (39) /* Vector 39: Timer 2 A */

#  define TIVA_IRQ_TIMER2B    (40) /* Vector 40: Timer 2 B */
#  define TIVA_IRQ_COMPARE0   (41) /* Vector 41: Analog Comparator 0 */
#  define TIVA_IRQ_COMPARE1   (42) /* Vector 42: Analog Comparator 1 */
#  define TIVA_RESERVED_43    (43) /* Vector 43: Reserved */
#  define TIVA_IRQ_SYSCON     (44) /* Vector 44: System Control */
#  define TIVA_IRQ_FLASHCON   (45) /* Vector 45: FLASH Control */
#  define TIVA_IRQ_GPIOF      (46) /* Vector 46: GPIO Port F */
#  define TIVA_IRQ_GPIOG      (47) /* Vector 47: GPIO Port G */
#  define TIVA_RESERVED_48    (48) /* Vector 48: Reserved */
#  define TIVA_RESERVED_49    (49) /* Vector 49: Reserved */

#  define TIVA_RESERVED_50    (50) /* Vector 50: Reserved */
#  define TIVA_RESERVED_51    (51) /* Vector 51: Reserved */
#  define TIVA_RESERVED_52    (52) /* Vector 52: Reserved */
#  define TIVA_RESERVED_53    (53) /* Vector 53: Reserved */
#  define TIVA_RESERVED_54    (54) /* Vector 54: Reserved */
#  define TIVA_RESERVED_55    (55) /* Vector 55: Reserved */
#  define TIVA_RESERVED_56    (56) /* Vector 56: Reserved */
#  define TIVA_RESERVED_57    (57) /* Vector 57: Reserved */
#  define TIVA_IRQ_ETHCON     (58) /* Vector 58: Ethernet Controller */
#  define TIVA_RESERVED_59    (59) /* Vector 59: Reserved */

#  define TIVA_RESERVED_60    (60) /* Vector 60: Reserved */
#  define TIVA_RESERVED_61    (61) /* Vector 61: Reserved */
#  define TIVA_RESERVED_62    (62) /* Vector 62: Reserved */
#  define TIVA_RESERVED_63    (63) /* Vector 63: Reserved */
#  define TIVA_RESERVED_64    (64) /* Vector 64: Reserved */
#  define TIVA_RESERVED_65    (65) /* Vector 65: Reserved */
#  define TIVA_RESERVED_66    (66) /* Vector 66: Reserved */
#  define TIVA_RESERVED_67    (67) /* Vector 67: Reserved */
#  define TIVA_RESERVED_68    (68) /* Vector 68: Reserved */
#  define TIVA_RESERVED_69    (69) /* Vector 69: Reserved */

#  define TIVA_RESERVED_70    (70) /* Vector 70: Reserved */

#  define NR_IRQS             (71) /* (Really less because of reserved vectors) */

#elif defined(CONFIG_ARCH_CHIP_LM3S6965)
#  define TIVA_IRQ_GPIOA      (16) /* Vector 16: GPIO Port A */
#  define TIVA_IRQ_GPIOB      (17) /* Vector 17: GPIO Port B */
#  define TIVA_IRQ_GPIOC      (18) /* Vector 18: GPIO Port C */
#  define TIVA_IRQ_GPIOD      (19) /* Vector 19: GPIO Port D */
#  define TIVA_IRQ_GPIOE      (20) /* Vector 20: GPIO Port E */

#  define TIVA_IRQ_UART0      (21) /* Vector 21: UART 0 */
#  define TIVA_IRQ_UART1      (22) /* Vector 22: UART 1 */
#  define TIVA_IRQ_SSI0       (23) /* Vector 23: SSI 0 */
#  define TIVA_IRQ_I2C0       (24) /* Vector 24: I2C 0 */
#  define TIVA_IRQ_PWMFAULT   (25) /* Vector 25: PWM Fault */
#  define TIVA_IRQ_PWM0       (26) /* Vector 26: PWM Generator 0 */
#  define TIVA_IRQ_PWM1       (27) /* Vector 27: PWM Generator 1 */
#  define TIVA_IRQ_PWM2       (28) /* Vector 28: PWM Generator 2 */
#  define TIVA_IRQ_QEI0       (29) /* Vector 29: QEI0 */

#  define TIVA_IRQ_ADC0       (30) /* Vector 30: ADC Sequence 0 */
#  define TIVA_IRQ_ADC1       (31) /* Vector 31: ADC Sequence 1 */
#  define TIVA_IRQ_ADC2       (32) /* Vector 32: ADC Sequence 2 */
#  define TIVA_IRQ_ADC3       (33) /* Vector 33: ADC Sequence 3 */
#  define TIVA_IRQ_WDOG       (34) /* Vector 34: Watchdog Timer */
#  define TIVA_IRQ_TIMER0A    (35) /* Vector 35: Timer 0 A */
#  define TIVA_IRQ_TIMER0B    (36) /* Vector 36: Timer 0 B */
#  define TIVA_IRQ_TIMER1A    (37) /* Vector 37: Timer 1 A */
#  define TIVA_IRQ_TIMER1B    (38) /* Vector 38: Timer 1 B */
#  define TIVA_IRQ_TIMER2A    (39) /* Vector 39: Timer 2 A */

#  define TIVA_IRQ_TIMER2B    (40) /* Vector 40: Timer 2 B */
#  define TIVA_IRQ_COMPARE0   (41) /* Vector 41: Analog Comparator 0 */
#  define TIVA_IRQ_COMPARE1   (42) /* Vector 42: Analog Comparator 1 */
#  define TIVA_RESERVED_43    (43) /* Vector 43: Reserved */
#  define TIVA_IRQ_SYSCON     (44) /* Vector 44: System Control */
#  define TIVA_IRQ_FLASHCON   (45) /* Vector 45: FLASH Control */
#  define TIVA_IRQ_GPIOF      (46) /* Vector 46: GPIO Port F */
#  define TIVA_IRQ_GPIOG      (47) /* Vector 47: GPIO Port G */
#  define TIVA_RESERVED_48    (48) /* Vector 48: Reserved */
#  define TIVA_IRQ_UART2      (49) /* Vector 49: UART 2 */

#  define TIVA_RESERVED_50    (50) /* Vector 50: Reserved */
#  define TIVA_IRQ_TIMER3A    (51) /* Vector 51: Timer 3 A */
#  define TIVA_IRQ_TIMER3B    (52) /* Vector 52: Timer 3 B */
#  define TIVA_IRQ_I2C1       (53) /* Vector 53: I2C 1 */
#  define TIVA_IRQ_QEI1       (54) /* Vector 54: QEI1 */
#  define TIVA_RESERVED_55    (55) /* Vector 55: Reserved */
#  define TIVA_RESERVED_56    (56) /* Vector 56: Reserved */
#  define TIVA_RESERVED_57    (57) /* Vector 57: Reserved */
#  define TIVA_IRQ_ETHCON     (58) /* Vector 58: Ethernet Controller */
#  define TIVA_IRQ_HIBERNATE  (59) /* Vector 59: Hibernation Module */

#  define TIVA_RESERVED_60    (60) /* Vector 60: Reserved */
#  define TIVA_RESERVED_61    (61) /* Vector 61: Reserved */
#  define TIVA_RESERVED_62    (62) /* Vector 62: Reserved */
#  define TIVA_RESERVED_63    (63) /* Vector 63: Reserved */
#  define TIVA_RESERVED_64    (64) /* Vector 64: Reserved */
#  define TIVA_RESERVED_65    (65) /* Vector 65: Reserved */
#  define TIVA_RESERVED_66    (66) /* Vector 66: Reserved */
#  define TIVA_RESERVED_67    (67) /* Vector 67: Reserved */
#  define TIVA_RESERVED_68    (68) /* Vector 68: Reserved */
#  define TIVA_RESERVED_69    (69) /* Vector 69: Reserved */

#  define TIVA_RESERVED_70    (70) /* Vector 70: Reserved */

#  define NR_IRQS             (71) /* (Really less because of reserved vectors) */

#elif defined(CONFIG_ARCH_CHIP_LM3S9B96)
#  define TIVA_IRQ_GPIOA      (16) /* Vector 16: GPIO Port A */
#  define TIVA_IRQ_GPIOB      (17) /* Vector 17: GPIO Port B */
#  define TIVA_IRQ_GPIOC      (18) /* Vector 18: GPIO Port C */
#  define TIVA_IRQ_GPIOD      (19) /* Vector 19: GPIO Port D */

#  define TIVA_IRQ_GPIOE      (20) /* Vector 20: GPIO Port E */
#  define TIVA_IRQ_UART0      (21) /* Vector 21: UART 0 */
#  define TIVA_IRQ_UART1      (22) /* Vector 22: UART 1 */
#  define TIVA_IRQ_SSI0       (23) /* Vector 23: SSI 0 */
#  define TIVA_IRQ_I2C0       (24) /* Vector 24: I2C 0 */
#  define TIVA_IRQ_PWMFAULT   (25) /* Vector 25: PWM Fault */
#  define TIVA_IRQ_PWM0       (26) /* Vector 26: PWM Generator 0 */
#  define TIVA_IRQ_PWM1       (27) /* Vector 27: PWM Generator 1 */
#  define TIVA_IRQ_PWM2       (28) /* Vector 28: PWM Generator 2 */
#  define TIVA_IRQ_QEI0       (29) /* Vector 29: QEI0 */

#  define TIVA_IRQ_ADC0       (30) /* Vector 30: ADC0 Sequence 0 */
#  define TIVA_IRQ_ADC1       (31) /* Vector 31: ADC0 Sequence 1 */
#  define TIVA_IRQ_ADC2       (32) /* Vector 32: ADC0 Sequence 2 */
#  define TIVA_IRQ_ADC3       (33) /* Vector 33: ADC0 Sequence 3 */
#  define TIVA_IRQ_WDOG       (34) /* Vector 34: Watchdog Timer */
#  define TIVA_IRQ_TIMER0A    (35) /* Vector 35: Timer 0 A */
#  define TIVA_IRQ_TIMER0B    (36) /* Vector 36: Timer 0 B */
#  define TIVA_IRQ_TIMER1A    (37) /* Vector 37: Timer 1 A */
#  define TIVA_IRQ_TIMER1B    (38) /* Vector 38: Timer 1 B */
#  define TIVA_IRQ_TIMER2A    (39) /* Vector 39: Timer 2 A */

#  define TIVA_IRQ_TIMER2B    (40) /* Vector 40: Timer 2 B */
#  define TIVA_IRQ_COMPARE0   (41) /* Vector 41: Analog Comparator 0 */
#  define TIVA_IRQ_COMPARE1   (42) /* Vector 42: Analog Comparator 1 */
#  define TIVA_IRQ_COMPARE2   (43) /* Vector 43: Analog Comparator 3 */
#  define TIVA_IRQ_SYSCON     (44) /* Vector 44: System Control */
#  define TIVA_IRQ_FLASHCON   (45) /* Vector 45: FLASH Control */
#  define TIVA_IRQ_GPIOF      (46) /* Vector 46: GPIO Port F */
#  define TIVA_IRQ_GPIOG      (47) /* Vector 47: GPIO Port G */
#  define TIVA_IRQ_GPIOH      (48) /* Vector 48: GPIO Port H */
#  define TIVA_IRQ_UART2      (49) /* Vector 49: UART 2 */

#  define TIVA_IRQ_SSI1       (50) /* Vector 50: SSI 1  */
#  define TIVA_IRQ_TIMER3A    (51) /* Vector 51: Timer 3 A */
#  define TIVA_IRQ_TIMER3B    (52) /* Vector 52: Timer 3 B */
#  define TIVA_IRQ_I2C1       (53) /* Vector 53: I2C 1 */
#  define TIVA_IRQ_QEI1       (54) /* Vector 54: QEI1 */
#  define TIVA_IRQ_CAN0       (55) /* Vector 55: CAN 1 */
#  define TIVA_IRQ_CAN1       (56) /* Vector 56: CAN 2 */
#  define TIVA_RESERVED_57    (57) /* Vector 57: Reserved */
#  define TIVA_IRQ_ETHCON     (58) /* Vector 58: Ethernet Controller */
#  define TIVA_RESERVED_59    (59) /* Vector 59: Reserved */

#  define TIVA_IRQ_USB        (60) /* Vector 60: USB */
#  define TIVA_IRQ_PWM3       (61) /* Vector 61: PWM Generator 3 */
#  define TIVA_IRQ_UDMASOFT   (62) /* Vector 62: uDMA Software */
#  define TIVA_IRQ_UDMAERROR  (63) /* Vector 63: uDMA Error */
#  define TIVA_IRQ_ADC1_0     (64) /* Vector 64: ADC1 Sequence 0 */
#  define TIVA_IRQ_ADC1_1     (65) /* Vector 65: ADC1 Sequence 1 */
#  define TIVA_IRQ_ADC1_2     (66) /* Vector 66: ADC1 Sequence 2 */
#  define TIVA_IRQ_ADC1_3     (67) /* Vector 67: ADC1 Sequence 3 */
#  define TIVA_IRQ_I2S0       (68) /* Vector 68: I2S0 */
#  define TIVA_IRQ_EPI        (69) /* Vector 69: EPI */

#  define TIVA_IRQ_GPIOJ      (70) /* Vector 70: GPIO Port J */
#  define TIVA_RESERVED_71    (71) /* Vector 71: Reserved */

#  define NR_IRQS             (72) /* (Really less because of reserved vectors) */

#elif defined(CONFIG_ARCH_CHIP_LM3S8962)
#  define TIVA_IRQ_GPIOA      (16) /* Vector 16: GPIO Port A */
#  define TIVA_IRQ_GPIOB      (17) /* Vector 17: GPIO Port B */
#  define TIVA_IRQ_GPIOC      (18) /* Vector 18: GPIO Port C */
#  define TIVA_IRQ_GPIOD      (19) /* Vector 19: GPIO Port D */

#  define TIVA_IRQ_GPIOE      (20) /* Vector 20: GPIO Port E */
#  define TIVA_IRQ_UART0      (21) /* Vector 21: UART 0 */
#  define TIVA_IRQ_UART1      (22) /* Vector 22: UART 1 */
#  define TIVA_IRQ_SSI0       (23) /* Vector 23: SSI 0 */
#  define TIVA_IRQ_I2C0       (24) /* Vector 24: I2C 0 */
#  define TIVA_IRQ_PWMFAULT   (25) /* Vector 25: PWM Fault */
#  define TIVA_IRQ_PWM0       (26) /* Vector 26: PWM Generator 0 */
#  define TIVA_IRQ_PWM1       (27) /* Vector 27: PWM Generator 1 */
#  define TIVA_IRQ_PWM2       (28) /* Vector 28: PWM Generator 2 */
#  define TIVA_IRQ_QEI0       (29) /* Vector 29: QEI0 */

#  define TIVA_IRQ_ADC0       (30) /* Vector 30: ADC Sequence 0 */
#  define TIVA_IRQ_ADC1       (31) /* Vector 31: ADC Sequence 1 */
#  define TIVA_IRQ_ADC2       (32) /* Vector 32: ADC Sequence 2 */
#  define TIVA_IRQ_ADC3       (33) /* Vector 33: ADC Sequence 3 */
#  define TIVA_IRQ_WDOG       (34) /* Vector 34: Watchdog Timer */
#  define TIVA_IRQ_TIMER0A    (35) /* Vector 35: Timer 0 A */
#  define TIVA_IRQ_TIMER0B    (36) /* Vector 36: Timer 0 B */
#  define TIVA_IRQ_TIMER1A    (37) /* Vector 37: Timer 1 A */
#  define TIVA_IRQ_TIMER1B    (38) /* Vector 38: Timer 1 B */
#  define TIVA_IRQ_TIMER2A    (39) /* Vector 39: Timer 2 A */

#  define TIVA_IRQ_TIMER2B    (40) /* Vector 40: Timer 2 B */
#  define TIVA_IRQ_COMPARE0   (41) /* Vector 41: Analog Comparator 0 */
#  define TIVA_RESERVED_42    (42) /* Vector 42: Reserved */
#  define TIVA_RESERVED_43    (43) /* Vector 43: Reserved */
#  define TIVA_IRQ_SYSCON     (44) /* Vector 44: System Control */
#  define TIVA_IRQ_FLASHCON   (45) /* Vector 45: FLASH Control */
#  define TIVA_IRQ_GPIOF      (46) /* Vector 46: GPIO Port F */
#  define TIVA_IRQ_GPIOG      (47) /* Vector 47: GPIO Port G */
#  define TIVA_RESERVED_48    (48) /* Vector 48: Reserved */
#  define TIVA_RESERVED_49    (49) /* Vector 49: Reserved */

#  define TIVA_RESERVED_50    (50) /* Vector 50: Reserved */
#  define TIVA_IRQ_TIMER3A    (51) /* Vector 51: Timer 3 A */
#  define TIVA_IRQ_TIMER3B    (52) /* Vector 52: Timer 3 B */
#  define TIVA_IRQ_I2C1       (53) /* Vector 53: I2C 1 */
#  define TIVA_IRQ_QEI1       (54) /* Vector 54: QEI1 */
#  define TIVA_IRQ_CAN0       (54) /* Vector 55: CAN0 */
#  define TIVA_RESERVED_56    (56) /* Vector 56: Reserved */
#  define TIVA_RESERVED_57    (57) /* Vector 57: Reserved */
#  define TIVA_IRQ_ETHCON     (58) /* Vector 58: Ethernet Controller */
#  define TIVA_IRQ_HIBERNATE  (59) /* Vector 59: Hibernation Module */

#  define TIVA_RESERVED_60    (60) /* Vector 60: Reserved */
#  define TIVA_RESERVED_61    (61) /* Vector 61: Reserved */
#  define TIVA_RESERVED_62    (62) /* Vector 62: Reserved */
#  define TIVA_RESERVED_63    (63) /* Vector 63: Reserved */
#  define TIVA_RESERVED_64    (64) /* Vector 64: Reserved */
#  define TIVA_RESERVED_65    (65) /* Vector 65: Reserved */
#  define TIVA_RESERVED_66    (66) /* Vector 66: Reserved */
#  define TIVA_RESERVED_67    (67) /* Vector 67: Reserved */
#  define TIVA_RESERVED_68    (68) /* Vector 68: Reserved */
#  define TIVA_RESERVED_69    (69) /* Vector 69: Reserved */

#  define TIVA_RESERVED_70    (70) /* Vector 70: Reserved */

#  define NR_IRQS             (71) /* (Really less because of reserved vectors) */

#else
#  error "IRQ Numbers not specified for this Stellaris chip"
#endif

/************************************************************************************
 * Public Types
 ************************************************************************************/

/************************************************************************************
 * Public Data
 ************************************************************************************/

#ifndef __ASSEMBLY__
#ifdef __cplusplus
extern "C"
{
#endif

/************************************************************************************
 * Public Functions
 ************************************************************************************/

#ifdef __cplusplus
}
#endif
#endif

#endif /* __ARCH_ARM_INCLUDE_TIVA_LM3S_IRQ_H */
