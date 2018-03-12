/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#ifndef INCLUDED_WWD_NETWORK_H_
#define INCLUDED_WWD_NETWORK_H_

#include "nx_api.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************
 *             Function declarations
 ******************************************************/

/**
 * These functions define the "wiced network driver" for the NetX network stack
 *
 * A pointer to one of these functions must be passed to nx_ip_create when creating the
 * network interface.
 */

extern VOID wiced_sta_netx_duo_driver_entry ( NX_IP_DRIVER* driver );
extern VOID wiced_ap_netx_duo_driver_entry  ( NX_IP_DRIVER* driver );
extern VOID wiced_p2p_netx_duo_driver_entry ( NX_IP_DRIVER* driver );
extern VOID wiced_ethernet_netx_driver_entry( NX_IP_DRIVER* driver );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ifndef INCLUDED_WWD_NETWORK_H_ */
