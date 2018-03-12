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

#include "wwd_buffer.h"
#include "wwd_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef wiced_buffer_t    wwd_eapol_packet_t;

typedef void (*eapol_packet_handler_t) (wiced_buffer_t buffer, wwd_interface_t interface);

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

extern wwd_result_t wwd_eapol_register_receive_handler  ( eapol_packet_handler_t eapol_packet_handler );
extern void         wwd_eapol_unregister_receive_handler( void );
extern void         wwd_eapol_receive_eapol_packet( /*@only@*/ wiced_buffer_t buffer, wwd_interface_t interface );
extern uint8_t*     wwd_eapol_get_eapol_data( wwd_eapol_packet_t packet );
extern uint16_t     wwd_get_eapol_packet_size( wwd_eapol_packet_t packet );

#ifdef __cplusplus
} /*extern "C" */
#endif
