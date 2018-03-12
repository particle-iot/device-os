/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#ifndef INCLUDED_WWD_BUFFER_H_
#define INCLUDED_WWD_BUFFER_H_

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *             Constants
 ******************************************************/

#define HOST_BUFFER_RELEASE_REMOVE_AT_FRONT_BUS_HEADER_SIZE    (sizeof(wwd_buffer_header_t) + MAX_SDPCM_HEADER_LENGTH)
#define HOST_BUFFER_RELEASE_REMOVE_AT_FRONT_FULL_SIZE          (HOST_BUFFER_RELEASE_REMOVE_AT_FRONT_BUS_HEADER_SIZE + WICED_ETHERNET_SIZE)

/******************************************************
 *             Structures
 ******************************************************/

struct NX_PACKET_STRUCT;

typedef struct NX_PACKET_STRUCT* wiced_buffer_t;

typedef struct
{
    wiced_buffer_t first;
    wiced_buffer_t last;
} wiced_buffer_fifo_t;

/******************************************************
 *             Function declarations
 ******************************************************/

/******************************************************
 *             Global variables
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_BUFFER_H_ */
