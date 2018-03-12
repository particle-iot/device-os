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
 *  Defines functions to manage the WICED system
 */

#pragma once

#include "network/wwd_network_interface.h"
#include "wiced_tcpip.h"
#include "wiced_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                     Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/** IP address configuration options */
typedef enum
{
    WICED_USE_EXTERNAL_DHCP_SERVER, /**< Client interface: use an external DHCP server  */
    WICED_USE_STATIC_IP,            /**< Client interface: use a fixed IP address       */
    WICED_USE_INTERNAL_DHCP_SERVER  /**< softAP interface: use the internal DHCP server */
} wiced_network_config_t;


/** DCT app section configuration item data type */
typedef enum
{
    CONFIG_STRING_DATA,       /**< String data type */
    CONFIG_UINT8_DATA,        /**< uint8 data type  */
    CONFIG_UINT16_DATA,       /**< uint16 data type */
    CONFIG_UINT32_DATA        /**< uint32 data type */
} configuration_data_type_t;


/** WICED Network link subscription types denote whether
 *  to subscribe for link up or link down events */
typedef enum
{
    WICED_LINK_UP_SUBSCRIPTION,     /**< Link up event subscription   */
    WICED_LINK_DOWN_SUBSCRIPTION    /**< Link down event subscription */
} wiced_link_subscription_t;

/** WICED Network link status */
typedef enum
{
    WICED_LINK_UP,   /**< Link status up   */
    WICED_LINK_DOWN  /**< Link status down */
} wiced_link_status_t;

typedef enum
{
    WICED_NETWORK_PACKET_TX,     /**< Network packet for data transmission */
    WICED_NETWORK_PACKET_RX      /**< Network packet for data reception    */
} wiced_network_packet_dir_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/** Network link callback */
typedef void (*wiced_network_link_callback_t)(void);

/******************************************************
 *                    Structures
 ******************************************************/

/** IP address settings */
typedef struct
{
    wiced_ip_address_t ip_address;  /**< IP address      */
    wiced_ip_address_t gateway;     /**< Gateway address */
    wiced_ip_address_t netmask;     /**< Netmask         */
} wiced_ip_setting_t;


/** DCT app section configuration item entry */
typedef struct
{
    char*                     name;        /**< Name of the entry              */
    uint32_t                  dct_offset;  /**< Offset of the entry in the DCT */
    uint32_t                  data_size;   /**< Size of the entry              */
    configuration_data_type_t data_type;   /**< Type of the entry              */
} configuration_entry_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 *
 ******************************************************/

/*****************************************************************************/
/** @defgroup mgmt       Management
 *
 *  WICED Management Functions
 */
/*****************************************************************************/


/*****************************************************************************/
/** @addtogroup initconf       Initialisation & configuration
 *  @ingroup mgmt
 *
 * Initialisation/Deinitialisation of WICED and device configuration functions
 *
 *  @{
 */
/*****************************************************************************/


/** Initialises the WICED system
 *
 * This function sets up the system by :
 *  \li initialising the platform interface
 *  \li initialising the RTOS & Network Stack
 *  \li initialising the WLAN driver and chip
 *  \li starting the event processing thread
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_init( void );


/** De-initialises the WICED system
 *
 * This function de-initialises the WICED system by :
 *  \li bringing down all network interfaces
 *  \li deleting all packet pools
 *  \li tearing down the event thread
 *  \li powering down the WLAN chip
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_deinit( void );


/** Initialises network sub-system only
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_network_init  ( void );


/** De-initialises network sub-system only
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_network_deinit( void );

/** Enables all powersave features
 *
 *  This is a convenience function that calls each of the powersave related functions listed below \n
 *  Please review the documentation for each function for further information
 *  \li @ref wiced_platform_mcu_enable_powersave()
 *  \li @ref wiced_wifi_enable_powersave()
 *  \li @ref wiced_network_suspend()
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_enable_powersave( void );


/** Disables all powersave features
 *
 *  This is a convenience functions that calls each of the powersave related functions listed below \n
 *  Please review the documentation for each function for further information
 *  \li @ref wiced_platform_mcu_disable_powersave()
 *  \li @ref wiced_wifi_disable_powersave()
 *  \li @ref wiced_network_resume()
 *
 * @return WICED_SUCCESS
 */
extern wiced_result_t wiced_disable_powersave( void );


/** Runs device configuration (if required)
 *
 * @param[in] config  : an array of user configurable variables in configuration_entry_t format.
 *                      The array must be terminated with a "null" entry {0,0,0,0}
 *
 * @return    WICED_SUCCESS
 */
extern wiced_result_t wiced_configure_device( const configuration_entry_t* config );


/** Re-runs device configuration
 *
 * @param[in] config  : an array of user configurable variables in configuration_entry_t format.
 *                      The array must be terminated with a "null" entry {0,0,0,0}
 *
 * @return    WICED_SUCCESS
 */
extern wiced_result_t wiced_reconfigure_device( const configuration_entry_t* config );

/** @} */



/*****************************************************************************/
/** @addtogroup netmgmt       Network management
 *  @ingroup mgmt
 *
 * Functions to manage the network interfaces
 *
 *  @{
 */
/*****************************************************************************/

/** Set network hostname in DCT
 *
 *  NOTE: This function will change the DCT.
 *
 * @param[in] name          : a null terminated string (Note: this will be truncated to a maximum of 32 characters)
 *
 * @return    @ref wiced_result_t
 */
extern wiced_result_t wiced_network_set_hostname( const char* name );

/** Get network hostname from DCT
 *
 * @param[in] name          : a pointer to a wiced_hostname_t object to store the hostname
 *
 * @return    @ref wiced_result_t
 */
extern wiced_result_t wiced_network_get_hostname( wiced_hostname_t* name );

/** Brings up a network interface
 *
 *
 * @param[in] interface     : the interface to bring up
 * @param[in] config        : the network IP configuration
 * @param[in] ip_settings   : static IP settings that are mandatory for the AP interface,
 *                        but are optional for the STA interface
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_network_up( wiced_interface_t interface, wiced_network_config_t config, const wiced_ip_setting_t* ip_settings );

	extern volatile uint8_t wiced_network_up_cencel;

extern wiced_result_t wiced_interface_up( wiced_interface_t interface );


/** Creates a network packet pool from a chunk of memory
 *
 * @param[in] memory_pointer : pointer to a chunk of memory
 * @param[in] memory_size    : size of the memory chunk
 * @param[in] direction      : network packet reception or transmission
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_network_create_packet_pool( uint8_t* memory_pointer, uint32_t memory_size, wiced_network_packet_dir_t direction );


/** Brings down a network interface
 *
 * @param[in] interface : the interface to bring down
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_network_down( wiced_interface_t interface );


/** Suspends network services and disables all network related timers
 *
 * This function must only be called before entering deep sleep. Prior to calling this function, ensure all
 * network sockets are in a disconnected state. After calling this function, networking functions
 * should not be used. To resume network operation, use the @ref wiced_network_resume() function.
 *
 * Example usage:
 *   @code
 *      wiced_network_suspend();
 *      wiced_rtos_delay_milliseconds(DEEP_SLEEP_TIME);
 *      wiced_network_resume();
 *   @endcode
 *
 * @return    WICED_SUCCESS : Network services are suspended.
 *            WICED_ERROR   : Network services were unable to be suspended, active sockets still exist
 */
extern wiced_result_t wiced_network_suspend( void );


/** Resumes network services
 *
 * This function resumes network services after suspension
 * with the wiced_network_suspend() function. After calling this function, all network functions
 * are available for use.
 *
 * Example usage:
 *   @code
 *      wiced_network_suspend();
 *      wiced_rtos_delay_milliseconds(DEEP_SLEEP_TIME);
 *      wiced_network_resume();
 *   @endcode
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_network_resume( void );


/** Checks if a network interface is up at the 802.11 link layer
 *
 * @param[in] interface : the interface to check
 *
 * @return @ref wiced_bool_t
 */
extern wiced_bool_t wiced_network_is_up( wiced_interface_t interface );

/** Checks if a network interface is up at the IP layer
 *
 * @param[in] interface : the interface to check
 *
 * @return @ref wiced_bool_t
 */
extern wiced_bool_t wiced_network_is_ip_up( wiced_interface_t interface );


/** Reads default network interface from DCT and brings up network
 *
 * Result from reading DCT is stored in interface
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_network_up_default( wiced_interface_t* interface, const wiced_ip_setting_t* ap_ip_settings );

/** Returns the default ready interface
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_get_default_ready_interface( wiced_interface_t* interface );


/** Register callback function/s that gets called when a change in network link status occurs
 *
 * @param link_up_callback   : the optional callback function to register for the link up event
 * @param link_down_callback : the optional callback function to register for the link down event
 *
 * @return    WICED_SUCCESS : on success.
 * @return    WICED_ERROR   : if an error occurred with any step
 */
extern wiced_result_t wiced_network_register_link_callback( wiced_network_link_callback_t link_up_callback, wiced_network_link_callback_t link_down_callback, wiced_interface_t interface );

/** De-register network link status callback function/s
 *
 * @param link_up_callback   : the optional callback function to deregister for the link up event
 * @param link_down_callback : the optional callback function to deregister for the link down event
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_network_deregister_link_callback( wiced_network_link_callback_t link_up_callback, wiced_network_link_callback_t link_down_callback, wiced_interface_t interface );


/** @} */

/*****************************************************************************/
/** @addtogroup sysmon       Advanced Init
 *  @ingroup mgmt
 *
 * Functions to initialise WICED in a more modular way.
 * wiced_init()    =  wiced_core_init()   + wiced_wlan_connectivity_init()
 * wiced_deinit()  =  wiced_core_deinit() + wiced_wlan_connectivity_deinit()
 *
 *  @{
 */
/*****************************************************************************/

/** Initialises the core parts of WICED without starting any WLAN systems
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_core_init               ( void );

/** De-initialises the core parts of WICED without touching any WLAN systems
 *
 * @note: WLAN should be already de-inited when this function is called
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_core_deinit             ( void );

/** Initialises the WLAN parts of WICED
 *
 * @note: The WICED core should have already been initialised when this is called
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_wlan_connectivity_init  ( void );

/** Initialises the WLAN parts of WICED
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_wlan_connectivity_deinit( void );


/*****************************************************************************/
/** @addtogroup initconf       Deep-sleep related functions
 *  @ingroup deep-sleep
 *
 * Functions to resume WICED after the deep-sleep.
 * Platform is not necessary to support deep-sleep mode.
 *
 *  @{
 */
/*****************************************************************************/

/** Resumes the WICED system after deep-sleep
 *
 * This function sets up the system same way as wiced_init
 * and has to be used when system resumes from deep-sleep
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_resume_after_deep_sleep( void );


/** Brings up a network interface after deep-sleep
 *
 *
 * @param[in] interface     : the interface to bring up
 * @param[in] config        : the network IP configuration
 * @param[in] ip_settings   : static IP settings that are mandatory for the AP interface,
 *                        but are optional for the STA interface
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_network_resume_after_deep_sleep( wiced_interface_t interface, wiced_network_config_t config, const wiced_ip_setting_t* ip_settings );


/** Resume the WLAN parts of WICED after host deep-sleep
 *
 * @note: The WICED core should have already been initialised when this is called
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_wlan_connectivity_resume_after_deep_sleep( void );

wiced_bool_t wiced_wlan_connectivity_initialized( void );

/** @} */

#ifdef __cplusplus
} /*extern "C" */
#endif
