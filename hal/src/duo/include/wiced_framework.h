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
 *  Defines functions that allow access to the Device Configuration Table (DCT)
 *
 */

#pragma once

#include <stdint.h>
#include "platform_dct.h"
#include "wiced_result.h"
#include "wiced_utilities.h"
#include "wiced_dct_common.h"
#include "wiced_waf_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 * @cond                Macros
 ******************************************************/

#define DCT_OFFSET( type, member )  ( (uint32_t)&((type *)0)->member )

#if defined(__IAR_SYSTEMS_ICC__)
#pragma section="initial_dct_section"
#define DEFINE_APP_DCT( type ) \
    const type _app_dct @ "initial_dct_section"; \
    const type _app_dct =
#else  /* #if defined(__IAR_SYSTEMS_ICC__) */
#define DEFINE_APP_DCT( type ) const type _app_dct =
#endif /* #if defined(__IAR_SYSTEMS_ICC__) */

/******************************************************
 *                    Constants
 ******************************************************/

#define WICED_FACTORY_RESET_MAGIC_VALUE  ( 0xA6C5A54E )
#define WICED_FRAMEWORK_LOAD_ALWAYS      ( 0x0 )
#define WICED_FRAMEWORK_LOAD_ONCE        ( 0x1 )


/******************************************************
 *                   Enumerations
 ******************************************************/

/**
 * Application validity
 */
typedef enum
{
    APP_INVALID = 0,
    APP_VALID   = 1
} app_valid_t;

/**
 * Bootloader status
 */
typedef enum boot_status_enum
{
    BOOTLOADER_BOOT_STATUS_START                   = 0x00,
    BOOTLOADER_BOOT_STATUS_APP_OK                  = 0x01,
    BOOTLOADER_BOOT_STATUS_WLAN_BOOTED_OK          = 0x02,
    BOOTLOADER_BOOT_STATUS_WICED_STARTED_OK        = 0x03,
    BOOTLOADER_BOOT_STATUS_DATA_CONNECTIVITY_OK    = 0x04
} boot_status_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/**
 * Copy Function Source Structure
 */
typedef struct
{
    union
    {
        void*    file_handle;
        uint32_t fixed_address;
    } val;
    enum
    {
        COPY_SRC_FILE,
        COPY_SRC_FIXED,
    } type;
} platform_copy_src_t;


/** Structure to hold information about a system monitor item */
typedef struct
{
    uint32_t last_update;              /**< Time of the last system monitor update */
    uint32_t longest_permitted_delay;  /**< Longest permitted delay between checkins with the system monitor */
} wiced_system_monitor_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 * @endcond
 ******************************************************/

/*****************************************************************************/
/** @defgroup framework       WICED Application Framework
 *
 *  WICED functions for managing apps and non-volatile data
 */
/*****************************************************************************/

/*****************************************************************************/
/** @addtogroup dct       DCT
 *  @ingroup framework
 *
 * Device Configuration Table (Non-volatile flash storage space)
 *
 *
 *  @{
 */
/*****************************************************************************/

/** Reads the DCT and returns a pointer to the DCT data
 *
 * The operation of this function depends on whether the DCT is located in
 * external or internal flash memory.
 * If ptr_is_writable is set to false and the DCT is located in internal flash,
 * then a direct pointer to the flash memory will be returned.
 * Otherwise memory will be allocated and the DCT data will be copied into it.
 *
 * @note : this function must be used in pairs with wiced_dct_read_unlock to
 *         ensure that any allocated memory is freed.
 *
 * @param info_ptr [out] : a pointer to the pointer that will be filled on return
 * @param ptr_is_writable [in] : if true then then the returned pointer will be in RAM
 *                          allowing it to be modified. e.g. before being written
 * @param section [in] : the section of the DCT which should be read
 * @param offset [in] : the offset in bytes within the section
 * @param size [in] : the length of data that should be read
 *
 * @return    Wiced Result
 */
extern wiced_result_t wiced_dct_read_lock(   void** info_ptr, wiced_bool_t ptr_is_writable, dct_section_t section, uint32_t offset, uint32_t size );


/** Frees any space allocated in wiced_dct_read_lock()
 *
 * @note : this function must be used in pairs with wiced_dct_read_lock
 *
 * @param info_ptr [in] : a pointer that was created with wiced_dct_read_lock()
 * @param ptr_is_writable[in] : indicates whether the pointer was retrevied as a writable pointer
 *
 * @return    Wiced Result
 */
extern wiced_result_t wiced_dct_read_unlock( void* info_ptr, wiced_bool_t ptr_is_writable );


/** Writes data to the DCT
 *
 * Writes a chunk of data to the DCT.
 *
 * @note : Ensure that this function is only called occasionally, otherwise
 *         the flash memory wear may result.
 *
 * @param info_ptr [in] : a pointer to the pointer that will be filled on return
 * @param section [in]: the section of the DCT which should be read
 * @param offset [in]: the offset in bytes within the section
 * @param size [in] : the length of data that should be written
 *
 * @return    Wiced Result
 */
wiced_result_t wiced_dct_write( const void* info_ptr, dct_section_t section, uint32_t offset, uint32_t size );

wiced_result_t wiced_dct_write_boot_details( const boot_detail_t* new_boot_details );
wiced_result_t wiced_dct_write_app_location( image_location_t* new_app_location_info, uint32_t dct_app_index );

/** @} */

/*****************************************************************************/
/** @addtogroup app       App management
 *
 * Application management functions
 *
 * @note: these functions are implemented as function pointers to allow
 *        them to be shared between a bootloader and application
 *
 *  @{
 */
/*****************************************************************************/

/** Sets the next booting application after reset
 *
 * Updates the boot details to point to the specified application ID. \p
 *
 * A valid application must exist in the specified application. If the applications
 * is not valid, the behaviour is undetermined.
 *
 * @warning To run the new a reboot is required.
 *
 * @param[in] app_id   : Application ID.
 * @param[in] load_once: A flag to indicate wether to  reload the application every time or just
 *                       once. A RAM application will need to be reloaded every time while
 *                       applications running from flash can be loaded just once.
 *
 * @return Wiced reuslt code
 *
 */
static inline wiced_result_t wiced_framework_set_boot( uint8_t app_id, char load_once );

/** Reboots the system
 *
 * Causes a soft-reset of the processor, which will restart the program
 * from the boot vector. The function does not return.
 *
 *  @return Does not return!
 */
static inline void wiced_framework_reboot( void );

/** Initialize the applicatin for modification.
 *
 * Unlock the application for later modification.
 *
 * @warning Applications must be opened before any write or read operations.
 *
 * @param[in] app_id   : Application ID.
 * @param[inout] app   : Application handler.
 *
 * @return Wiced reuslt code
 */
static inline wiced_result_t wiced_framework_app_open( uint8_t app_id, wiced_app_t* app );

/** Finilaize application modification.
 *
 * Lock the application (flush any cached operations).
 *
 * @warning Applications must be closed after all write and read operations.
 *
 * @param[in] app   : Application handler.
 *
 * @return Wiced reuslt code
 */
static inline wiced_result_t wiced_framework_app_close( wiced_app_t* app );

/** Erases the application from external flash.
 *
 * Erase the full application content from external flash.
 *
 * @warning Applications must be erased before being rewritten.
 *
 * @param[inout] app   : Application handler.
 *
 * @return Wiced reuslt code
 */
static inline wiced_result_t wiced_framework_app_erase( wiced_app_t* app );

/** Writes a piece of the application to external flash
 *
 * Write a piece of a the application into external flash.
 *
 * @warning Applications must be erased before being rewritten. To fully erase an
 *          application call wiced_framework_erase_app. Applications can also be
 *          erased on go by passing last_erased_sector pointer. However when erasing
 *          on the go, writing to the file must be sequential with no gaps.
 *
 * @param[inout] app : Application handler.
 * @param[in] offset : The offset from the start of the application in bytes
 * @param[in] data   : The data to be written
 * @param[in] size   : The number of bytes to be written
 * @param[in/out] last_erased_sector : Holds the last erased sector.
 *
 *
 *  @return Wiced reuslt code
 */
static inline wiced_result_t  wiced_framework_app_write_chunk( wiced_app_t* app, const uint8_t* data, uint32_t size );

/** Reads a piece of the application from external flash
 *
 * Reads a piece of the application from external flash.
 *
 * @param[inout] app : Application handler.
 * @param[in] offset : The offset from the start of the application in bytes
 * @param[in] data   : The buffer for the data to be read
 * @param[in] size   : The number of bytes to be read
 *
 *  @return Wiced reuslt code
 */
static inline wiced_result_t  wiced_framework_app_read_chunk( wiced_app_t* app, uint32_t offset, uint8_t* data, uint32_t size );

/** Returns the current size of the application.
 *
 * Returns the current size of the application.
 *
 * @warning The size of the application is always aligned to sector boundaries. Application
 *          size may be different from actual size on a PC.
 *
 * @param[inout] app : Application handler.
 * @param[out] size  : The size allocated to the application in bytes.
 *
 *  @return Wiced reuslt code
 */
static inline wiced_result_t  wiced_framework_app_get_size( wiced_app_t* app, uint32_t* size );

/** Sets the current size of the application.
 *
 * Sets the current size of the application.
 *
 * @warning Before updating the application, the size must be set to match the new size of
 *          the application. The size of the application is always aligned to sector
 *          boundaries. Application size may be different from actual size on a PC. If the
 *          provided size is smaller than the current size, the current size is maintained.
 *
 * @param[inout] app : Application handler.
 * @param[in] size   : The new size allocated to the application in bytes.
 *
 *  @return Wiced reuslt code
 */
static inline wiced_result_t  wiced_framework_app_set_size( wiced_app_t* app, uint32_t size );

/** @} */


/*****************************************************************************/
/** @addtogroup sysmon       System Monitor
 *  @ingroup mgmt
 *
 * Functions to communicate with the system monitor
 *
 *  @{
 */
/*****************************************************************************/

/** Registers a system monitor with the system monitor thread
 *
 * @param[out] system_monitor          : A pointer to a system monitor object that will be watched
 * @param[in]  initial_permitted_delay : The maximum time in milliseconds allowed between monitor updates
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_register_system_monitor(wiced_system_monitor_t* system_monitor, uint32_t initial_permitted_delay);

/** Updates a system monitor and resets the last update time
 *
 * @param[out] system_monitor  : A pointer to a system monitor object to be updated
 * @param[in]  permitted_delay : The maximum time in milliseconds allowed between monitor updates
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_update_system_monitor(wiced_system_monitor_t* system_monitor, uint32_t permitted_delay);

/** Wakeup system monitor thread
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t wiced_wakeup_system_monitor_thread(void);

/** @} */

/*****************************************************************************/
/** @addtogroup deprecated_dct       Deprecated DCT functions
 *  @ingroup dct
 *
 * Device Configuration Table (Non-volatile flash storage space)
 *
 *
 *  @{
 */
/*****************************************************************************/

#ifndef EXTERNAL_DCT

/** Retrieves a pointer to the application section of the current DCT
 *
 * @deprecated Please use @wiced_dct_read_app_section instead.
 *             Function is not compatible with a DCT located in external flash, and will
 *             be removed in future SDK versions
 *
 * @return    The app section pointer
 */
extern void const* DEPRECATE( wiced_dct_get_app_section( void ) );


/** Retrieves a pointer to the manufacturing info section of the current DCT
 *
 * @deprecated Please use @wiced_dct_get_mfg_info instead.
 *             Function is not compatible with a DCT located in external flash, and will
 *             be removed in future SDK versions
 *
 * @return    The manufacturing section pointer
 */
extern platform_dct_mfg_info_t const* DEPRECATE( wiced_dct_get_mfg_info_section( void ) );


/** Retrieves a pointer to the security section of the current DCT
 *
 * @deprecated Please use @wiced_dct_get_security_section instead.
 *             Function is not compatible with a DCT located in external flash, and will
 *             be removed in future SDK versions
 *
 * @return    The security section pointer
 */
extern platform_dct_security_t const* DEPRECATE( wiced_dct_get_security_section( void ) );


/** Retrieves a pointer to the Wi-Fi config info section of the current DCT
 *
 * @deprecated Please use @wiced_dct_get_wifi_config instead.
 *             Function is not compatible with a DCT located in external flash, and will
 *             be removed in future SDK versions
 *
 * @return    The Wi-Fi section pointer
 */
extern platform_dct_wifi_config_t const* DEPRECATE( wiced_dct_get_wifi_config_section( void ) );

/*------------------------------------- */

/** Reads a volatile copy of the DCT security section from flash into a block of RAM
 *
 * @param[out] security_dct : A pointer to the RAM that will receive a copy of the DCT security section
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t DEPRECATE( wiced_dct_read_security_section( platform_dct_security_t* security_dct ) );


/** Writes a volatile copy of the DCT security section in RAM to the flash
 *
 * @warning: To avoid flash wear, this function should only be used for settings which are changed rarely
 *
 * @param[in] security_dct : A pointer to the volatile copy of the DCT security section in RAM
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t DEPRECATE( wiced_dct_write_security_section( const platform_dct_security_t* security_dct ) );

/*------------------------------------- */

/** Reads a volatile copy of the DCT wifi config section from flash into a block of RAM
 *
 * @param[out] wifi_config_dct : A pointer to the RAM that will receive a copy of the DCT wifi config section
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t DEPRECATE( wiced_dct_read_wifi_config_section( platform_dct_wifi_config_t* wifi_config_dct ) );


/** Writes a volatile copy of the DCT Wi-Fi config section in RAM to the flash
 *
 * @warning: To avoid flash wear, this function should only be used for settings which are changed rarely
 *
 * @param[in] wifi_config_dct : A pointer to the volatile copy of DCT Wi-Fi config section in RAM
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t DEPRECATE( wiced_dct_write_wifi_config_section( const platform_dct_wifi_config_t* wifi_config_dct ) );

/*------------------------------------- */

/** Reads a volatile copy of the DCT app section from flash into a block of RAM
 *
 * @param[out] app_dct : A pointer to the RAM that will receive a copy of the DCT app section
 * @param[in]  size    : The size of the DCT app section
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t DEPRECATE( wiced_dct_read_app_section( void* app_dct, uint32_t size ) );


/** Writes a volatile copy of the DCT app section in RAM to the flash
 *
 * @note: To avoid wearing out the flash, this function should only be
 *        used for settings which are changed rarely.
 *
 * @param[in] size    : The size of the DCT app section
 * @param[in] app_dct : A pointer to the volatile copy of DCT app section in RAM
 *
 * @return @ref wiced_result_t
 */
extern wiced_result_t DEPRECATE( wiced_dct_write_app_section( const void* app_dct, uint32_t size ) );

/*------------------------------------- */

#endif /* ifndef EXTERNAL_DCT */

/** @} */

/*****************************************************************************/
/**  Implementations of inline functions                                    **/
/*****************************************************************************/
//
//static inline ALWAYS_INLINE wiced_result_t wiced_dct_write( const void* info_ptr, dct_section_t section, uint32_t offset, uint32_t size )
//{
//    return wiced_dct_update( info_ptr, section, offset, size );
//}

static inline ALWAYS_INLINE wiced_result_t wiced_framework_set_boot( uint8_t app_id, char load_once)
{
    return wiced_waf_app_set_boot( app_id, load_once );
}

static inline ALWAYS_INLINE void wiced_framework_reboot( void )
{
    wiced_waf_reboot( );
}

static inline ALWAYS_INLINE wiced_result_t wiced_framework_app_open( uint8_t app_id, wiced_app_t* app )
{
    return wiced_waf_app_open( app_id, app );
}

static inline ALWAYS_INLINE wiced_result_t wiced_framework_app_erase( wiced_app_t* app )
{
    return wiced_waf_app_erase( app );
}

static inline ALWAYS_INLINE wiced_result_t  wiced_framework_app_write_chunk( wiced_app_t* app, const uint8_t* data, uint32_t size )
{
    return wiced_waf_app_write_chunk( app, data, size);
}

static inline ALWAYS_INLINE wiced_result_t  wiced_framework_app_read_chunk( wiced_app_t* app, uint32_t offset, uint8_t* data, uint32_t size )
{
    return wiced_waf_app_read_chunk( app, offset, data, size);
}

static inline ALWAYS_INLINE wiced_result_t  wiced_framework_app_get_size( wiced_app_t* app, uint32_t* size )
{
    return wiced_waf_app_get_size( app, size);
}

static inline ALWAYS_INLINE wiced_result_t  wiced_framework_app_set_size( wiced_app_t* app, uint32_t size )
{
    return wiced_waf_app_set_size( app, size);
}

static inline ALWAYS_INLINE wiced_result_t wiced_framework_app_close( wiced_app_t* app )
{
    return wiced_waf_app_close( app );
}

/*****************************************************************************/
/**  Deprecated functions                                                   **/
/*****************************************************************************/
#ifdef __cplusplus
} /*extern "C" */
#endif
