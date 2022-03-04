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

#include "nonsecure.h"

extern "C" {
#include "rtl8721d.h"
}

/*
// <e>Setup behaviour of Sleep and Exception Handling
*/
#define SCB_CSR_AIRCR_INIT  1

/*
//   <o> Deep Sleep can be enabled by
//     <0=>Secure and Non-Secure state
//     <1=>Secure state only
//   <i> Value for SCB->CSR register bit DEEPSLEEPS
*/
#define SCB_CSR_DEEPSLEEPS_VAL  1

/*
//   <o>System reset request accessible from
//     <0=> Secure and Non-Secure state
//     <1=> Secure state only
//   <i> Value for SCB->AIRCR register bit SYSRESETREQS
*/
#define SCB_AIRCR_SYSRESETREQS_VAL  1

/*
//   <o>Priority of Non-Secure exceptions is
//     <0=> Not altered
//     <1=> Lowered to 0x80-0xFF
//   <i> Value for SCB->AIRCR register bit PRIS
*/
#define SCB_AIRCR_PRIS_VAL      1

/*
//   <o>BusFault, HardFault, and NMI target
//     <0=> Secure state
//     <1=> Non-Secure state
//   <i> Value for SCB->AIRCR register bit BFHFNMINS
//    Notice: HardFault only behaves as a banked exception if AIRCR.BFHFNMINS is 1, otherwise it behaves as an
//    unbanked exception targeting Secure state.
*/
#define SCB_AIRCR_BFHFNMINS_VAL 1

namespace {

const TZ_CFG_TypeDef tz_config[]= {
    //  Start                    End                NSC
    {0x40000000,            0x50000000-1,            0},  // entry0: Peripherals NS
    {0x1010A000,            0x101D4000-1,            0},  // entry1: IROM & DROM NS
    {0x100E0000,            0x10100000-1,            0},  // entry2: BT/WIFI Extention SRAM
    {0x00000000,            0x08060000-1,            0},  // entry3: KM0 ROM, KM0 SRAM, Retention SRAM, PSRAM, KM0 FLASH
    {0x08060000,            0x1007C000-1,            0},  // entry4: KM4 Flash, KM4 SRAM
    {0xFFFFFFFF,            0xFFFFFFFF,              0},  // entry5: None
    {0xFFFFFFFF,            0xFFFFFFFF,              0},  // entry6: None
    {0xFFFFFFFF,            0xFFFFFFFF,              0},  // entry7: None
};

__attribute__((always_inline)) inline
void trustzone_configuration(void) {
    IDAU_TypeDef* IDAU = ((IDAU_TypeDef *) KM4_IDAU_BASE);
    for (int index = 0; index < IDAU_ENTRYS_NUM; index++) {
        // Check if search to end
        if (tz_config[index].Start == 0xFFFFFFFF) {
            break;
        }

        // set IDAU, the enabled regions are treated as Non-secure space
        if (tz_config[index].NSC == 0) {
            IDAU->ENTRY[index].IDAU_BARx = tz_config[index].Start;
            IDAU->ENTRY[index].IDAU_LARx = tz_config[index].End;
            IDAU->IDAU_CTRL |= BIT(index);
        }
    }

    IDAU->IDAU_LOCK = 1;
    for (int index = 0; index < SAU_ENTRYS_NUM; index++) {
        // Check if search to end
        if (tz_config[index].Start == 0xFFFFFFFF) {
            break;
        }

        // set SAU
        SAU->RNR  =  (index & SAU_RNR_REGION_Msk); \
        SAU->RBAR =  (tz_config[index].Start & SAU_RBAR_BADDR_Msk); \
        SAU->RLAR =  (tz_config[index].End & SAU_RLAR_LADDR_Msk) | \
            ((tz_config[index].NSC << SAU_RLAR_NSC_Pos)  & SAU_RLAR_NSC_Msk)   | \
            ENABLE << SAU_RLAR_ENABLE_Pos;
    }
    SAU->CTRL = ((SAU_INIT_CTRL_ENABLE << SAU_CTRL_ENABLE_Pos) & SAU_CTRL_ENABLE_Msk) |
                ((SAU_INIT_CTRL_ALLNS  << SAU_CTRL_ALLNS_Pos)  & SAU_CTRL_ALLNS_Msk)   ;

    SCB->SCR = (SCB->SCR & ~(SCB_SCR_SLEEPDEEPS_Msk)) |
        ((SCB_CSR_DEEPSLEEPS_VAL << SCB_SCR_SLEEPDEEPS_Pos) & SCB_SCR_SLEEPDEEPS_Msk);

    SCB->AIRCR = (SCB->AIRCR &
        ~(SCB_AIRCR_VECTKEY_Msk   | SCB_AIRCR_SYSRESETREQS_Msk | SCB_AIRCR_BFHFNMINS_Msk |  SCB_AIRCR_PRIS_Msk)) |
        ((0x05FAU                    << SCB_AIRCR_VECTKEY_Pos)      & SCB_AIRCR_VECTKEY_Msk)      |
        //((SCB_AIRCR_SYSRESETREQS_VAL << SCB_AIRCR_SYSRESETREQS_Pos) & SCB_AIRCR_SYSRESETREQS_Msk) | /* reset both secure and non-secure */
        ((SCB_AIRCR_PRIS_VAL         << SCB_AIRCR_PRIS_Pos)         & SCB_AIRCR_PRIS_Msk)         |
        ((SCB_AIRCR_BFHFNMINS_VAL    << SCB_AIRCR_BFHFNMINS_Pos)    & SCB_AIRCR_BFHFNMINS_Msk);

    // <0=> Secure state <1=> Non-Secure state
    NVIC->ITNS[0] = 0xFFFFFFFF; // IRQ 0~31: Non-Secure state
    NVIC->ITNS[1] = 0x0003FFFF; // IRQ 32~49: Non-Secure state, 50~63
}

__attribute__((always_inline)) inline
void jump_to_nonsecure(u32 Addr) {
    __ASM volatile ("MOV r0, %0\n\t"
        "BLXNS   r0\n\t" : : "r" (Addr));
}

} // Anonymous

__attribute__((used, section(".secure.ram.text")))
void nonsecure_jump_to_system(uint32_t addr) {
    // FIXME: Disable MPU just in case for now
    mpu_disable();

    trustzone_configuration();

    // Enable SecureFault, UsageFault, MemManageFault, BusFault
    SCB->SHCSR |= SCB_SHCSR_SECUREFAULTENA_Msk | SCB_SHCSR_USGFAULTENA_Msk | \
        SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk;

    // Enable all access to FPU
    SCB->CPACR |= 0x00f00000;
    SCB_NS->CPACR |=  0x00f00000;
    SCB->NSACR |= BIT(10) | BIT(11); // enable non-secure to access VFP

    // Set non-secure main stack (MSP_NS)
    __TZ_set_MSP_NS(MSP_RAM_HP_NS);

    // Set PSPS Temp
    __set_PSP(MSP_RAM_HP_NS-2048);

    volatile uint32_t NonSecure_ResetHandler;
    NonSecure_ResetHandler = cmse_nsfptr_create(addr);

    // We found enabling cache in the system part1 will mess up RAM (e.g. LED configuration),
    // it is probably caused by sharing some hal drivers in bootloader and Device OS,
    // let's enable it in the bootloader
    Cache_Enable(1);
    Cache_Flush();
    __DSB();

    // Start non-secure state software application
    jump_to_nonsecure(NonSecure_ResetHandler);
}
