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
 *  Provides prototypes for initialization and other management functions for Wiced system
 *
 */

#ifndef INCLUDED_WWD_MANAGEMENT_H
#define INCLUDED_WWD_MANAGEMENT_H

#include "wwd_constants.h"  /* for wwd_result_t and country codes */
#ifdef __cplusplus
extern "C"
{
#endif

/** @addtogroup mgmt WICED Management
 *  User functions for initialization and other management functions for the WICED system
 *  @{
 */

/******************************************************
 *             Function declarations
 ******************************************************/
/*@-exportlocal@*/

/**
 * Initialise Wi-Fi platform
 *
 * - Initialises the required parts of the hardware platform
 *   i.e. pins for SDIO/SPI, interrupt, reset, power etc.
 *
 * - Initialises the Wiced thread which arbitrates access
 *   to the SDIO/SPI bus
 *
 * @return WWD_SUCCESS if initialization is successful, Error code otherwise
 */
wwd_result_t wwd_management_wifi_platform_init( wiced_country_code_t country, wiced_bool_t resume_after_deep_sleep );

/*
 * WARNING: After setting to WICED_TRUE,
 * Call wwd_management_wifi_platform_init_halt( WICED_FALSE )
 * prior to next wwd_management_wifi_platform_init.
 * Halt Wi-Fi platform init by causing abort of firmware download loop.
 *
 * @return WWD_SUCCESS if successfully set the flag to abort
 */
wwd_result_t wwd_management_wifi_platform_init_halt( wiced_bool_t halt );

/**
 * Turn on the Wi-Fi device
 *
 * - Initialise Wi-Fi platform
 *
 * - Program various WiFi parameters and modes
 *
 * @return WWD_SUCCESS if initialization is successful, error code otherwise
 */
wwd_result_t wwd_management_wifi_on( wiced_country_code_t country );

/**
 * Get WLAN powersave sleep clock enabled function
 *
 * @return WICED_TRUE if powersave sleep clock is enabled, WICED_FALSE if powersave sleep clock disabled
 */
wiced_bool_t wwd_get_wlan_sleep_clock_enabled( void );

/**
 * Set WLAN powersave sleep clock enabled function
 *
 * @param enabled : enables/disables the powersave sleep clock
 */
wwd_result_t wwd_set_wlan_sleep_clock_enabled(wiced_bool_t enabled);

/**
 * Turn off the Wi-Fi device
 *
 * - De-Initialises the required parts of the hardware platform
 *   i.e. pins for SDIO/SPI, interrupt, reset, power etc.
 *
 * - De-Initialises the Wiced thread which arbitrates access
 *   to the SDIO/SPI bus
 *
 * @return WWD_SUCCESS if deinitialization is successful, Error code otherwise
 */
wwd_result_t wwd_management_wifi_off( void );

/*@+exportlocal@*/

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* ifndef INCLUDED_WWD_MANAGEMENT_H */
