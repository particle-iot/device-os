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

#include "rtl8721d_vector_defines.h"

#if defined (ARM_CPU_CORTEX_M33)
#ifdef AMEBAD_TODO
#ifndef __ARMV8MML_REV
#define __ARMV8MML_REV                 0x0000U  /*!< ARMV8MML Core Revision                                                    */
#endif // __ARMV8MML_REV
#ifndef __Vendor_SysTickConfig
#define __Vendor_SysTickConfig         0        /*!< Set to 1 if different SysTick Config is used                              */
#endif // __Vendor_SysTickConfig
#ifndef __VTOR_PRESENT
#define __VTOR_PRESENT                 1        /*!< Set to 1 if CPU supports Vector Table Offset Register                     */
#endif // __VTOR_PRESENT
#ifndef __FPU_DP
#define __FPU_DP                       0        /*!< Double Precision FPU                                                      */
#endif // __FPU_DP
#endif
#ifndef __CM3_REV
#define __CM3_REV                      0x0200    /**< Core revision r0p0 */
#endif // __CM3_REV
#ifndef __MPU_PRESENT
#define __MPU_PRESENT                  1         /**< Defines if an MPU is present or not */
#endif // __MPU_PRESENT
#ifndef __NVIC_PRIO_BITS
#define __NVIC_PRIO_BITS               3         /**< Number of priority bits implemented in the NVIC */
#endif // __NVIC_PRIO_BITS
#ifndef __Vendor_SysTickConfig
#define __Vendor_SysTickConfig         1         /**< Vendor specific implementation of SysTickConfig is defined *///see vPortSetupTimerInterrupt
#endif // __Vendor_SysTickConfig
#ifndef __SAUREGION_PRESENT
#define __SAUREGION_PRESENT            1        /*!< SAU present or not                                                        */
#endif // __SAUREGION_PRESENT

#ifndef __FPU_PRESENT
#define __FPU_PRESENT             1       /*!< FPU present                                   */
#endif // __FPU_PRESENT
#ifndef __VFP_FP__
#define __VFP_FP__	1
#endif // __VFP_FP__
#ifndef __ARM_FEATURE_CMSE
#define __ARM_FEATURE_CMSE	3
#endif
#include <arm_cmse.h>   /* Use CMSE intrinsics */
#include "core_armv8mml.h"
#include "core_cache.h"
#else
#ifndef __ARMV8MBL_REV
#define __ARMV8MBL_REV                 0x0000U  /*!< ARMV8MBL Core Revision                                                    */
#endif // __ARMV8MBL_REV
#ifndef __NVIC_PRIO_BITS
#define __NVIC_PRIO_BITS               2        /*!< Number of Bits used for Priority Levels                                   */
#endif // __NVIC_PRIO_BITS
#ifndef __Vendor_SysTickConfig
#define __Vendor_SysTickConfig         0        /*!< Set to 1 if different SysTick Config is used                              */
#endif // __Vendor_SysTickConfig
#ifndef __VTOR_PRESENT
#define __VTOR_PRESENT                 1        /*!< Set to 1 if CPU supports Vector Table Offset Register                     */
#endif // __VTOR_PRESENT
#ifndef __SAU_REGION_PRESENT
#define __SAU_REGION_PRESENT           0        /*!< SAU present or not                                                        */
#endif // __SAU_REGION_PRESENT

#ifndef __MPU_PRESENT
#define __MPU_PRESENT                  1         /**< Defines if an MPU is present or not */
#endif // __MPU_PRESENT
#include "core_armv8mbl.h"
#include "core_cache.h"
#endif // #if defined (ARM_CPU_CORTEX_M33)
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
