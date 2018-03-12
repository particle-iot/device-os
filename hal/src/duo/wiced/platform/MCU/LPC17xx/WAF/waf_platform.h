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

/*
 * Sector Sizes for LPC17xx
 *
 * Sector  Size   Start      End
 * Num     KByte  Address    Address
 * ------------------------------------
 * 0       4      0X00000000 0X00000FFF
 * 1       4      0X00001000 0X00001FFF
 * 2       4      0X00002000 0X00002FFF
 * 3       4      0X00003000 0X00003FFF
 * 4       4      0X00004000 0X00004FFF
 * 5       4      0X00005000 0X00005FFF
 * 6       4      0X00006000 0X00006FFF
 * 7       4      0X00007000 0X00007FFF
 * 8       4      0x00008000 0X00008FFF
 * 9       4      0x00009000 0X00009FFF
 * 10      4      0x0000A000 0X0000AFFF
 * 11      4      0x0000B000 0X0000BFFF
 * 12      4      0x0000C000 0X0000CFFF
 * 13      4      0x0000D000 0X0000DFFF
 * 14      4      0x0000E000 0X0000EFFF
 * 15      4      0x0000F000 0X0000FFFF
 * 16      32     0x00010000 0x00017FFF
 * 17      32     0x00018000 0x0001FFFF
 * 18      32     0x00020000 0x00027FFF
 * 19      32     0x00028000 0x0002FFFF
 * 20      32     0x00030000 0x00037FFF
 * 21      32     0x00038000 0x0003FFFF
 * 22      32     0x00040000 0x00047FFF
 * 23      32     0x00048000 0x0004FFFF
 * 24      32     0x00050000 0x00057FFF
 * 25      32     0x00058000 0x0005FFFF
 * 26      32     0x00060000 0x00067FFF
 * 27      32     0x00068000 0x0006FFFF
 * 28      32     0x00070000 0x00077FFF
 * 29      32     0x00078000 0x0007FFFF
 */


#define DCT1_START_ADDR  ((uint32_t)&dct1_start_addr_loc)
#define DCT1_SIZE        ((uint32_t)&dct1_size_loc)
#define DCT2_START_ADDR  ((uint32_t)&dct2_start_addr_loc)
#define DCT2_SIZE        ((uint32_t)&dct2_size_loc)

#define PLATFORM_DCT_COPY1_START_SECTOR      ( 4  )
#define PLATFORM_DCT_COPY1_START_ADDRESS     ( DCT1_START_ADDR )
#define PLATFORM_DCT_COPY1_END_SECTOR        ( 7 )
#define PLATFORM_DCT_COPY1_END_ADDRESS       ( DCT1_START_ADDR + DCT1_SIZE )
#define PLATFORM_DCT_COPY2_START_SECTOR      ( 8  )
#define PLATFORM_DCT_COPY2_START_ADDRESS     ( DCT2_START_ADDR )
#define PLATFORM_DCT_COPY2_END_SECTOR        ( 11 )
#define PLATFORM_DCT_COPY2_END_ADDRESS       ( DCT1_START_ADDR + DCT1_SIZE )

/* DEFAULT APPS (eg FR and OTA) need only be loaded once. */
#define PLATFORM_DEFAULT_LOAD                ( WICED_FRAMEWORK_LOAD_ONCE )




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
/* These come from the linker script */
extern void* dct1_start_addr_loc;
extern void* dct1_size_loc;
extern void* dct2_start_addr_loc;
extern void* dct2_size_loc;

/******************************************************
 *               Function Declarations
 ******************************************************/

/* WAF platform functions */
void platform_start_app         ( uint32_t entry_point );
void platform_load_app_chunk    ( const image_location_t* app_header_location, uint32_t offset, void* physical_address, uint32_t size );
void platform_erase_app_area    ( uint32_t physical_address, uint32_t size );
platform_result_t platform_erase_flash       ( uint16_t start_sector, uint16_t end_sector );
platform_result_t platform_write_flash_chunk ( uint32_t address, const void* data, uint32_t size );

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

