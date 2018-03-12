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

#include <stdint.h>

#include "wwd_constants.h"

#include "queue.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *             Constants
 ******************************************************/

/******************************************************
 *             Structures
 ******************************************************/

typedef struct wiced_buffer_impl_s
{
    sq_entry_t      node;    /* must be first field */
    uint8_t*        buffer;
    uint16_t        size;
    uint16_t        offset;
    void*           pool;
    wwd_interface_t interface;
} wiced_buffer_impl_t;

typedef wiced_buffer_impl_t* wiced_buffer_t;

typedef struct sq_queue_s wiced_buffer_fifo_t;

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
