/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                   Constants
 ******************************************************/

/******************************************************
 *                    Macros
 ******************************************************/
#define PLATFORM_SFLASH_PERIPHERAL_ID (0)

#define PLATFORM_DCT_COPY1_START_SECTOR      ( 0 )
#define PLATFORM_DCT_COPY1_START_ADDRESS     ( 0 )
#define PLATFORM_DCT_COPY1_END_SECTOR        ( 4 )
#define PLATFORM_DCT_COPY1_SIZE              ( 16 * 1024 )
#define PLATFORM_DCT_COPY1_END_ADDRESS       ( PLATFORM_DCT_COPY1_START_ADDRESS + PLATFORM_DCT_COPY1_SIZE )
#define PLATFORM_DCT_COPY2_START_SECTOR      ( PLATFORM_DCT_COPY1_END_SECTOR  )
#define PLATFORM_DCT_COPY2_START_ADDRESS     ( PLATFORM_DCT_COPY1_SIZE )
#define PLATFORM_DCT_COPY2_END_SECTOR        ( PLATFORM_DCT_COPY1_END_SECTOR * 2  )
#define PLATFORM_DCT_COPY2_SIZE              ( PLATFORM_DCT_COPY1_SIZE )
#define PLATFORM_DCT_COPY2_END_ADDRESS       ( PLATFORM_DCT_COPY2_START_ADDRESS + PLATFORM_DCT_COPY2_SIZE )

/* DEFAULT APPS (eg FR and OTA) need to be loaded always. */
#define PLATFORM_DEFAULT_LOAD                ( WICED_FRAMEWORK_LOAD_ALWAYS )

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/* WAF platform functions */
void platform_start_app         ( uint32_t vector_table_address );
void platform_load_app_chunk    ( const image_location_t *app_header_location, uint32_t offset, void * physical_address, uint32_t size);
void platform_erase_app_area    ( uint32_t physical_address, uint32_t size );

/* Check length of time the "Factory Reset" button is pressed
 *
 * NOTES: This is used for bootloader (PLATFORM_HAS_OTA) and ota2_bootloader.
 *        You must at least call NoOS_setup_timing(); before calling this.
 *
 *        To change the button used for this test on your platform, change
 *           platforms/<platform>/platform.h: PLATFORM_FACTORY_RESET_BUTTON_GPIO
 *
 *        To change the granularity of the timer (default 100us), change
 *           platforms/<platform>/platform.h: PLATFORM_FACTORY_RESET_CHECK_PERIOD
 *           NOTE: This also changes the "flashing" frequency of the LED)
 *
 *        To change the polarity of the button (default is pressed = low), change
 *           platforms/<platform>/platform.h: PLATFORM_WICED_BUTTON_ACTIVE_VALUE
 *           also, look at the gpio pin initialization below
 *           platform_gpio_init( &platform_gpio_pins[ PLATFORM_FACTORY_RESET_BUTTON_GPIO ], INPUT_PULL_UP );
 *
 *        To change the LED used for this purpose, change
 *           platforms/<platform>/platform.h: PLATFORM_FACTORY_RESET_LED_GPIO
 *
 *        To change the polarity of the LED (default is lit = high), change
 *           platforms/<platform>/platform.h: PLATFORM_FACTORY_RESET_LED_ON_STATE
 *
 * USAGE for OTA support (see <Wiced-SDK>/WICED/platform/MCU/wiced_waf_common.c::wiced_waf_check_factory_reset() )
 *            > 5 seconds - initiate Factory Reset
 *
 *      uint32_t    usecs_pressed;
 *      usecs_pressed = platform_get_factory_reset_button_time (PLATFORM_FACTORY_RESET_TIMEOUT);
 *      if (usecs_pressed >= PLATFORM_FACTORY_RESET_TIMEOUT)
 *      {
 *          //Factory Reset here
 *      }
 *
 * USAGE for OTA2 support (Over The Air version 2 see <Wiced-SDK>/apps/waf/ota2_bootloader/ota2_bootloader.c). (Over The Air version 2).
 *             ~5 seconds - start SoftAP
 *            ~10 seconds - initiate Factory Reset
 *
 *      uint32_t    usecs_pressed;
 *      usecs_pressed = platform_get_factory_reset_button_time (2 * PLATFORM_FACTORY_RESET_TIMEOUT);
 *      if ((usecs_pressed >= 4000) & (usecs_pressed  <= 6000))
 *      {
 *          //about 5 seconds here
 *      }
 *      else if ((usecs_pressed >= 9000) & (usecs_pressed  <= 11000))
 *      {
 *          //about 10 seconds here
 *      }
 *
 * param    max_time    - maximum time to wait
 *
 * returns  usecs button was held
 *
 */
uint32_t  platform_get_factory_reset_button_time ( uint32_t max_time );

#ifdef __cplusplus
} /* extern "C" */
#endif

