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
 *  Defines the SPI part of the WICED Platform Interface.
 *
 *  Provides constants and prototypes for functions that
 *  enable WICED to use a SPI bus on a particular hardware platform.
 */

#include <stdint.h>
#include "wiced_utilities.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** @addtogroup platif Platform Interface
 *  @{
 */

/** @name SPI Bus Functions
 *  Functions that enable WICED to use a SPI bus
 *  on a particular hardware platform.
 */
/**@{*/

/******************************************************
 *             Function declarations
 ******************************************************/


/**
 * Transfers SPI data
 *
 * Implemented in the WICED platform interface, which is specific to the
 * platform in use.
 * WICED uses this function as a generic way to transfer data
 * across a SPI bus.
 *
 * @param dir          : Direction of transfer - Write = To Wi-Fi device,
 *                                               Read  = from Wi-Fi device
 * @param buffer       : Pointer to data buffer where either data to send is
 *                       stored, or where received data is to be stored
 * @param buffer_length : Length of the data buffer provided
 *
 */
extern wwd_result_t host_platform_spi_transfer( wwd_bus_transfer_direction_t dir, uint8_t* buffer, uint16_t buffer_length );


/**
 * SPI interrupt handler
 *
 * This function is implemented by Wiced and must be called
 * from the SPI interrupt vector
 *
 */
extern void exti_irq( void );

/** @} */
/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif
