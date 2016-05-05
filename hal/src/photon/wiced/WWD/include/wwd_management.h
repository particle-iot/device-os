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
 * WICED Initialisation Function
 *
 * - Initialises parts of the hardware platform used for Wi-Fi control
 *   including pins for : SDIO/SPI, interrupt, reset, power etc.
 * - Initialises the WICED thread (which arbitrates access to the SDIO/SPI bus)
 * - Sets the country code to set which radio channels are allowed
 * - Clear the events mask
 * - Bring the Wireless interface to the 'Up' state
 * - Turn off minimum power consumption feature to avoid SDIO timeouts
 *
 * @param country : enumerated country identifier, which sets the allowed
 *                  radio channels according to country regulations
 * @param buffer_interface_arg : abstract parameter which is passed
 *                        to the buffer interface during init.
 *                        Look in @ref host_buffer_init for details
 *                        of the argument required for your particular buffering system
 *
 * @return WWD_SUCCESS if initialization is successful, Error code otherwise
 */

wwd_result_t wwd_management_init( wiced_country_code_t country, /*@null@*/ void* buffer_interface_arg );

/**
 * Turn on the Wi-Fi device
 *
 * - Initialises the required parts of the hardware platform
 *   i.e. pins for SDIO/SPI, interrupt, reset, power etc.
 *
 * - Bring the Wireless interface "Up"
 * - Initialises the Wiced thread which arbitrates access
 *   to the SDIO/SPI bus
 *
 * @return WWD_SUCCESS if initialization is successful, Error code otherwise
 */
wwd_result_t wwd_management_wifi_on( void );

/**
 * Turn off the Wi-Fi device
 *
 * - De-Initialises the required parts of the hardware platform
 *   i.e. pins for SDIO/SPI, interrupt, reset, power etc.
 *
 * - Bring the Wireless interface "Down"
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
