/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INTERRUPTS_IRQ_H
#define INTERRUPTS_IRQ_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "stdint.h"

#ifdef USE_STDPERIPH_DRIVER
// FIXME
typedef int32_t IRQn_Type;
#ifdef AMEBAD_TODO
#define __ARMV8MML_REV                 0x0000U  /*!< ARMV8MML Core Revision                                                    */
#define __Vendor_SysTickConfig         0        /*!< Set to 1 if different SysTick Config is used                              */
#define __VTOR_PRESENT                 1        /*!< Set to 1 if CPU supports Vector Table Offset Register                     */
#define __FPU_DP                       0        /*!< Double Precision FPU                                                      */
#endif
#define __CM3_REV                      0x0200    /**< Core revision r0p0 */
#define __MPU_PRESENT                  1         /**< Defines if an MPU is present or not */
#define __NVIC_PRIO_BITS               3         /**< Number of priority bits implemented in the NVIC */
#define __Vendor_SysTickConfig         1         /**< Vendor specific implementation of SysTickConfig is defined *///see vPortSetupTimerInterrupt
#define __SAUREGION_PRESENT            1        /*!< SAU present or not                                                        */

#define __FPU_PRESENT             1       /*!< FPU present                                   */
#define __VFP_FP__	1
#ifndef __ARM_FEATURE_CMSE
#define __ARM_FEATURE_CMSE	3
#endif
#include <arm_cmse.h>   /* Use CMSE intrinsics */
#include "core_armv8mml.h"
#include "core_cache.h"
#endif /* USE_STDPERIPH_DRIVER */

typedef enum hal_irq_t {
    __Last_irq = 0
} hal_irq_t;

#define IRQN_TO_IDX(irqn) ((int)irqn + 16)

void HAL_Core_Restore_Interrupt(IRQn_Type irqn);
#ifdef  __cplusplus
}
#endif

#endif  /* INTERRUPTS_IRQ_H */
