/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

void HardFault_Handler(void) __attribute__(( naked ));
void MemManage_Handler(void) __attribute__(( naked ));
void BusFault_Handler(void) __attribute__(( naked ));
void UsageFault_Handler(void) __attribute__(( naked ));

#define HardFault           1
#define NMIFault            2
#define MemManage           3
#define BusFault            4
#define UsageFault          5

extern CPU_PWR_SEQ HSPWR_OFF_SEQ[];

// Use the same name as KM4 in case of forgetting bugfix
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

    // Turn off KM4 and reboot KM0
    BOOT_ROM_CM4PON((u32)HSPWR_OFF_SEQ);
    NVIC_SystemReset();

    /* Go to infinite loop when Hard Fault exception occurs */
    while (1) {
        ;
    }
}

__attribute__(( naked )) void Fault_Handler(uint32_t panic_code) {
    __asm volatile
    (
        " mov r1, r0                                                \n"
        " mrs r0, msp                                               \n"
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
