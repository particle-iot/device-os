/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 * Defines BCM43909 mapping
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ASSEMBLER__
#include "stdint.h"
#endif /* ifndef __ASSEMBLER__ */

/****************************************************************************************************************
 *                      Macros
 ****************************************************************************************************************/

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)     (sizeof(a) / sizeof(a[0]))
#endif

/****************************************************************************************************************
 *                    Constants
 ****************************************************************************************************************/
#define PLATFORM_WLAN_RAM_SIZE                                  (0x90000)
#define PLATFORM_SOCSRAM_AON_RAM_SIZE                           (8 * 1024)

#define PLATFORM_ATCM_BASE(offset)                              (0x0 + (offset))
#define PLATFORM_ATCM_ROM_BASE(offset)                          PLATFORM_ATCM_BASE(0x0      + (offset))
#define PLATFORM_ATCM_RAM_BASE(offset)                          PLATFORM_ATCM_BASE(0x1B0000 + (offset))

#define PLATFORM_SOCSRAM_CH0_BASE(offset)                       (0x400000 + (offset))
#define PLATFORM_SOCSRAM_CH0_ROM_BASE(offset)                   PLATFORM_SOCSRAM_CH0_BASE(0x0      + (offset))
#define PLATFORM_SOCSRAM_CH0_RAM_BASE(offset)                   PLATFORM_SOCSRAM_CH0_BASE(0xA0000  + (offset))
#define PLATFORM_SOCSRAM_CH0_AON_RAM_BASE(offset)               PLATFORM_SOCSRAM_CH0_BASE(0x2A0000 + (offset))

#define PLATFORM_SOCSRAM_CH1_BASE(offset)                       (0x800000 + (offset))
#define PLATFORM_SOCSRAM_CH1_ROM_BASE(offset)                   PLATFORM_SOCSRAM_CH1_BASE(0x0      + (offset))
#define PLATFORM_SOCSRAM_CH1_RAM_BASE(offset)                   PLATFORM_SOCSRAM_CH1_BASE(0xA0000  + (offset))
#define PLATFORM_SOCSRAM_CH1_AON_RAM_BASE(offset)               PLATFORM_SOCSRAM_CH1_BASE(0x2A0000 + (offset))

#define PLATFORM_SOCSRAM_MAX_SIZE                               (0x2A2000)

#define PLATFORM_DDR_BASE(offset)                               (0x40000000 + (offset))
#define PLATFORM_DDR_MAX_SIZE                                   (128 * 1024 * 1024)

#define PLATFORM_FLASH_BASE(offset)                             (0x14000000 + (offset))

#define PLATFORM_REGBASE(offset)                                (0x18000000 + (offset))
#define PLATFORM_CHIPCOMMON_REGBASE(offset)                     (PLATFORM_REGBASE(0x000000) + (offset))
#define PLATFORM_I2S0_REGBASE(offset)                           (PLATFORM_REGBASE(0x001000) + (offset))
#define PLATFORM_I2S1_REGBASE(offset)                           (PLATFORM_REGBASE(0x002000) + (offset))
#define PLATFORM_APPSCR4_REGBASE(offset)                        (PLATFORM_REGBASE(0x003000) + (offset))
#define PLATFORM_M2M_REGBASE(offset)                            (PLATFORM_REGBASE(0x004000) + (offset))
#define PLATFORM_GMAC_REGBASE(offset)                           (PLATFORM_REGBASE(0x005000) + (offset))
#define PLATFORM_SDIOH_REGBASE(offset)                          (PLATFORM_REGBASE(0x008000) + (offset))
#define PLATFORM_EHCI_REGBASE(offset)                           (PLATFORM_REGBASE(0x006000) + (offset))
#define PLATFORM_USB20D_REGBASE(offset)                         (PLATFORM_REGBASE(0x007000) + (offset))
#define PLATFORM_CRYPTO_REGBASE(offset)                         (PLATFORM_REGBASE(0x00A000) + (offset))
#define PLATFORM_DDR_CONTROLLER_REGBASE(offset)                 (PLATFORM_REGBASE(0x00B000) + (offset))
#define PLATFORM_SOCSRAM_CONTROLLER_REGBASE(offset)             (PLATFORM_REGBASE(0x00C000) + (offset))
#define PLATFORM_OHCI_REGBASE(offset)                           (PLATFORM_REGBASE(0x00D000) + (offset))
#define PLATFORM_WLANCR4_REGBASE(offset)                        (PLATFORM_REGBASE(0x011000) + (offset))
#define PLATFORM_PMU_REGBASE(offset)                            (PLATFORM_REGBASE(0x020000) + (offset))
#define PLATFORM_GCI_COREBASE(offset)                           (PLATFORM_REGBASE(0x022000) + (offset))
#define PLATFORM_GCI_REGBASE(offset)                            (PLATFORM_GCI_COREBASE(0x000C00) + (offset))

#define PLATFORM_CHIPCOMMON_MASTER_WRAPPER_REGBASE(offset)      (PLATFORM_REGBASE(0x100000) + (offset))
#define PLATFORM_I2S0_MASTER_WRAPPER_REGBASE(offset)            (PLATFORM_REGBASE(0x101000) + (offset))
#define PLATFORM_I2S1_MASTER_WRAPPER_REGBASE(offset)            (PLATFORM_REGBASE(0x102000) + (offset))
#define PLATFORM_APPSCR4_MASTER_WRAPPER_REGBASE(offset)         (PLATFORM_REGBASE(0x103000) + (offset))
#define PLATFORM_M2M_MASTER_WRAPPER_REGBASE(offset)             (PLATFORM_REGBASE(0x104000) + (offset))
#define PLATFORM_GMAC_MASTER_WRAPPER_REGBASE(offset)            (PLATFORM_REGBASE(0x105000) + (offset))
#define PLATFORM_USB20H_MASTER_WRAPPER_REGBASE(offset)          (PLATFORM_REGBASE(0x106000) + (offset))
#define PLATFORM_USB20D_MASTER_WRAPPER_REGBASE(offset)          (PLATFORM_REGBASE(0x107000) + (offset))
#define PLATFORM_SDIOH_MASTER_WRAPPER_REGBASE(offset)           (PLATFORM_REGBASE(0x108000) + (offset))
#define PLATFORM_SDIOD_MASTER_WRAPPER_REGBASE(offset)           (PLATFORM_REGBASE(0x109000) + (offset))
#define PLATFORM_CRYPTO_MASTER_WRAPPER_REGBASE(offset)          (PLATFORM_REGBASE(0x10A000) + (offset))
#define PLATFORM_DDR_CONTROLLER_SLAVE_WRAPPER_REGBASE(offset)   (PLATFORM_REGBASE(0x10B000) + (offset))
#define PLATFORM_SOCSRAM_CH0_SLAVE_WRAPPER_REGBASE(offset)      (PLATFORM_REGBASE(0x10C000) + (offset))
#define PLATFORM_SOCSRAM_CH1_SLAVE_WRAPPER_REGBASE(offset)      (PLATFORM_REGBASE(0x10D000) + (offset))
#define PLATFORM_APPSCR4_SLAVE_WRAPPER_REGBASE(offset)          (PLATFORM_REGBASE(0x10E000) + (offset))
#define PLATFORM_CHIPCOMMON_SLAVE_WRAPPER_REGBASE(offset)       (PLATFORM_REGBASE(0x10F000) + (offset))
#define PLATFORM_DOT11MAC_MASTER_WRAPPER_REGBASE(offset)        (PLATFORM_REGBASE(0x110000) + (offset))
#define PLATFORM_WLANCR4_MASTER_WRAPPER_REGBASE(offset)         (PLATFORM_REGBASE(0x111000) + (offset))
#define PLATFORM_WLANCR4_SLAVE_WRAPPER_REGBASE(offset)          (PLATFORM_REGBASE(0x112000) + (offset))
#define PLATFORM_WLAN_APB0_SLAVE_WRAPPER_REGBASE(offset)        (PLATFORM_REGBASE(0x113000) + (offset))
#define PLATFORM_WLAN_DEFAULT_SLAVE_WRAPPER_REGBASE(offset)     (PLATFORM_REGBASE(0x114000) + (offset))
#define PLATFORM_AON_APB0_SLAVE_WRAPPER_REGBASE(offset)         (PLATFORM_REGBASE(0x120000) + (offset))
#define PLATFORM_APPS_APB0_SLAVE_WRAPPER_REGBASE(offset)        (PLATFORM_REGBASE(0x130000) + (offset))
#define PLATFORM_APPS_DEFAULT_SLAVE_WRAPPER_REGBASE(offset)     (PLATFORM_REGBASE(0x131000) + (offset))

#define PLATFORM_SOCSRAM_CH1_SLAVE_WRAPPER_RESET_CTRL           PLATFORM_SOCSRAM_CH1_SLAVE_WRAPPER_REGBASE(0x800)

/****************************************************************************************************************
 *                    Functions
 ****************************************************************************************************************/

#ifndef __ASSEMBLER__
static inline void* platform_addr_cached_to_uncached( const void* cached )
{
    /* First channel SOCSRAM addresses are mapped as cached via MPU, and second as uncached. */
    const uint32_t cached_addr = (uint32_t)cached;
    const uint32_t uncached_addr = cached_addr - PLATFORM_SOCSRAM_CH0_BASE(0x0) + PLATFORM_SOCSRAM_CH1_BASE(0x0);
    return (void*)uncached_addr;
}

static inline void* platform_addr_uncached_to_cached( const void* uncached )
{
    /* First channel SOCSRAM addresses are mapped as cached via MPU, and second as uncached. */
    const uint32_t uncached_addr = (uint32_t)uncached;
    const uint32_t cached_addr = uncached_addr - PLATFORM_SOCSRAM_CH1_BASE(0x0) + PLATFORM_SOCSRAM_CH0_BASE(0x0);
    return (void*)cached_addr;
}

static inline uint32_t platform_addr_cpu_to_dma( const void* cpu_addr )
{
    /* For performance optimization let CPU mainly use first SOCSRAM channel, and DMA operations use second channel. */
    uint32_t dma_addr = (uint32_t)cpu_addr;
    if ((dma_addr >= PLATFORM_SOCSRAM_CH0_BASE(0x0)) && (dma_addr < PLATFORM_SOCSRAM_CH0_BASE(0x0) + PLATFORM_SOCSRAM_MAX_SIZE))
    {
        dma_addr = dma_addr - PLATFORM_SOCSRAM_CH0_BASE(0x0) + PLATFORM_SOCSRAM_CH1_BASE(0x0);
    }
    return dma_addr;
}

static inline int platform_addr_is_uncached_region( const void* memory )
{
    /* The second channel SOCSRAM addresses are mapped as uncached. Check if given memory address is in this region. */
    const uint32_t mem_addr = (uint32_t)memory;
    if ((mem_addr >= PLATFORM_SOCSRAM_CH1_BASE(0x0)) && (mem_addr < (PLATFORM_SOCSRAM_CH1_BASE(0x0) + PLATFORM_SOCSRAM_MAX_SIZE)))
    {
        return 1;
    }
    return 0;
}
#endif /* __ASSEMBLER__ */

#ifdef __cplusplus
} /*extern "C" */
#endif
