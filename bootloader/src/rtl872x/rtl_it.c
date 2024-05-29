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

#include "rtl_it.h"
#include <stdbool.h>
#include <rtl8721d.h>
#include "logging.h"
#include "hw_config.h"
#include "button_hal.h"
#include "hal_platform_config.h"
#include "interrupts_irq.h"
#include "security_mode.h"

extern void Timing_Decrement(void);

void HardFault_Handler(void) __attribute__(( naked ));
void MemManage_Handler(void) __attribute__(( naked ));
void BusFault_Handler(void) __attribute__(( naked ));
void UsageFault_Handler(void) __attribute__(( naked ));
void SecureFault_Handler(void) __attribute__(( naked ));

static __attribute__((always_inline)) inline bool is_address_in_rom(uint32_t addr) {
    // XXX: we don't have ROM linker symbols, should probably add them
    if (addr >= 0x10100000 && addr < (0x101C8000 + 0x10000)) {
        return true;
    }
    return false;
}

__attribute__((externally_visible)) void prvGetRegistersFromStack(uint32_t *pulFaultStackAddress, uint32_t panicCode ) {
    /* These are volatile to try and prevent the compiler/linker optimising them
    away as the variables never actually get used.  If the debugger won't show the
    values of the variables, make them global my moving their declaration outside
    of this function. */
    volatile uint32_t r0;
    volatile uint32_t r1;
    volatile uint32_t r2;
    volatile uint32_t r3;
    volatile uint32_t r12;
    volatile uint32_t lr; /* Link register. */
    volatile uint32_t pc; /* Program counter. */
    volatile uint32_t psr;/* Program status register. */

    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    /* Silence "variable set but not used" error */
    if (false) {
        (void)r0; (void)r1; (void)r2; (void)r3; (void)r12; (void)lr; (void)pc; (void)psr;
    }

    if (SCB->CFSR & (1<<25) /* DIVBYZERO */) {
        // stay consistent with the core and cause 5 flashes
        panicCode = UsageFault;
    }

    switch (panicCode) {
        case HardFault: {
            PANIC(panicCode, "HardFault");
            break; 
        }
        case MemManage: {
            PANIC(panicCode, "MemManage");
            break; 
        }
        case BusFault: {
            PANIC(panicCode, "BusFault");
            break; 
        }
        case UsageFault: {
            PANIC(panicCode, "UsageFault");
            break; 
        }
        case SecureFault: {
            PANIC(panicCode, "SecureFault");
            break;
        }
        default: {
            // Shouldn't enter this case
            PANIC(panicCode, "Unknown");
            break;
        }
    }

    /* Go to infinite loop when Hard Fault exception occurs */
    while (1) {
        ;
    }
}


__attribute__(( naked )) void Fault_Handler(uint32_t panic_code) {
    __asm volatile
    (
        " mov r1, r0                                                \n"
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r2, handler2_address_const                            \n"
        " bx r2                                                     \n"
        " .balign 4                                                 \n"
        " handler2_address_const: .word prvGetRegistersFromStack    \n"
    );
}

void HardFault_Handler(void) {
    Fault_Handler(HardFault);
}

void MemManage_Handler(void) {
    /* Go to infinite loop when Memory Manage exception occurs */
    Fault_Handler(MemManage);
}

void BusFault_Handler(void) {
    /* Go to infinite loop when Bus Fault exception occurs */
    Fault_Handler(BusFault);
}

void UsageFault_Handler(void) {
    /* Go to infinite loop when Usage Fault exception occurs */
    Fault_Handler(UsageFault);
}

void SecureFault_Handler_NS(void) {
    Fault_Handler(SecureFault);
}

__attribute__((always_inline)) inline
void jump_to_nonsecure(u32 Addr) {
    __ASM volatile ("MOV r0, %0\n\t"
        "BLXNS   r0\n\t" : : "r" (Addr));
}

__attribute__((used, section(".secure.ram.text")))
void SecureFault_Handler(void) {
    volatile uint32_t handler = (uint32_t)&SecureFault_Handler_NS;
    if (SCB_NS->VTOR) {
        volatile uint32_t handler_ns = ((uint32_t*)SCB_NS->VTOR)[IRQN_TO_IDX(SecureFault_IRQn)];
        volatile bool in_rom = is_address_in_rom(handler_ns);
        if (!in_rom) {
            handler = handler_ns;
        }
    }
    volatile uint32_t ptr = cmse_nsfptr_create(handler);
    jump_to_nonsecure(ptr);
}

void SysTick_Handler(void)
{
    System1MsTick();
    security_mode_notify_system_tick();
    Timing_Decrement();

#if HAL_PLATFORM_BUTTON_DEBOUNCE_IN_SYSTICK
    hal_button_timer_handler();
#endif
}
