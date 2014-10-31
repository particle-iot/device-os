/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
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
