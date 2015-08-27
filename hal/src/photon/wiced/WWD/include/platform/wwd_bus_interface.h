/*
 * Copyright (c) 2015 Broadcom
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * 3. Neither the name of Broadcom nor the names of other contributors to this
 * software may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * 4. This software may not be used as a standalone product, and may only be used as
 * incorporated in your product or device that incorporates Broadcom wireless connectivity
 * products and solely for the purpose of enabling the functionalities of such Broadcom products.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY WARRANTIES OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT, ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

/** @file
 *  Defines the Bus part of the WICED Platform Interface.
 *
 *  Provides prototypes for functions that allow WICED to start and stop
 *  the hardware communications bus for a platform.
 */

#include "network/wwd_buffer_interface.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** @addtogroup platif Platform Interface
 *  @{
 */

/** @name Bus Functions
 *  Functions that enable WICED to start and stop
 *  the hardware communications bus for a paritcular platform.
 */
/**@{*/

/******************************************************
 *             Function declarations
 ******************************************************/

/**
 * Initializes the WICED Bus
 *
 * Implemented in the WICED Platform interface which is specific to the
 * platform in use.
 * This function should set the bus GPIO pins to the correct modes,  set up
 * the interrupts for the WICED bus, enable power and clocks to the bus, and
 * set up the bus peripheral.
 *
 * @return WWD_SUCCESS or Error code
 */
extern wwd_result_t host_platform_bus_init( void );

/**
 * De-Initializes the WICED Bus
 *
 * Implemented in the WICED Platform interface which is specific to the
 * platform in use.
 * This function does the reverse of @ref host_platform_bus_init
 *
 * @return WWD_SUCCESS or Error code
 */
extern wwd_result_t host_platform_bus_deinit( void );


/**
 * Informs WWD of an interrupt
 *
 * This function should be called from the SDIO/SPI interrupt function
 * and usually indicates newly received data is available.
 * It wakes the WWD Thread, forcing it to check the send/receive
 *
 */
extern void wwd_thread_notify_irq( void );


/**
 * Enables the bus interrupt
 *
 * This function is called by WICED during init, once
 * the system is ready to receive interrupts
 */
extern wwd_result_t host_platform_bus_enable_interrupt( void );

/**
 * Disables the bus interrupt
 *
 * This function is called by WICED during de-init, to stop
 * the system supplying any more interrupts in preparation
 * for shutdown
 */
extern wwd_result_t host_platform_bus_disable_interrupt( void );


/**
 * Informs the platform bus implementation that a buffer has been freed
 *
 * This function is called by WICED during buffer release to allow
 * the platform to reuse the released buffer - especially where
 * a DMA chain needs to be refilled.
 */
extern void host_platform_bus_buffer_freed( wwd_buffer_dir_t direction );

/** @} */
/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif
