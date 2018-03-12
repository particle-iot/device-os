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

/** @file
 *  Defines the Non-Bus part of the WICED Platform Interface.
 *
 *  Provides prototypes for functions that allow WICED to use
 *  the hardware platform.
 */

#include "wwd_structures.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** @addtogroup platif Platform Interface
 * Enables WICED to use the hardware platform.
 *  @{
 */

/** @name General Platform Functions
 *  Functions that enable WICED to use the hardware platform.
 */
/**@{*/

/******************************************************
 *             Function declarations
 ******************************************************/


/**
 * Switches the Reset signal to the Broadcom Wi-Fi
 *
 * WICED uses this function to switch the reset
 * signal to the Broadcom Wi-Fi chip. It should
 * toggle the appropriate GPIO pins.
 *
 * @param reset_asserted: WICED_TRUE  = reset asserted
 *                        WICED_FALSE = reset de-asserted
 */
/*@external@*/ extern void host_platform_reset_wifi( wiced_bool_t reset_asserted );

/**
 * Switches the Power signal to the Broadcom Wi-Fi
 *
 * WICED uses this function to switch the
 * power signal to the Broadcom Wi-Fi chip. It should
 * toggle the appropriate GPIO pins.
 *
 * @param power_enabled : WICED_TRUE  = power up
 *                        WICED_FALSE = power down
 */
/*@external@*/ extern void host_platform_power_wifi( wiced_bool_t power_enabled );

/**
 * Initialises platform for WICED
 *
 * Sets up parts of the hardware platform which are not
 * directly part of the bus. These may include: Reset Pin
 * and Regulator-On Pin.
 *
 * @return WWD_SUCCESS or Error code
 */
/*@external@*/ extern wwd_result_t host_platform_init( void );

/**
 * De-Initialises platform for WICED
 *
 * Does the opposite to the @ref host_platform_init function,
 * leaving the Broadcom Wi-Fi in a powered down, reset
 * state.
 *
 * @return WWD_SUCCESS or Error code
 */
/*@external@*/ extern wwd_result_t host_platform_deinit( void );


/**
 * Disable power saving modes that would stop clocks
 */
/*@external@*/ extern void host_platform_clocks_needed( void );

/**
 * Enable power saving modes that would stop clocks
 */
/*@external@*/ extern void host_platform_clocks_not_needed( void );

/**
 * The host MCU can provide a MAC address to the Wi-Fi driver (rather than
 * the driver using the MAC in NVRAM or OTP). To use this functionality,
 * add a global define (MAC_ADDRESS_SET_BY_HOST) in the application makefile.
 * Further information is available in the generated_mac_address.txt file
 * that is created during the application build process.
 * @param mac : A wiced_mac_t pointer to the Wi-Fi MAC address
 */
/*@external@*/ extern wwd_result_t host_platform_get_mac_address( wiced_mac_t* mac );

/**
 * Returns Ethernet MAC address
 *
 * @param mac : A wiced_mac_t pointer to the Ethernet MAC address
 */
/*@external@*/ extern wwd_result_t host_platform_get_ethernet_mac_address( wiced_mac_t* mac );

/**
 * Returns the current CPU cycle count.
 *
 * This function is used to accurately calculate sub-systick timing.
 */
/*@external@*/ extern uint32_t host_platform_get_cycle_count( void );

/**
 * Returns TRUE if the CPU is currently running in interrupt context
 *
 */
/*@external@*/ extern wiced_bool_t host_platform_is_in_interrupt_context( void );

extern wwd_result_t host_platform_init_wlan_powersave_clock( void );

extern wwd_result_t host_platform_deinit_wlan_powersave_clock( void );

/** @} */
/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif
