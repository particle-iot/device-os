/*
 * Copyright (c) 2022 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

typedef hal_interrupt_handler_t HAL_InterruptHandler __attribute__((deprecated("Use hal_interrupt_handler_t")));
typedef hal_interrupt_direct_handler_t HAL_Direct_Interrupt_Handler __attribute__((deprecated("Use hal_interrupt_handler_t")));
typedef hal_interrupt_direct_flags_t HAL_Direct_Interrupt_Flags __attribute__((deprecated("Use hal_interrupt_direct_flags_t")));
typedef hal_interrupt_callback_t HAL_InterruptCallback __attribute__((deprecated("Use hal_interrupt_callback_t")));
typedef hal_interrupt_extra_configuration_t HAL_InterruptExtraConfiguration __attribute__((deprecated(("Use hal_interrupt_extra_configuration_t"))));

static inline void __attribute__((deprecated("Use hal_interrupt_init() instead"), always_inline))
HAL_Interrupts_Init(void) {
    hal_interrupt_init();
}

static inline void __attribute__((deprecated("Use hal_interrupt_uninit() instead"), always_inline))
HAL_Interrupts_Uninit(void) {
    hal_interrupt_uninit();
}

static inline int __attribute__((deprecated("Use hal_interrupt_attach() instead"), always_inline))
HAL_Interrupts_Attach(uint16_t pin, HAL_InterruptHandler handler, void* data, InterruptMode mode, HAL_InterruptExtraConfiguration* config) {
    return hal_interrupt_attach(pin, handler, data, mode, config);
}

static inline int __attribute__((deprecated("Use hal_interrupt_detach() instead"), always_inline))
HAL_Interrupts_Detach(uint16_t pin) {
    return hal_interrupt_detach(pin);
}

static inline int __attribute__((deprecated("Use hal_interrupt_detach_ext() instead"), always_inline))
HAL_Interrupts_Detach_Ext(uint16_t pin, uint8_t keepHandler, void* reserved) {
    return hal_interrupt_detach_ext(pin, keepHandler, reserved);
}

static inline void __attribute__((deprecated("Use hal_interrupt_enable_all() instead"), always_inline))
HAL_Interrupts_Enable_All(void) {
    hal_interrupt_enable_all();
}

static inline void __attribute__((deprecated("Use hal_interrupt_disable_all() instead"), always_inline))
HAL_Interrupts_Disable_All(void) {
    hal_interrupt_disable_all();
}

static inline void __attribute__((deprecated("Use hal_interrupt_suspend() instead"), always_inline))
HAL_Interrupts_Suspend(void) {
    hal_interrupt_suspend();
}

static inline void __attribute__((deprecated("Use hal_interrupt_restore() instead"), always_inline))
HAL_Interrupts_Restore(void) {
    hal_interrupt_restore();
}

static inline void __attribute__((deprecated("Use hal_interrupt_trigger() instead"), always_inline))
HAL_Interrupts_Trigger(uint16_t pin, void* reserved) {
    hal_interrupt_trigger(pin, reserved);
}

static inline uint8_t __attribute__((deprecated("Use hal_interrupt_set_system_handler() instead"), always_inline))
HAL_Set_System_Interrupt_Handler(hal_irq_t irq, const HAL_InterruptCallback* callback, HAL_InterruptCallback* previous, void* reserved) {
    return hal_interrupt_set_system_handler(irq, callback, previous, reserved);
}

static inline uint8_t __attribute__((deprecated("Use hal_interrupt_get_system_handler() instead"), always_inline))
HAL_Get_System_Interrupt_Handler(hal_irq_t irq, HAL_InterruptCallback* callback, void* reserved) {
    return hal_interrupt_get_system_handler(irq, callback, reserved);
}

static inline void __attribute__((deprecated("Use hal_interrupt_trigger_system() instead"), always_inline))
HAL_System_Interrupt_Trigger(hal_irq_t irq, void* reserved) {
    hal_interrupt_trigger_system(irq, reserved);
}

static inline int __attribute__((deprecated("Use hal_interrupt_set_direct_handler() instead"), always_inline))
HAL_Set_Direct_Interrupt_Handler(IRQn_Type irqn, HAL_Direct_Interrupt_Handler handler, uint32_t flags, void* reserved) {
    return hal_interrupt_set_direct_handler(irqn, handler, flags, reserved);
}

static inline bool __attribute__((deprecated("Use hal_interrupt_is_isr() instead"), always_inline))
HAL_IsISR() {
    return hal_interrupt_is_isr();
}

static inline int32_t __attribute__((deprecated("Use hal_interrupt_serviced_irqn() instead"), always_inline))
HAL_ServicedIRQn() {
    return hal_interrupt_serviced_irqn();
}

static inline uint32_t __attribute__((deprecated("Use hal_interrupt_get_basepri() instead"), always_inline))
HAL_GetBasePri() {
    return hal_interrupt_get_basepri();
}

static inline bool __attribute__((deprecated("Use hal_interrupt_is_irq_masked() instead"), always_inline))
HAL_IsIrqMasked(int32_t irqn) {
    return hal_interrupt_is_irq_masked(irqn);
}

static inline bool __attribute__((deprecated("Use hal_interrupt_will_preempt() instead"), always_inline))
HAL_WillPreempt(int32_t irqn1, int32_t irqn2) {
    return hal_interrupt_will_preempt(irqn1, irqn2);
}
