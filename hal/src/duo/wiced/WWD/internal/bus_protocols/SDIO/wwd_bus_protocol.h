/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#ifndef INCLUDED_SDIO_WWD_BUS_PROTOCOL_H
#define INCLUDED_SDIO_WWD_BUS_PROTOCOL_H

#include "wwd_buffer.h"
#include "internal/wwd_thread_internal.h"
#include "platform/wwd_sdio_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *             Constants
 ******************************************************/

/******************************************************
 *             Structures
 ******************************************************/

#undef WWD_BUS_HAS_HEADER

#define WWD_BUS_HEADER_SIZE                     ( 0 )

#define WWD_BUS_USE_STATUS_REPORT_SCHEME        ( 1 == 0 )

#define WWD_BUS_MAX_BACKPLANE_TRANSFER_SIZE     ( WICED_PAYLOAD_MTU )
#define WWD_BUS_BACKPLANE_READ_PADD_SIZE        ( 0 )

/******************************************************
 *             Function declarations
 ******************************************************/


/******************************************************
 *             Global variables
 ******************************************************/

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* ifndef INCLUDED_SDIO_WWD_BUS_PROTOCOL_H */
