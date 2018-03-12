/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#ifndef INCLUDED_SPI_WWD_BUS_PROTOCOL_H
#define INCLUDED_SPI_WWD_BUS_PROTOCOL_H

#include <stdint.h>
#include "internal/wwd_thread_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *             Constants
 ******************************************************/

/******************************************************
 *             Structures
 ******************************************************/

typedef uint32_t wwd_bus_gspi_header_t;


#pragma pack(1)

typedef struct
{
    wwd_bus_gspi_header_t gspi_header;
} wwd_bus_header_t;

#pragma pack()

#define WWD_BUS_HAS_HEADER

#define WWD_BUS_HEADER_SIZE                     ( sizeof(wwd_bus_header_t) )

#define WWD_BUS_USE_STATUS_REPORT_SCHEME        ( 1 == 1 )

#define WWD_BUS_MAX_BACKPLANE_TRANSFER_SIZE     ( 64 ) /* Max packet size on F1 */
#define WWD_BUS_BACKPLANE_READ_PADD_SIZE        ( 4 )

/******************************************************
 *             Function declarations
 ******************************************************/

/******************************************************
 *             Global variables
 ******************************************************/

#ifdef __cplusplus
} /*extern "C" */
#endif

#endif /* ifndef INCLUDED_SPI_WWD_BUS_PROTOCOL_H */
