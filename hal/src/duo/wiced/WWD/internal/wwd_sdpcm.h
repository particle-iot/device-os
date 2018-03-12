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
 *  Provides Wiced with function prototypes for IOCTL commands,
 *  and for communicating with the SDPCM module
 *
 */

#ifndef INCLUDED_WWD_SDPCM_H
#define INCLUDED_WWD_SDPCM_H

#include "wwd_buffer.h"
#include "wwd_constants.h"
#include "wwd_bus_protocol.h"
#include "chip_constants.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *             Constants
 ******************************************************/

/* CDC flag definition taken from bcmcdc.h */
#ifndef CDCF_IOC_SET
#define CDCF_IOC_SET                (0x02)      /** 0=get, 1=set cmd */
#endif /* ifndef CDCF_IOC_SET */

typedef enum sdpcm_command_type_enum
{
    SDPCM_GET = 0x00,
    SDPCM_SET = CDCF_IOC_SET
} sdpcm_command_type_t;


/* IOCTL swapping mode for Big Endian host with Little Endian wlan.  Default to off */
#ifdef IL_BIGENDIAN
wiced_bool_t swap = WICED_FALSE;
#define htod32(i) (swap?bcmswap32(i):i)
#define htod16(i) (swap?bcmswap16(i):i)
#define dtoh32(i) (swap?bcmswap32(i):i)
#define dtoh16(i) (swap?bcmswap16(i):i)
#else /* IL_BIGENDIAN */
#define htod32(i) ((uint32_t)(i))
#define htod16(i) ((uint16_t)(i))
#define dtoh32(i) ((uint32_t)(i))
#define dtoh16(i) ((uint16_t)(i))
#endif /* IL_BIGENDIAN */

/******************************************************
 *             Structures
 ******************************************************/

#define IOCTL_OFFSET ( sizeof(wwd_buffer_header_t) + 12 + 16 )

/******************************************************
 *             Function declarations
 ******************************************************/

extern /*@null@*/ /*@exposed@*/ void* wwd_sdpcm_get_iovar_buffer                    ( /*@special@*/ /*@out@*/ wiced_buffer_t* buffer, uint16_t data_length, const char* name )  /*@allocates *buffer@*/ /*@defines **buffer@*/;
extern /*@null@*/ /*@exposed@*/ void* wwd_sdpcm_get_ioctl_buffer                    ( /*@special@*/ /*@out@*/ wiced_buffer_t* buffer, uint16_t data_length ) /*@allocates *buffer@*/  /*@defines **buffer@*/;
extern wwd_result_t                   wwd_sdpcm_send_ioctl                          ( sdpcm_command_type_t type, uint32_t command, wiced_buffer_t send_buffer_hnd, /*@null@*/ /*@out@*/ wiced_buffer_t* response_buffer_hnd, wwd_interface_t interface ) /*@releases send_buffer_hnd@*/ ;
extern wwd_result_t                   wwd_sdpcm_send_iovar                          ( sdpcm_command_type_t type, /*@only@*/ wiced_buffer_t send_buffer_hnd, /*@special@*/ /*@out@*/ /*@null@*/ wiced_buffer_t* response_buffer_hnd, wwd_interface_t interface )  /*@allocates *response_buffer_hnd@*/  /*@defines **response_buffer_hnd@*/;
extern void                           wwd_sdpcm_process_rx_packet                   ( /*@only@*/ wiced_buffer_t buffer );
extern wwd_result_t                   wwd_sdpcm_init                                ( void );
extern void                           wwd_sdpcm_quit                                ( void ) /*@modifies internalState@*/;
extern wwd_result_t                   wwd_sdpcm_get_packet_to_send                  ( /*@out@*/ wiced_buffer_t* buffer );
extern void                           wwd_sdpcm_update_credit                       ( uint8_t* data );
extern uint8_t                        wwd_sdpcm_get_available_credits               ( void );
extern void                           wwd_update_host_interface_to_bss_index_mapping( wwd_interface_t interface, uint32_t bssid_index );


/******************************************************
 *             Global variables
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_SDPCM_H */
