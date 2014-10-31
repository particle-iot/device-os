/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 * Defines internal configuration of the BCM943362WCD4 board
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************
 *  MCU Constants and Options
 *
 *  NOTE: The clock configuration utility from ST is used to calculate these values
 *        http://www.st.com/st-web-ui/static/active/en/st_prod_software_internet/resource/technical/software/utility/stsw-stm32090.zip
 ******************************************************/

/*  CPU clock : 120 MHz */
#define CPU_CLOCK_HZ         ( 120000000 )

/*  Use external crystal */
#define HSE_SOURCE           ( RCC_HSE_ON )

/*  AHB clock  : System clock */
#define AHB_CLOCK_DIVIDER    ( RCC_SYSCLK_Div1 )

/*  APB1 clock : AHB clock / 4 */
#define APB1_CLOCK_DIVIDER   ( RCC_HCLK_Div4 )

/*  APB2 clock : AHB clock / 2 */
#define APB2_CLOCK_DIVIDER   ( RCC_HCLK_Div2 )

/*  PLL source : external crystal */
#define PLL_SOURCE           ( RCC_PLLSource_HSE )

/*  PLLM : 26 */
#define PLL_M_CONSTANT       ( 26 )

/*  PLLN : 240 */
#define PLL_N_CONSTANT       ( 240 )

/*  PLLP : 2 */
#define PLL_P_CONSTANT       ( 2 )

/*  PLLQ : 5 */
#define PPL_Q_CONSTANT       ( 5 )

/*  System clock source  : PLL clock */
#define SYSTEM_CLOCK_SOURCE  ( RCC_SYSCLKSource_PLLCLK )

/*  SysTick clock source : AHB clock  */
#define SYSTICK_CLOCK_SOURCE ( SysTick_CLKSource_HCLK )

/*  Internal flash wait state : 3 cycles */
#define INT_FLASH_WAIT_STATE ( FLASH_Latency_3 )

/*  Internal flash voltage range : 2.7 to 3.6V */
#define PLATFORM_STM32_VOLTAGE_2V7_TO_3V6


/******************************************************
 *  Wi-Fi Options
 ******************************************************/

/*  GPIO pins are used to bootstrap Wi-Fi to SDIO or gSPI mode */
#define WICED_WIFI_USE_GPIO_FOR_BOOTSTRAP

/*  Wi-Fi GPIO0 pin is used for out-of-band interrupt */
#define WICED_WIFI_OOB_IRQ_GPIO_PIN  ( 0 )

/*  Wi-Fi power pin is present */
#define WICED_USE_WIFI_POWER_PIN

/*  Wi-Fi reset pin is present */
#define WICED_USE_WIFI_RESET_PIN

/*  WLAN Powersave Clock Source
 *  The WLAN sleep clock can be driven from one of two sources:
 *  1. MCO (MCU Clock Output) - default
 *     NOTE: Versions of BCM943362WCD4 up to and including P200 require a hardware patch to enable this mode
 *     - Connect STM32F205RGT6 pin 41 (PA8) to pin 44 (PA11)
 *  2. WLAN 32K internal oscillator (30% inaccuracy)
 *     - Comment the following directive : WICED_USE_WIFI_32K_CLOCK_MCO
 */
#define WICED_USE_WIFI_32K_CLOCK_MCO


#ifdef __cplusplus
} /* extern "C" */
#endif
