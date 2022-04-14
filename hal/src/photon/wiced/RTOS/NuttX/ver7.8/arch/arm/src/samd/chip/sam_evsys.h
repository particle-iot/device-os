/********************************************************************************************
 * arch/arm/src/samd/chip/sam_evsys.h
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * References:
 *   "Atmel SAM D20J / SAM D20G / SAM D20E ARM-Based Microcontroller
 *   Datasheet", 42129J�SAM�12/2013
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
 ********************************************************************************************/

#ifndef __ARCH_ARM_SRC_SAMD_CHIP_SAM_EVSYS_H
#define __ARCH_ARM_SRC_SAMD_CHIP_SAM_EVSYS_H

/********************************************************************************************
 * Included Files
 ********************************************************************************************/

#include <nuttx/config.h>

#include "chip.h"
#include "chip/sam_sercom.h"

/********************************************************************************************
 * Pre-processor Definitions
 ********************************************************************************************/
/* EVSYS register offsets *******************************************************************/

#define SAM_EVSYS_CTRL_OFFSET        0x0000  /* Control register */
#define SAM_EVSYS_CHANNEL_OFFSET     0x0004  /* Channel register */
#define SAM_EVSYS_USER_OFFSET        0x0008  /* User multiplexer register */
#define SAM_EVSYS_CHSTATUS_OFFSET    0x000c  /* Channel status register */
#define SAM_EVSYS_INTENCLR_OFFSET    0x0010  /* Interrupt enable clear register */
#define SAM_EVSYS_INTENSET_OFFSET    0x0014  /* Interrupt enable set register */
#define SAM_EVSYS_INTFLAG_OFFSET     0x0018  /* Interrupt flag status and clear register */

/* EVSYS register addresses *****************************************************************/

#define SAM_EVSYS_CTRL               (SAM_EVSYS_BASE+SAM_EVSYS_CTRL_OFFSET)
#define SAM_EVSYS_CHANNEL            (SAM_EVSYS_BASE+SAM_EVSYS_CHANNEL_OFFSET)
#define SAM_EVSYS_USER               (SAM_EVSYS_BASE+SAM_EVSYS_USER_OFFSET)
#define SAM_EVSYS_CHSTATUS           (SAM_EVSYS_BASE+SAM_EVSYS_CHSTATUS_OFFSET
#define SAM_EVSYS_INTENCLR           (SAM_EVSYS_BASE+SAM_EVSYS_INTENCLR_OFFSET
#define SAM_EVSYS_INTENSET           (SAM_EVSYS_BASE+SAM_EVSYS_INTENSET_OFFSET
#define SAM_EVSYS_INTFLAG            (SAM_EVSYS_BASE+SAM_EVSYS_INTFLAG_OFFSET)

/* EVSYS register bit definitions ***********************************************************/

/* Control register */

#define EVSYS_CTRL_SWRST             (1 << 0)  /* Bit 0: Software Reset */
#define EVSYS_CTRL_GCLKREQ           (1 << 4)  /* Bit 4: Generic Clock Requests */

/* Channel register */

#define EVSYS_CHANNEL_SHIFT          (0)       /* Bits 0-7: Channel Selection */
#define EVSYS_CHANNEL_MASK           (0xff << EVSYS_CHANNEL_SHIFT)
#  define EVSYS_CHANNEL(n)           ((uint32_t)(n) << EVSYS_CHANNEL_SHIFT)
#  define EVSYS_CHANNEL_1            (1 << EVSYS_CHANNEL_SHIFT)
#  define EVSYS_CHANNEL_2            (2 << EVSYS_CHANNEL_SHIFT)
#  define EVSYS_CHANNEL_3            (3 << EVSYS_CHANNEL_SHIFT)
#  define EVSYS_CHANNEL_4            (4 << EVSYS_CHANNEL_SHIFT)
#  define EVSYS_CHANNEL_5            (5 << EVSYS_CHANNEL_SHIFT)
#  define EVSYS_CHANNEL_6            (6 << EVSYS_CHANNEL_SHIFT)
#  define EVSYS_CHANNEL_7            (7 << EVSYS_CHANNEL_SHIFT)
#define EVSYS_CHANNEL_SWEVT          (1 << 8)  /* Bit 8: Software Event */
#define EVSYS_CHANNEL_EVGEN_SHIFT    (16)      /* Bits 16-23: Event Generator */
#define EVSYS_CHANNEL_EVGEN_MASK     (0xff << EVSYS_CHANNEL_EVGEN_SHIFT)
#  define EVSYS_CHANNEL_EVGEN_NONE      (0 << EVSYS_CHANNEL_EVGEN_SHIFT)  /* No event generator selected */
#  define EVSYS_CHANNEL_EVGEN_RTCCMP0   (1 << EVSYS_CHANNEL_EVGEN_SHIFT)  /* RTC Compare 0 or Alarm 0 */
#  define EVSYS_CHANNEL_EVGEN_RTCCMP1   (2 << EVSYS_CHANNEL_EVGEN_SHIFT)  /* Compare 1 */
#  define EVSYS_CHANNEL_EVGEN_RTCOVF    (3 << EVSYS_CHANNEL_EVGEN_SHIFT)  /* RTC Overflow */
#  define EVSYS_CHANNEL_EVGEN_RTCPER0   (4 << EVSYS_CHANNEL_EVGEN_SHIFT)  /* RTC Period 0 */
#  define EVSYS_CHANNEL_EVGEN_RTCPER1   (5 << EVSYS_CHANNEL_EVGEN_SHIFT)  /* RTC Period 1 */
#  define EVSYS_CHANNEL_EVGEN_RTCPER2   (6 << EVSYS_CHANNEL_EVGEN_SHIFT)  /* RTC Period 2 */
#  define EVSYS_CHANNEL_EVGEN_RTCPER3   (7 << EVSYS_CHANNEL_EVGEN_SHIFT)  /* RTC Period 3 */
#  define EVSYS_CHANNEL_EVGEN_RTCPER4   (8 << EVSYS_CHANNEL_EVGEN_SHIFT)  /* RTC Period 4 */
#  define EVSYS_CHANNEL_EVGEN_RTCPER5   (9 << EVSYS_CHANNEL_EVGEN_SHIFT)  /* RTC Period 5 */
#  define EVSYS_CHANNEL_EVGEN_RTCPER6   (10 << EVSYS_CHANNEL_EVGEN_SHIFT) /* RTC Period 6 */
#  define EVSYS_CHANNEL_EVGEN_RTCPER7   (11 << EVSYS_CHANNEL_EVGEN_SHIFT) /* RTC Period 7 */
#  define EVSYS_CHANNEL_EVGEN_EXTINT0   (12 << EVSYS_CHANNEL_EVGEN_SHIFT) /* EIC External Interrupt 0 */
#  define EVSYS_CHANNEL_EVGEN_EXTINT1   (13 << EVSYS_CHANNEL_EVGEN_SHIFT) /* EIC External Interrupt 1 */
#  define EVSYS_CHANNEL_EVGEN_EXTINT2   (14 << EVSYS_CHANNEL_EVGEN_SHIFT) /* EIC External Interrupt 2 */
#  define EVSYS_CHANNEL_EVGEN_EXTINT3   (15 << EVSYS_CHANNEL_EVGEN_SHIFT) /* EIC External Interrupt 3 */
#  define EVSYS_CHANNEL_EVGEN_EXTINT4   (16 << EVSYS_CHANNEL_EVGEN_SHIFT) /* EIC External Interrupt 4 */
#  define EVSYS_CHANNEL_EVGEN_EXTINT5   (17 << EVSYS_CHANNEL_EVGEN_SHIFT) /* EIC External Interrupt 5 */
#  define EVSYS_CHANNEL_EVGEN_EXTINT6   (18 << EVSYS_CHANNEL_EVGEN_SHIFT) /* EIC External Interrupt 6 */
#  define EVSYS_CHANNEL_EVGEN_EXTINT7   (19 << EVSYS_CHANNEL_EVGEN_SHIFT) /* EIC External Interrupt 7 */
#  define EVSYS_CHANNEL_EVGEN_EXTINT8   (20 << EVSYS_CHANNEL_EVGEN_SHIFT) /* EIC External Interrupt 8 */
#  define EVSYS_CHANNEL_EVGEN_EXTINT9   (21 << EVSYS_CHANNEL_EVGEN_SHIFT) /* EIC External Interrupt 9 */
#  define EVSYS_CHANNEL_EVGEN_EXTINT10  (22 << EVSYS_CHANNEL_EVGEN_SHIFT) /* EIC External Interrupt 10 */
#  define EVSYS_CHANNEL_EVGEN_EXTINT11  (23 << EVSYS_CHANNEL_EVGEN_SHIFT) /* EIC External Interrupt 11 */
#  define EVSYS_CHANNEL_EVGEN_EXTINT12  (24 << EVSYS_CHANNEL_EVGEN_SHIFT) /* EIC External Interrupt 12 */
#  define EVSYS_CHANNEL_EVGEN_EXTINT13  (25 << EVSYS_CHANNEL_EVGEN_SHIFT) /* EIC External Interrupt 13 */
#  define EVSYS_CHANNEL_EVGEN_EXTINT14  (26 << EVSYS_CHANNEL_EVGEN_SHIFT) /* EIC External Interrupt 14 */
#  define EVSYS_CHANNEL_EVGEN_EXTINT15  (27 << EVSYS_CHANNEL_EVGEN_SHIFT) /* EIC External Interrupt 15 */
#  define EVSYS_CHANNEL_EVGEN_TC0OVF    (28 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC0 Overflow/Underflow */
#  define EVSYS_CHANNEL_EVGEN_TC0MC0    (29 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC0 Match/Capture 0 */
#  define EVSYS_CHANNEL_EVGEN_TC0MC1    (30 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC0 Match/Capture 1 */
#  define EVSYS_CHANNEL_EVGEN_TC1OVF    (31 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC1 Overflow/Underflow */
#  define EVSYS_CHANNEL_EVGEN_TC1MC0    (32 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC1 Match/Capture 0 */
#  define EVSYS_CHANNEL_EVGEN_TC1MC1    (33 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC1 Match/Capture 1 */
#  define EVSYS_CHANNEL_EVGEN_TC2OVF    (34 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC2 Overflow/Underflow */
#  define EVSYS_CHANNEL_EVGEN_TC2MC0    (35 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC2 Match/Capture 0 */
#  define EVSYS_CHANNEL_EVGEN_TC2MC1    (36 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC2 Match/Capture 1 */
#  define EVSYS_CHANNEL_EVGEN_TC3OVF    (37 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC3 Overflow/Underflow */
#  define EVSYS_CHANNEL_EVGEN_TC3MC0    (38 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC3 Match/Capture 0 */
#  define EVSYS_CHANNEL_EVGEN_TC3MC1    (39 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC3 Match/Capture 1 */
#  define EVSYS_CHANNEL_EVGEN_TC4OVF    (40 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC4 Overflow/Underflow */
#  define EVSYS_CHANNEL_EVGEN_TC4MC0    (41 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC4 Match/Capture 0 */
#  define EVSYS_CHANNEL_EVGEN_TC4MC1    (42 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC4 Match/Capture 1 */
#  define EVSYS_CHANNEL_EVGEN_TC5OVF    (43 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC5 Overflow/Underflow */
#  define EVSYS_CHANNEL_EVGEN_TC5MC0    (44 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC5 Match/Capture 0 */
#  define EVSYS_CHANNEL_EVGEN_TC5MC1    (45 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC5 Match/Capture 1 */
#  define EVSYS_CHANNEL_EVGEN_TC6OVF    (46 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC6 Overflow/Underflow */
#  define EVSYS_CHANNEL_EVGEN_TC6MC0    (47 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC6 Match/Capture 0 */
#  define EVSYS_CHANNEL_EVGEN_TC6MC1    (48 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC6 Match/Capture 1 */
#  define EVSYS_CHANNEL_EVGEN_TC7OVF    (49 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC7 Overflow/Underflow */
#  define EVSYS_CHANNEL_EVGEN_TC7MC0    (50 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC7 Match/Capture 0 */
#  define EVSYS_CHANNEL_EVGEN_TC7MC1    (51 << EVSYS_CHANNEL_EVGEN_SHIFT) /* TC7 Match/Capture 1 */
#  define EVSYS_CHANNEL_EVGEN_ADCRESRDY (52 << EVSYS_CHANNEL_EVGEN_SHIFT) /* ADC  Result Ready */
#  define EVSYS_CHANNEL_EVGEN_ADCWINMON (53 << EVSYS_CHANNEL_EVGEN_SHIFT) /* ADC  Window Monitor */
#  define EVSYS_CHANNEL_EVGEN_ACCOMP0   (54 << EVSYS_CHANNEL_EVGEN_SHIFT) /* AC  Comparator 0 */
#  define EVSYS_CHANNEL_EVGEN_ACCOMP1   (55 << EVSYS_CHANNEL_EVGEN_SHIFT) /* AC  Comparator 1 */
#  define EVSYS_CHANNEL_EVGEN_ACWIN     (56 << EVSYS_CHANNEL_EVGEN_SHIFT) /* AC  Window 0 */
#  define EVSYS_CHANNEL_EVGEN_DACEMPTY  (57 << EVSYS_CHANNEL_EVGEN_SHIFT) /* DAC  Data Buffer Empty */
#  define EVSYS_CHANNEL_EVGEN_PTCEOC    (58 << EVSYS_CHANNEL_EVGEN_SHIFT) /* PTC  End of Conversion */
#  define EVSYS_CHANNEL_EVGEN_PTCWCOMP  (59 << EVSYS_CHANNEL_EVGEN_SHIFT) /* PTC  Window Comparator */
#define EVSYS_CHANNEL_PATH_SHIFT     (24)       /* Bits 24-25: Path Selection */
#define EVSYS_CHANNEL_PATH_MASK      (3 << EVSYS_CHANNEL_PATH_SHIFT)
#  define EVSYS_CHANNEL_PATH_SYNCH      (0 << EVSYS_CHANNEL_PATH_SHIFT) /* Synchronous path */
#  define EVSYS_CHANNEL_PATH_RESYNCH    (1 << EVSYS_CHANNEL_PATH_SHIFT) /* Resynchronized path */
#  define EVSYS_CHANNEL_PATH_ASYNCH     (2 << EVSYS_CHANNEL_PATH_SHIFT) /* Asynchronous path */
#define EVSYS_CHANNEL_EDGSEL_SHIFT   (26)       /* Bits 26-27: Edge Detection Selection */
#define EVSYS_CHANNEL_EDGSEL_MASK    (3 << EVSYS_CHANNEL_EDGSEL_SHIFT)
#  define EVSYS_CHANNEL_EDGSEL_NOEVT    (0 << EVSYS_CHANNEL_EDGSEL_SHIFT) /* No event output */
#  define EVSYS_CHANNEL_EDGSEL_RISING   (1 << EVSYS_CHANNEL_EDGSEL_SHIFT) /* Event detection on rising edge */
#  define EVSYS_CHANNEL_EDGSEL_FALLING  (2 << EVSYS_CHANNEL_EDGSEL_SHIFT) /* Event detection on falling edge */
#  define EVSYS_CHANNEL_EDGSEL_BOTH     (3 << EVSYS_CHANNEL_EDGSEL_SHIFT) /* Event detection on both edges */

/* User multiplexer register */

#define EVSYS_USER_USER_SHIFT        (0)       /* Bits 0-7: User Multiplexer Selection */
#define EVSYS_USER_USER_MASK         (0xff << EVSYS_USER_USER_SHIFT)
#  define EVSYS_USER_USER_TC0        (0 << EVSYS_USER_USER_SHIFT)  /* TC0 paths */
#  define EVSYS_USER_USER_TC1        (1 << EVSYS_USER_USER_SHIFT)  /* TC1 paths */
#  define EVSYS_USER_USER_TC2        (2 << EVSYS_USER_USER_SHIFT)  /* TC2 paths */
#  define EVSYS_USER_USER_TC3        (3 << EVSYS_USER_USER_SHIFT)  /* TC3 paths */
#  define EVSYS_USER_USER_TC4        (4 << EVSYS_USER_USER_SHIFT)  /* TC4 paths */
#  define EVSYS_USER_USER_TC5        (5 << EVSYS_USER_USER_SHIFT)  /* TC5 paths */
#  define EVSYS_USER_USER_TC6        (6 << EVSYS_USER_USER_SHIFT)  /* TC6 paths */
#  define EVSYS_USER_USER_TC7        (7 << EVSYS_USER_USER_SHIFT)  /* TC7 paths */
#  define EVSYS_USER_USER_ADCSTART   (8 << EVSYS_USER_USER_SHIFT)  /* ADC start conversion asynch path */
#  define EVSYS_USER_USER_ADCSYNC    (9 << EVSYS_USER_USER_SHIFT)  /* Flush ADC asynch path */
#  define EVSYS_USER_USER_ACCOMP0    (10 << EVSYS_USER_USER_SHIFT) /* Start comparator 0 asynch path */
#  define EVSYS_USER_USER_ACCOMP1    (11 << EVSYS_USER_USER_SHIFT) /* Start comparator 1 asynch path */
#  define EVSYS_USER_USER_DACSTART   (12 << EVSYS_USER_USER_SHIFT) /* DAC start conversion asynch path */
#  define EVSYS_USER_USER_PTCSTCONV  (13 << EVSYS_USER_USER_SHIFT) /* PTC start conversion asynch path */
#define EVSYS_USER_CHANNEL_SHIFT     (8)       /* Bits 8-15: Channel Event Selection */
#define EVSYS_USER_CHANNEL_MASK      (0xff << EVSYS_USER_CHANNEL_SHIFT)
#  define EVSYS_USER_CHANNEL(n)      ((uint16_t)((n)+1) << EVSYS_USER_CHANNEL_SHIFT)
#  define EVSYS_USER_CHANNEL_NONE    (0 << EVSYS_USER_CHANNEL_SHIFT) /* No channel output selected
#  define EVSYS_USER_CHANNEL_0       (1 << EVSYS_USER_CHANNEL_SHIFT) /* Channel 0 selected */
#  define EVSYS_USER_CHANNEL_1       (2 << EVSYS_USER_CHANNEL_SHIFT) /* Channel 0 selected */
#  define EVSYS_USER_CHANNEL_2       (3 << EVSYS_USER_CHANNEL_SHIFT) /* Channel 0 selected */
#  define EVSYS_USER_CHANNEL_3       (4 << EVSYS_USER_CHANNEL_SHIFT) /* Channel 0 selected */
#  define EVSYS_USER_CHANNEL_4       (5 << EVSYS_USER_CHANNEL_SHIFT) /* Channel 0 selected */
#  define EVSYS_USER_CHANNEL_5       (6 << EVSYS_USER_CHANNEL_SHIFT) /* Channel 0 selected */
#  define EVSYS_USER_CHANNEL_6       (7 << EVSYS_USER_CHANNEL_SHIFT) /* Channel 0 selected */
#  define EVSYS_USER_CHANNEL_7       (8 << EVSYS_USER_CHANNEL_SHIFT) /* Channel 0 selected */

/* Channel status register */

#define EVSYS_CHSTATUS_USRRDY_SHIFT  (0)       /* Bits 0-7: User Ready for Channel n */
#define EVSYS_CHSTATUS_USRRDY_MASK   (0xff << EVSYS_CHSTATUS_USRRDY_SHIFT)
#  define EVSYS_CHSTATUS_USRRDY(n)   ((uint32_t)(n) << EVSYS_CHSTATUS_USRRDY_SHIFT)
#define EVSYS_CHSTATUS_CHBUSY_SHIFT  (8)       /* Bits 8-15: Channel Busy n */
#  define EVSYS_CHSTATUS_CHBUSY(n)   ((uint32_t)(n) << EVSYS_CHSTATUS_CHBUSY_SHIFT)

/* Interrupt enable clear, interrupt enable set, and interrupt flag status and clear registers */

#define EVSYS_INT_OVR_SHIFT          (0)       /* Bits 0-7: Overrun channel n interrupt */
#define EVSYS_INT_OVR_MASK           (0xff << EVSYS_INT_OVR_SHIFT)
#  define EVSYS_INT_OVR(n)           (1 << (n)
#  define EVSYS_INT_OVR0             (1 << 0)  /* Bit 0: Overrun channel 0 interrupt */
#define EVSYS_INT_EVD_SHIFT          (8)       /* Bits 8-15: Event detected channel n interrupt */
#define EVSYS_INT_EVD_MASK           (0xff << EVSYS_INT_EVD_SHIFT)
#  define EVSYS_INT_EVD(n)           (1 << ((n)+8))
#  define EVSYS_INT_EVD0             (1 << 8)  /* Bit 8: Event detected channel 0 interrupt */

/********************************************************************************************
 * Public Types
 ********************************************************************************************/

/********************************************************************************************
 * Public Data
 ********************************************************************************************/

/********************************************************************************************
 * Public Functions
 ********************************************************************************************/

#endif /* __ARCH_ARM_SRC_SAMD_CHIP_SAM_EVSYS_H */
