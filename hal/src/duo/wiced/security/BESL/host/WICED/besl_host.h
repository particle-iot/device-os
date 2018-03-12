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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "wiced_utilities.h"
#include "wwd_constants.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define BESL_ASSERT(string, x)

#define BESL_INFO(x)     WPRINT_SECURITY_INFO(x)
#define BESL_ERROR(x)    WPRINT_SECURITY_ERROR(x)
#define BESL_DEBUG(x)    WPRINT_SECURITY_DEBUG(x)

#ifdef BESL_HOST_IS_ALIGNED

#define BESL_READ_16(ptr)              ((uint16_t)(((uint8_t*)ptr)[0] + (((uint8_t*)ptr)[1] << 8)))
#define BESL_READ_16_BE(ptr)           ((uint16_t)(((uint8_t*)ptr)[1] + (((uint8_t*)ptr)[0] << 8)))
#define BESL_READ_32(ptr)              ((uint32_t)(((uint8_t*)ptr)[0] + ((((uint8_t*)ptr)[1] << 8)) + (((uint8_t*)ptr)[2] << 16) + (((uint8_t*)ptr)[3] << 24)))
#define BESL_READ_32_BE(ptr)           ((uint32_t)(((uint8_t*)ptr)[3] + ((((uint8_t*)ptr)[2] << 8)) + (((uint8_t*)ptr)[1] << 16) + (((uint8_t*)ptr)[0] << 24)))
#define BESL_WRITE_16(ptr, value)      do { ((uint8_t*)ptr)[0] = (uint8_t)value; ((uint8_t*)ptr)[1]=(uint8_t)(value>>8); } while(0)
#define BESL_WRITE_16_BE(ptr, value)   do { ((uint8_t*)ptr)[1] = (uint8_t)value; ((uint8_t*)ptr)[0]=(uint8_t)(value>>8); } while(0)
#define BESL_WRITE_32(ptr, value)      do { ((uint8_t*)ptr)[0] = (uint8_t)value; ((uint8_t*)ptr)[1]=(uint8_t)(value>>8); ((uint8_t*)ptr)[2]=(uint8_t)(value>>16); ((uint8_t*)ptr)[3]=(uint8_t)(value>>24); } while(0)
#define BESL_WRITE_32_BE(ptr, value)   do { ((uint8_t*)ptr)[3] = (uint8_t)value; ((uint8_t*)ptr)[2]=(uint8_t)(value>>8); ((uint8_t*)ptr)[1]=(uint8_t)(value>>16); ((uint8_t*)ptr)[0]=(uint8_t)(value>>24); } while(0)

#else

#define BESL_READ_16(ptr)            ((uint16_t*)ptr)[0]
#define BESL_READ_32(ptr)            ((uint32_t*)ptr)[0]
#define BESL_WRITE_16(ptr, value)    ((uint16_t*)ptr)[0] = value
#define BESL_WRITE_16_BE(ptr, value) ((uint16_t*)ptr)[0] = htobe16(value)
#define BESL_WRITE_32(ptr, value)    ((uint32_t*)ptr)[0] = value
#define BESL_WRITE_32_BE(ptr, value) ((uint32_t*)ptr)[0] = htobe32(value)

/* Prevents errors about strict aliasing */
static inline ALWAYS_INLINE uint16_t BESL_READ_16_BE(uint8_t* ptr_in)
{
    uint16_t* ptr = (uint16_t*)ptr_in;
    uint16_t  v   = *ptr;
    return (uint16_t)(((v&0x00FF) << 8) | ((v&0xFF00)>>8));
}

static inline ALWAYS_INLINE uint32_t BESL_READ_32_BE(uint8_t* ptr_in)
{
    uint32_t* ptr = (uint32_t*)ptr_in;
    uint32_t  v   = *ptr;
    return (uint32_t)(((v&0x000000FF) << 24) | ((v&0x0000FF00) << 8) | ((v&0x00FF0000) >> 8) | ((v&0xFF000000) >> 24));
}

#endif

/******************************************************
 *                    Constants
 ******************************************************/

#define BESL_NEVER_TIMEOUT    WICED_NEVER_TIMEOUT

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    BESL_EVENT_NO_EVENT,
    BESL_EVENT_TIMER_TIMEOUT,
    BESL_EVENT_ABORT_REQUESTED,
    BESL_EVENT_EAPOL_PACKET_RECEIVED,
    BESL_EVENT_RECEIVED_IDENTITY_REQUEST,
    BESL_EVENT_COMPLETE,
    SUPPLICANT_EVENT_PACKET_TO_SEND,
    WPS_EVENT_DISCOVER_COMPLETE,
    WPS_EVENT_ENROLLEE_ASSOCIATED,
    WPS_EVENT_RECEIVED_IDENTITY,
    WPS_EVENT_RECEIVED_WPS_START,
    WPS_EVENT_RECEIVED_EAPOL_START,
    WPS_EVENT_PBC_OVERLAP_NOTIFY_USER,
} besl_event_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef void*   besl_packet_t;

/******************************************************
 *                    Structures
 ******************************************************/

typedef union
{
    besl_packet_t           packet;
    uint32_t                value;
} supplicant_event_message_data_t;

typedef struct
{
    besl_event_t                    event_type;
    supplicant_event_message_data_t data;
} besl_event_message_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/* Packet functions */
besl_result_t besl_host_create_packet   ( besl_packet_t* packet, uint16_t size );
void          besl_host_consume_bytes   ( besl_packet_t* packet, int32_t number_of_bytes);
uint8_t*      besl_host_get_data        ( besl_packet_t packet );
besl_result_t besl_host_set_packet_size ( besl_packet_t packet, uint16_t packet_length );
uint16_t      besl_host_get_packet_size ( besl_packet_t packet );
void          besl_host_free_packet     ( besl_packet_t packet );
void          besl_host_send_packet     ( void* workspace, besl_packet_t packet, uint16_t size );
besl_result_t besl_host_leave           ( wwd_interface_t interface );
void          besl_host_start_timer     ( void* workspace, uint32_t timeout );
void          besl_host_stop_timer      ( void* workspace );
uint32_t      besl_host_get_current_time( void );
uint32_t      besl_host_get_timer       ( void* workspace );
besl_result_t besl_queue_message_packet ( void* workspace, besl_event_t type, besl_packet_t packet );
besl_result_t besl_queue_message_uint   ( void* workspace, besl_event_t type, uint32_t value );
void          besl_get_bssid            ( besl_mac_t* mac );
besl_result_t besl_set_passphrase       ( const uint8_t* security_key, uint8_t key_length );
void          besl_host_hex_bytes_to_chars( char* cptr, const uint8_t* bptr, uint32_t blen );
besl_result_t besl_queue_message        ( void* host_workspace, besl_event_t type, supplicant_event_message_data_t data );

#ifdef __cplusplus
} /*extern "C" */
#endif
