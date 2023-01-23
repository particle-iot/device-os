/**
 ******************************************************************************
 * @file    interrupts_hal.h
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2018 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __INTERRUPTS_HAL_H
#define __INTERRUPTS_HAL_H

#include "platforms.h"
/* Includes ------------------------------------------------------------------*/
#if defined(USE_STDPERIPH_DRIVER) || (!defined(SPARK_NO_PLATFORM) && !defined(INTERRUPTS_HAL_EXCLUDE_PLATFORM_HEADERS))
#include "interrupts_irq.h"
#else
#include <stdint.h>
typedef int32_t IRQn_Type;
typedef int32_t hal_irq_t;
#endif // SPARK_NO_PLATFORM
#include "pinmap_hal.h"
#include "hal_irq_flag.h"

/* Exported types ------------------------------------------------------------*/
typedef enum InterruptMode {
  CHANGE,
  RISING,
  FALLING
} InterruptMode;

typedef void (*hal_interrupt_handler_t)(void* data);

typedef void (*hal_interrupt_direct_handler_t)(void);

typedef enum {
  HAL_INTERRUPT_DIRECT_FLAG_NONE    = 0x00,
  HAL_INTERRUPT_DIRECT_FLAG_RESTORE = 0x01,
  HAL_INTERRUPT_DIRECT_FLAG_DISABLE = 0x02,
  HAL_INTERRUPT_DIRECT_FLAG_ENABLE  = 0x04
} hal_interrupt_direct_flags_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

typedef struct hal_interrupt_callback_t {
    hal_interrupt_handler_t handler;
    void* data;
} hal_interrupt_callback_t;

#define HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION_1 4
#define HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION_2 5
#define HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION_3 6
#define HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION_3

typedef struct hal_interrupt_extra_configuration_t {
  uint8_t version;
  uint8_t IRQChannelPreemptionPriority;
  uint8_t IRQChannelSubPriority;
  union {
    uint8_t flags;
    struct {
      uint8_t keepPriority  : 1;
      uint8_t keepHandler   : 1;
      uint8_t appendHandler : 1;
    };
  };
#if HAL_PLATFORM_SHARED_INTERRUPT
  uint8_t chainPriority;
#endif
} hal_interrupt_extra_configuration_t;

#ifdef __cplusplus
extern "C" {
#endif

void hal_interrupt_init(void);
void hal_interrupt_uninit(void);
int hal_interrupt_attach(uint16_t pin, hal_interrupt_handler_t handler, void* data, InterruptMode mode, hal_interrupt_extra_configuration_t* config);
int hal_interrupt_detach(uint16_t pin);
int hal_interrupt_detach_ext(uint16_t pin, uint8_t keepHandler, void* reserved);
void hal_interrupt_enable_all(void);
void hal_interrupt_disable_all(void);

void hal_interrupt_suspend(void);
void hal_interrupt_restore(void);

void hal_interrupt_trigger(uint16_t pin, void* reserved);


uint8_t hal_interrupt_set_system_handler(hal_irq_t irq, const hal_interrupt_callback_t* callback, hal_interrupt_callback_t* previous, void* reserved);
uint8_t hal_interrupt_get_system_handler(hal_irq_t irq, hal_interrupt_callback_t* callback, void* reserved);
void hal_interrupt_trigger_system(hal_irq_t irq, void* reserved);

int hal_interrupt_set_direct_handler(IRQn_Type irqn, hal_interrupt_direct_handler_t handler, uint32_t flags, void* reserved);

#ifdef USE_STDPERIPH_DRIVER

    #ifdef nRF52840
        #include <nrf52840.h>
    #endif /* nRF52840 */

#if defined(STM32F10X_MD) || defined(STM32F10X_HD) || defined(STM32F2XX) || defined(nRF52840)

static inline bool hal_interrupt_is_isr() {
	return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
}

static inline int32_t hal_interrupt_serviced_irqn() {
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) - 16;
}

static inline uint32_t hal_interrupt_get_basepri() {
    return (__get_BASEPRI() >> (8 - __NVIC_PRIO_BITS));
}

static inline bool hal_interrupt_is_irq_masked(int32_t irqn) {
    uint32_t basepri = hal_interrupt_get_basepri();
    return __get_PRIMASK() || (basepri > 0 && NVIC_GetPriority((IRQn_Type)irqn) >= basepri);
}

static inline bool hal_interrupt_will_preempt(int32_t irqn1, int32_t irqn2) {
    if (irqn1 == irqn2) {
        return false;
    }

    uint32_t priorityGroup = NVIC_GetPriorityGrouping();
    uint32_t priority1 = NVIC_GetPriority((IRQn_Type)irqn1);
    uint32_t priority2 = NVIC_GetPriority((IRQn_Type)irqn2);
    uint32_t p1, sp1, p2, sp2;
    NVIC_DecodePriority(priority1, priorityGroup, &p1, &sp1);
    NVIC_DecodePriority(priority2, priorityGroup, &p2, &sp2);
    if (p1 < p2) {
        return true;
    }
    return false;
}

#elif defined(CONFIG_PLATFORM_8721D)


static inline bool hal_interrupt_is_isr() {
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
}

static inline int32_t hal_interrupt_serviced_irqn() {
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) - 16;
}

static inline uint32_t hal_interrupt_get_basepri() {
#if defined (ARM_CPU_CORTEX_M33)
    return (__get_BASEPRI() >> (8 - __NVIC_PRIO_BITS));
#else
    return 0;
#endif
}

static inline bool hal_interrupt_is_irq_masked(int32_t irqn) {
    uint32_t basepri = hal_interrupt_get_basepri();
    return __get_PRIMASK() || (basepri > 0 && NVIC_GetPriority((IRQn_Type)irqn) >= basepri);
}

static inline bool hal_interrupt_will_preempt(int32_t irqn1, int32_t irqn2) {
    if (irqn1 == irqn2) {
        return false;
    }

    uint32_t priority1 = NVIC_GetPriority((IRQn_Type)irqn1);
    uint32_t priority2 = NVIC_GetPriority((IRQn_Type)irqn2);
#if defined (ARM_CPU_CORTEX_M33)
    uint32_t priorityGroup = NVIC_GetPriorityGrouping();
    uint32_t p1, sp1, p2, sp2;
    NVIC_DecodePriority(priority1, priorityGroup, &p1, &sp1);
    NVIC_DecodePriority(priority2, priorityGroup, &p2, &sp2);
    if (p1 < p2) {
        return true;
    }
#else
    if (priority1 < priority2) {
        return true;
    }
#endif // defined (ARM_CPU_CORTEX_M33)
    return false;
}

#elif PLATFORM_ID == PLATFORM_NEWHAL || PLATFORM_ID == PLATFORM_GCC

inline bool hal_interrupt_is_isr() {
    return false;
}

inline int32_t hal_interrupt_serviced_irqn() {
    return 0;
}

inline bool hal_interrupt_will_preempt(int32_t irqn1, int32_t irqn2) {
    return false;
}

inline uint32_t hal_interrupt_get_basepri() {
    return 0;
}

inline bool hal_interrupt_is_irq_masked(int32_t irqn) {
    return false;
}

#else

#error "*** MCU architecture not supported by hal_interrupt_is_isr(). ***"

#endif

#include "interrupts_hal_compat.h"

#endif // defined(USE_STDPERIPH_DRIVER)

#ifdef __cplusplus
}
#endif

#endif  /* __INTERRUPTS_HAL_H */
