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
 *  Block device driver declarations for WICED
 */

#pragma once


#include <stdint.h>
#include "wiced_result.h"


#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                     Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/* Use this with erase_block_size */
#define BLOCK_DEVICE_ERASE_NOT_REQUIRED (0)

/* Use this with write_block_size */
#define BLOCK_DEVICE_WRITE_NOT_ALLOWED (0)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    BLOCK_DEVICE_UNINITIALIZED,
    BLOCK_DEVICE_DOWN,
    BLOCK_DEVICE_UP_READ_ONLY,
    BLOCK_DEVICE_UP_READ_WRITE,
} wiced_block_device_status_t;

typedef enum
{
    BLOCK_DEVICE_READ_ONLY,
    BLOCK_DEVICE_WRITE_IMMEDIATELY,
    BLOCK_DEVICE_WRITE_BEHIND_ALLOWED
} wiced_block_device_write_mode_t;


/******************************************************
 *                 Type Definitions
 ******************************************************/

/* Forward delclared due to self references */
typedef struct wiced_block_device_struct        wiced_block_device_t;         /** This is the main block device handle */
typedef struct wiced_block_device_driver_struct wiced_block_device_driver_t;

typedef void (*wiced_block_device_status_change_callback_t)( wiced_block_device_t* device, wiced_block_device_status_t new_status );

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    uint64_t                                    base_address_offset;                         /** Offset address used when accessing the device */
    uint64_t                                    maximum_size;                                /** 0 = use the underlying device limit */
    wiced_bool_t                                volatile_and_requires_format_when_mounting;  /** Will cause the device to be formatted before EVERY mount - use for RAM disks */
} wiced_block_device_init_data_t;


struct wiced_block_device_struct
{
    const wiced_block_device_init_data_t* init_data;
    const wiced_block_device_driver_t*    driver;
    wiced_bool_t                          initialized;
    uint32_t                              device_id;
    uint64_t                              device_size;
    uint32_t                              read_block_size;      /** 1 indicates data can be accessed byte-by-byte */
    uint32_t                              write_block_size;     /** Zero if writing is not allowed - e.g. device is read only.   1 indicates data can be accessed byte-by-byte */
    uint32_t                              erase_block_size;     /** Zero if erasing is not required - e.g. for a RAM disk.       1 indicates data can be accessed byte-by-byte */
    void*                                 device_specific_data; /** Points to init data & space for variables for the specific underlying device e.g. SD-Card, USB, Serial-Flash etc */
    wiced_block_device_status_change_callback_t callback;
};

struct wiced_block_device_driver_struct
{
    /**
     * Initialises the block device
     *
     * This must be run before accessing any of the other driver functions
     * or any of the structure variables, except those in init_data.
     *
     * @param[in]  device               - The block device to initialize - elements init_data, driver and device_specific_data must be valid.
     * @param[in]  write_mode           - Determines whether write is allowed, and whether write-behind is allowed
     *
     * @return WICED_SUCCESS on success
     */
    wiced_result_t (*init)( wiced_block_device_t* device, wiced_block_device_write_mode_t write_mode );

    /**
     * De-initialises the block device
     *
     * Must have been previously initialized with the "init" function
     *
     * @param[in]  device               - The block device to de-initialize
     *
     * @return WICED_SUCCESS on success
     */
    wiced_result_t (*deinit)( wiced_block_device_t* device );

    /**
     * Erases a block on the device
     *
     * This function may not be implemented, so you MUST check whether it is NULL
     * before using it.
     *
     * @param[in]  device        - The device on which to erase
     * @param[in]  start_address - The start address - must be located on the start of a erase block boundary
     * @param[in]  size          - The number of bytes to erase - must match the size of a whole number of erase blocks
     *
     * @return WICED_SUCCESS on success, Error on failure or if start/end are not on an erase block boundary
     */
    wiced_result_t (*erase)( wiced_block_device_t* device, uint64_t start_address, uint64_t size );

    /**
     * Writes data to the device
     *
     * Check whether device requires erasing via the erase_block_size element. If erasing is required, it must be done
     * before writing.
     *
     * write_behind == BLOCK_DEVICE_WRITE_BEHIND_ALLOWED then this may return immediately with data in write queue
     *
     * @param[in]  device        - The device to which to write
     * @param[in]  start_address - The start address - must be located on the start of a write block boundary
     * @param[in]  data          - The buffer containing the data to write
     * @param[in]  size          - The number of bytes to write - must match the size of a whole number of write blocks
     *
     * @return WICED_SUCCESS on success, Error on failure or if start/end are not on an write block boundary
     */
    wiced_result_t (*write)( wiced_block_device_t* device, uint64_t start_address, const uint8_t* data, uint64_t size );

    /**
     * Flushes data to the device
     *
     * This function may not be implemented, so you MUST check whether it is NULL before using it.
     *
     * If write_behind == BLOCK_DEVICE_WRITE_BEHIND_ALLOWED then this will write any pending data to the device before
     * returning
     *
     * @param[in]  device        - The device to flush
     *
     * @return WICED_SUCCESS on success, Error on failure or if start/end are not on an write block boundary
     */
    wiced_result_t (*flush)( wiced_block_device_t* device );

    /**
     * Reads data from the device
     *
     * @param[in]  device        - The device from which to read
     * @param[in]  start_address - The start address - must be located on the start of a read block boundary
     * @param[out] data          - The buffer which will receive the data
     * @param[in]  size          - The number of bytes to read - must match the size of a whole number of read blocks
     *
     * @return WICED_SUCCESS on success, Error on failure or if start/end are not on an read block boundary
     */
    wiced_result_t (*read)( wiced_block_device_t* device, uint64_t start_address, uint8_t* data, uint64_t size );


    /**
     * Get the current status of the device
     *
     * @param[in]  device  - The device to query
     * @param[out] status  - Variable which receives the status
     *
     * @return WICED_SUCCESS on success, Error on failure or if start/end are not on an read block boundary
     */
    wiced_result_t (*status)( wiced_block_device_t* device, wiced_block_device_status_t* status );


    /**
     * Register a callback which the device will call when there is a status change
     *
     * @param[in]  device   - The device to query
     * @param[in]  callback - The callback function
     *
     * @return WICED_SUCCESS on success, Error on failure or if start/end are not on an read block boundary
     */
    wiced_result_t (*register_callback)( wiced_block_device_t* device, wiced_block_device_status_change_callback_t callback );

};


/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

#ifdef __cplusplus
} /*extern "C" */
#endif

