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

#ifndef INCLUDED_WWD_BUS_PROTOCOL_INTERFACE_H_
#define INCLUDED_WWD_BUS_PROTOCOL_INTERFACE_H_

#include "wwd_constants.h"
#include "wwd_buffer.h"
#include "wwd_bus_protocol.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *             Constants
 ******************************************************/

typedef enum
{
    BUS_FUNCTION       = 0,
    BACKPLANE_FUNCTION = 1,
    WLAN_FUNCTION      = 2
} wwd_bus_function_t;

#define BUS_FUNCTION_MASK (0x3)  /* Update this if adding functions */

/******************************************************
 *             Structures
 ******************************************************/

#pragma pack(1)

typedef struct
{
#ifdef WWD_BUS_HAS_HEADER
                 wwd_bus_header_t  bus_header;
#endif /* ifdef WWD_BUS_HAS_HEADER */
                 uint32_t data[1];
} wwd_transfer_bytes_packet_t;

#pragma pack()

/******************************************************
 *             Function declarations
 ******************************************************/

/* Initialisation functions */
extern wwd_result_t wwd_bus_init                       ( void );
extern wwd_result_t wwd_bus_deinit                     ( void );

/* Device register access functions */
extern wwd_result_t wwd_bus_write_backplane_value      ( uint32_t address, uint8_t register_length, uint32_t value );
extern wwd_result_t wwd_bus_read_backplane_value       ( uint32_t address, uint8_t register_length, /*@out@*/ uint8_t* value );
extern wwd_result_t wwd_bus_write_register_value       ( wwd_bus_function_t function, uint32_t address, uint8_t value_length, uint32_t value );

/* Device data transfer functions */
extern wwd_result_t wwd_bus_send_buffer                ( wiced_buffer_t buffer );
extern wwd_result_t wwd_bus_transfer_bytes             ( wwd_bus_transfer_direction_t direction, wwd_bus_function_t function, uint32_t address, uint16_t size, /*@in@*/ /*@out@*/ wwd_transfer_bytes_packet_t* data );

/* Frame transfer function */
extern wwd_result_t wwd_bus_read_frame( /*@out@*/  wiced_buffer_t* buffer );

/* Bus energy saving functions */
extern wwd_result_t wwd_bus_allow_wlan_bus_to_sleep    ( void );
extern wwd_result_t wwd_bus_ensure_is_up               ( void );

extern wwd_result_t wwd_bus_poke_wlan                  ( void );
extern wwd_result_t wwd_bus_set_flow_control           ( uint8_t value );
extern wiced_bool_t wwd_bus_is_flow_controlled         ( void );
extern uint32_t     wwd_bus_packet_available_to_read   ( void );
extern wwd_result_t wwd_bus_ack_interrupt              ( uint32_t intstatus );
extern wwd_result_t wwd_bus_write_wifi_firmware_image  ( void );
extern wwd_result_t wwd_bus_write_wifi_nvram_image     ( void );
extern void         wwd_bus_init_backplane_window      ( void );
extern wwd_result_t wwd_bus_set_backplane_window       ( uint32_t addr );

#ifdef MFG_TEST_ALTERNATE_WLAN_DOWNLOAD
extern wwd_result_t external_write_wifi_firmware_and_nvram_image  ( void );
#endif /* ifdef MFG_TEST_ALTERNATE_WLAN_DOWNLOAD */

/******************************************************
 *             Global variables
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_BUS_PROTOCOL_INTERFACE_H_ */
