/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/*
 * Defines BSP-related configuration.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Constant to be used to define others */
#ifdef WICED_NO_WIFI
#define PLATFORM_WLAN_PRESENT             0
#else
#define PLATFORM_WLAN_PRESENT             1
#endif

/* Define which frequency CPU run */
#ifndef PLATFORM_CPU_CLOCK_FREQUENCY
#if defined(PLATFORM_4390X_OVERCLOCK)
#define PLATFORM_CPU_CLOCK_FREQUENCY      PLATFORM_CPU_CLOCK_FREQUENCY_480_MHZ
#else
#define PLATFORM_CPU_CLOCK_FREQUENCY      PLATFORM_CPU_CLOCK_FREQUENCY_320_MHZ
#endif /* PLATFORM_4390X_OVERCLOCK */
#endif

/* Common switch to be used to enable/disable various modules powersaving. */
#ifndef PLATFORM_POWERSAVE_DEFAULT
#define PLATFORM_POWERSAVE_DEFAULT        0
#endif

/* Define WLAN powersave feature en/dis */
#ifndef PLATFORM_WLAN_POWERSAVE
#define PLATFORM_WLAN_POWERSAVE           PLATFORM_WLAN_PRESENT
#endif

/* Define cores powersave feature en/dis */
#ifndef PLATFORM_CORES_POWERSAVE
#define PLATFORM_CORES_POWERSAVE          PLATFORM_POWERSAVE_DEFAULT
#endif

/* Define APPS domain powersave feature en/dis */
#ifndef PLATFORM_APPS_POWERSAVE
#define PLATFORM_APPS_POWERSAVE           PLATFORM_POWERSAVE_DEFAULT
#endif

/* Define ticks powersave feature en/dis */
#ifndef PLATFORM_TICK_POWERSAVE
#define PLATFORM_TICK_POWERSAVE           PLATFORM_APPS_POWERSAVE
#endif

/* Define initial state of MCU power-save enable/disable API */
#ifndef PLATFORM_MCU_POWERSAVE_INIT_STATE
#define PLATFORM_MCU_POWERSAVE_INIT_STATE 1
#endif

/* Define MCU powersave mode set during platform initialization */
#ifndef PLATFORM_MCU_POWERSAVE_MODE_INIT
#define PLATFORM_MCU_POWERSAVE_MODE_INIT  PLATFORM_MCU_POWERSAVE_MODE_SLEEP
#endif

/* Define tick powersave mode set during platform initialization */
#ifndef PLATFORM_TICK_POWERSAVE_MODE_INIT
#define PLATFORM_TICK_POWERSAVE_MODE_INIT PLATFORM_TICK_POWERSAVE_MODE_TICKLESS_IF_MCU_POWERSAVE_ENABLED
#endif

/* Define RTOS timers default configuration */
//
#ifndef PLATFORM_TICK_TINY
#if defined(BOOTLOADER) || defined(TINY_BOOTLOADER)
#define PLATFORM_TICK_TINY                1
#else
#define PLATFORM_TICK_TINY                0
#endif /* BOOTLODER || TINY_BOOTLOADER */
#endif /* PLATFORM_TICK_TINY */
//
#ifndef PLATFORM_TICK_CPU
#define PLATFORM_TICK_CPU                 !PLATFORM_TICK_TINY
#endif
//
#ifndef PLATFORM_TICK_PMU
#if PLATFORM_APPS_POWERSAVE
#define PLATFORM_TICK_PMU                 !PLATFORM_TICK_TINY
#else
#define PLATFORM_TICK_PMU                 0
#endif /* PLATFORM_APPS_POWERSAVE */
#endif /* PLATFORM_TICK_PMU */

/* By default enable ticks statistic */
#ifndef PLATFORM_TICK_STATS
#define PLATFORM_TICK_STATS               !PLATFORM_TICK_TINY
#endif

/* By default switch off WLAN powersave statistic */
#ifndef PLATFORM_WLAN_POWERSAVE_STATS
#define PLATFORM_WLAN_POWERSAVE_STATS     0
#endif

/* By default use external LPO */
#ifndef PLATFORM_LPO_CLOCK_EXT
#if defined(BOOTLOADER) || defined(TINY_BOOTLOADER)
#define PLATFORM_LPO_CLOCK_EXT            0
#else
#define PLATFORM_LPO_CLOCK_EXT            1
#endif /* BOOTLODER || TINY_BOOTLOADER */
#endif /* PLATFORM_LPO_CLOCK_EXT */

/* Define various HIB parameters */
// Disable various HIB parameters by one shot
#ifndef PLATFORM_HIB_NOT_AVAILABLE
#define PLATFORM_HIB_NOT_AVAILABLE        0
#endif
#if PLATFORM_HIB_NOT_AVAILABLE
#define PLATFORM_HIB_ENABLE               0
#define PLATFORM_HIB_CLOCK_AS_EXT_LPO     0
#define PLATFORM_HIB_CLOCK_POWER_UP       0
#endif /* PLATFORM_HIB_NOT_AVAILABLE */
// HIB provide clock to be used as external LPO, use it by default
#ifndef PLATFORM_HIB_CLOCK_AS_EXT_LPO
#if defined(BOOTLOADER) || defined(TINY_BOOTLOADER)
#define PLATFORM_HIB_CLOCK_AS_EXT_LPO     0
#else
#define PLATFORM_HIB_CLOCK_AS_EXT_LPO     1
#endif /* BOOTLODER || TINY_BOOTLOADER */
#endif /* PLATFORM_HIB_CLOCK_AS_EXT_LPO */
// Power-up clocks in bootloader
#ifndef PLATFORM_HIB_CLOCK_POWER_UP
#if defined(BOOTLOADER) || defined(TINY_BOOTLOADER)
#define PLATFORM_HIB_CLOCK_POWER_UP       1
#else
#define PLATFORM_HIB_CLOCK_POWER_UP       0
#endif /* BOOTLODER || TINY_BOOTLOADER */
#endif /* PLATFORM_HIB_CLOCK_POWER_UP */
//
#ifndef PLATFORM_HIB_ENABLE
#if defined(BOOTLOADER) || defined(TINY_BOOTLOADER)
#define PLATFORM_HIB_ENABLE               0
#else
#define PLATFORM_HIB_ENABLE               1
#endif /* BOOTLOADER || TINY_BOOTLOADER */
#endif /* PLATFORM_HIB_ENABLE */

/* Define DDR default configuration */
#ifndef PLATFORM_DDR_HEAP_SIZE_CONFIG
#define PLATFORM_DDR_HEAP_SIZE_CONFIG     0 /* can be defined per-application */
#endif
//
#ifndef PLATFORM_DDR_CODE_AND_DATA_ENABLE
#define PLATFORM_DDR_CODE_AND_DATA_ENABLE 0 /* can be defined per-platform as it require to specifically build application and bootloader same time */
#endif
//
#if PLATFORM_DDR_CODE_AND_DATA_ENABLE
#ifdef PLATFORM_NO_DDR
#if PLATFORM_NO_DDR != 0
#error "Misconfiguration"
#endif /* PLATFORM_NO_DDR */
#else
#define PLATFORM_NO_DDR                   0
#endif /* PLATFORM_NO_DDR */
#endif /* PLATFORM_DDR_CODE_AND_DATA_ENABLE */
//
#if PLATFORM_DDR_CODE_AND_DATA_ENABLE
#ifdef BOOTLOADER
#define PLATFORM_DDR_SKIP_INIT            0
#else
#define PLATFORM_DDR_SKIP_INIT            1
#endif /* BOOTLOADER */
#else
#define PLATFORM_DDR_SKIP_INIT            0
#endif /* PLATFORM_DDR_CODE_AND_DATA_ENABLE */
//
#ifndef PLATFORM_NO_DDR
#if defined(BOOTLOADER) || defined(TINY_BOOTLOADER)
#define PLATFORM_NO_DDR                   1
#else
#define PLATFORM_NO_DDR                   0
#endif /* BOOTLODER || TINY_BOOTLOADER */
#endif /* PLATFORM_NO_DDR */

/* Define SPI flash default configuration */
#ifndef PLATFORM_NO_SFLASH_WRITE
#if defined(BOOTLOADER) || defined(TINY_BOOTLOADER)
#if defined(PLATFORM_HAS_OTA) || defined(OTA2_SUPPORT)
#define PLATFORM_NO_SFLASH_WRITE          0
#else
#define PLATFORM_NO_SFLASH_WRITE          1
#endif /* OTA2_SUPPORT           */
#else
#define PLATFORM_NO_SFLASH_WRITE          0
#endif /* BOOTLODER || TINY_BOOTLOADER */
#endif /* PLATFORM_NO_SFLASH_WRITE */

/* Define backplane configuration */
#ifndef PLATFORM_NO_BP_INIT
#if defined(BOOTLOADER) || defined(TINY_BOOTLOADER)
#define PLATFORM_NO_BP_INIT               1
#else
#define PLATFORM_NO_BP_INIT               0
#ifndef PLATFORM_BP_TIMEOUT
#define PLATFORM_BP_TIMEOUT               0xFF
#endif /* PLATFORM_BP_TIMEOUT */
#endif /* BOOTLODER || TINY_BOOTLOADER */
#endif /* PLATFORM_NO_BP_INIT */

 /* TraceX storage buffer in DDR */
#if defined(TX_ENABLE_EVENT_TRACE) && (PLATFORM_NO_DDR != 1)
#ifndef WICED_TRACEX_BUFFER_DDR_OFFSET
#define WICED_TRACEX_BUFFER_DDR_OFFSET    (0x0)
#endif
#define WICED_TRACEX_BUFFER_ADDRESS       ((uint8_t *)PLATFORM_DDR_BASE(WICED_TRACEX_BUFFER_DDR_OFFSET))
#endif

/* Define that platform require some fixup to use ALP clock. No need for chips starting from B1. */
#ifndef PLATFORM_ALP_CLOCK_RES_FIXUP
#define PLATFORM_ALP_CLOCK_RES_FIXUP      PLATFORM_WLAN_PRESENT
#endif

/* Define platform USB require some fixup to use ALP clock. Necessary for A0/B0/B1 chips. */
#ifndef PLATFORM_USB_ALP_CLOCK_RES_FIXUP
#define PLATFORM_USB_ALP_CLOCK_RES_FIXUP  PLATFORM_WLAN_PRESENT
#endif

/* Define that platform need WLAN assistance to wake-up */
#ifndef PLATFORM_WLAN_ASSISTED_WAKEUP
#define PLATFORM_WLAN_ASSISTED_WAKEUP     PLATFORM_WLAN_PRESENT
#endif

/* Define that by default platform no need extra hook */
#ifndef PLATFORM_IRQ_DEMUXER_HOOK
#define PLATFORM_IRQ_DEMUXER_HOOK         0
#endif

#ifndef PLATFORM_LPLDO_VOLTAGE
#define PLATFORM_LPLDO_VOLTAGE            PMU_REGULATOR_LPLDO1_0_9_V
#endif

#ifndef PLATFORM_NO_SDIO
#define PLATFORM_NO_SDIO                  0
#endif

#ifndef PLATFORM_NO_GMAC
#define PLATFORM_NO_GMAC                  0
#endif

#ifndef PLATFORM_NO_USB
#define PLATFORM_NO_USB                   0
#endif

#ifndef PLATFORM_NO_HSIC
#define PLATFORM_NO_HSIC                  0
#endif

#ifndef PLATFORM_NO_I2S
#define PLATFORM_NO_I2S                   0
#endif

#ifndef PLATFORM_NO_I2C
#define PLATFORM_NO_I2C                   0
#endif

#ifndef PLATFORM_NO_UART
#define PLATFORM_NO_UART                  0
#endif

#ifndef PLATFORM_NO_DDR
#define PLATFORM_NO_DDR                   0
#endif

#ifndef PLATFORM_NO_SPI
#define PLATFORM_NO_SPI                   0
#endif

#ifndef PLATFORM_NO_JTAG
#define PLATFORM_NO_JTAG                  0
#endif

#ifndef PLATFORM_NO_PWM
#define PLATFORM_NO_PWM                   0
#endif

#ifndef PLATFORM_NO_SOCSRAM_POWERDOWN
#define PLATFORM_NO_SOCSRAM_POWERDOWN     0
#endif

#ifndef PLATFORM_HIB_WAKE_CTRL_REG_RCCODE
#define PLATFORM_HIB_WAKE_CTRL_REG_RCCODE -1
#endif

#ifndef __ASSEMBLER__
#include "platform_isr.h"
#include "platform_map.h"
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
