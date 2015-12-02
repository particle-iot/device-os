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

extern /*@null@*/ /*@exposed@*/ void* wwd_sdpcm_get_iovar_buffer      ( /*@special@*/ /*@out@*/ wiced_buffer_t* buffer, uint16_t data_length, const char* name )  /*@allocates *buffer@*/ /*@defines **buffer@*/;
extern /*@null@*/ /*@exposed@*/ void* wwd_sdpcm_get_ioctl_buffer      ( /*@special@*/ /*@out@*/ wiced_buffer_t* buffer, uint16_t data_length ) /*@allocates *buffer@*/  /*@defines **buffer@*/;
extern wwd_result_t                   wwd_sdpcm_send_ioctl            ( sdpcm_command_type_t type, uint32_t command, wiced_buffer_t send_buffer_hnd, /*@null@*/ /*@out@*/ wiced_buffer_t* response_buffer_hnd, wwd_interface_t interface ) /*@releases send_buffer_hnd@*/ ;
extern wwd_result_t                   wwd_sdpcm_send_iovar            ( sdpcm_command_type_t type, /*@only@*/ wiced_buffer_t send_buffer_hnd, /*@special@*/ /*@out@*/ /*@null@*/ wiced_buffer_t* response_buffer_hnd, wwd_interface_t interface )  /*@allocates *response_buffer_hnd@*/  /*@defines **response_buffer_hnd@*/;
extern void                           wwd_sdpcm_process_rx_packet     ( /*@only@*/ wiced_buffer_t buffer );
extern wwd_result_t                   wwd_sdpcm_init                  ( void );
extern void                           wwd_sdpcm_quit                  ( void ) /*@modifies internalState@*/;
extern wwd_result_t                   wwd_sdpcm_get_packet_to_send    ( /*@out@*/ wiced_buffer_t* buffer );
extern void                           wwd_sdpcm_update_credit         ( uint8_t* data );
extern uint8_t                        wWd_sdpcm_get_available_credits ( void );

/******************************************************
 *             Global variables
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_SDPCM_H */
