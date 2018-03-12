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
 *  Declares platform SDIO Host/Device functions
 */
#pragma once

#include "wiced_platform.h"
#include "platform/wwd_sdio_interface.h"    /* To use sdio_command_t, sdio_transfer_mode_t, and sdio_block_size_t */
#include <bcmsdbus.h>   /* bcmsdh to/from specific controller APIs */

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 * @cond       Constants
 ******************************************************/

typedef struct
{
    uint8_t       write_data;           /* 0 - 7 */
    unsigned int  _stuff2          : 1; /* 8     */
    unsigned int  register_address :17; /* 9-25  */
    unsigned int  _stuff           : 1; /* 26    */
    unsigned int  raw_flag         : 1; /* 27    */
    unsigned int  function_number  : 3; /* 28-30 */
    unsigned int  rw_flag          : 1; /* 31    */
} sdio_cmd52_argument_t;

typedef struct
{
    unsigned int  count            : 9; /* 0-8   */
    unsigned int  register_address :17; /* 9-25  */
    unsigned int  op_code          : 1; /* 26    */
    unsigned int  block_mode       : 1; /* 27    */
    unsigned int  function_number  : 3; /* 28-30 */
    unsigned int  rw_flag          : 1; /* 31    */
} sdio_cmd53_argument_t;

typedef union
{
    uint32_t              value;
    sdio_cmd52_argument_t cmd52;
    sdio_cmd53_argument_t cmd53;
} sdio_cmd_argument_t;

typedef enum
{
    SDIO_READ = 0,
    SDIO_WRITE = 1
} sdio_transfer_direction_t;

/* Configure callback to client when host receive client interrupt */
typedef void (*sdio_isr_callback_function_t)(void *);

/******************************************************
 *             Global declarations
 ******************************************************/


/******************************************************
 *             Structures
 ******************************************************/

/** @endcond */

/** @addtogroup platif Platform Interface
 *  @{
 */

/** @name SDIO Bus Functions
 *  Functions that enable WICED to use an SDIO bus
 *  on a particular hardware platform.
 */
/**@{*/


/******************************************************
 *             Function declarations
 ******************************************************/

/**
 * Initializes the SDIO Bus
 *
 * This function set up the interrupts, enable power and clocks to the bus.
 *
 * @return WICED_SUCCESS or Error code
 */
extern wiced_result_t platform_sdio_host_init( sdio_isr_callback_function_t client_check_isr_function );

/**
 * De-Initializes the SDIO Bus
 *
 * This function does the reverse of @ref platform_sdio_host_init.
 *
 * @return WICED_SUCCESS or Error code
 */
extern wiced_result_t platform_sdio_host_deinit( void );

/**
 * Transfers SDIO data
 *
 * Implemented in the WICED Platform interface, which is specific to the
 * platform in use.
 * WICED uses this function as a generic way to transfer data
 * across an SDIO bus.
 * Please refer to the SDIO specification.
 *
 * @param direction         : Direction of transfer - Write = to Wi-Fi device,
 *                                                    Read  = from Wi-Fi device
 * @param command           : The SDIO command number
 * @param mode              : Indicates whether transfer will be byte mode or block mode
 * @param block_size        : The block size to use (if using block mode transfer)
 * @param argument          : The argument of the particular SDIO command
 * @param data              : A pointer to the data buffer used to transmit or receive
 * @param data_size         : The length of the data buffer
 * @param response_expected : Indicates if a response is expected - RESPONSE_NEEDED = Yes
 *                                                                  NO_RESPONSE     = No
 * @param response  : A pointer to a variable which will receive the SDIO response.
 *                    Can be null if the caller does not care about the response value.
 *
 * @return WICED_SUCCESS if successful, otherwise an error code
 */
extern wiced_result_t platform_sdio_host_transfer( sdio_transfer_direction_t direction, sdio_command_t command, sdio_transfer_mode_t mode, sdio_block_size_t block_size, uint32_t argument, /*@null@*/ uint32_t* data, uint16_t data_size, sdio_response_needed_t response_expected, /*@out@*/ /*@null@*/ uint32_t* response );

/**
 * Switch SDIO bus to high speed mode
 *
 * When SDIO starts, it must be in a low speed mode
 * This function switches it to high speed mode (up to 50MHz)
 *
 */
extern void platform_sdio_host_enable_high_speed( void );

/**
 * Enables the bus interrupt
 *
 * This function is called once the system is ready to receive interrupts
 */
extern wiced_result_t platform_sdio_host_enable_interrupt( void );

/**
 * Disables the bus interrupt
 *
 * This function is called to stop the system supplying any more interrupts
 */
extern wiced_result_t platform_sdio_host_disable_interrupt( void );


/** @} */
/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif
