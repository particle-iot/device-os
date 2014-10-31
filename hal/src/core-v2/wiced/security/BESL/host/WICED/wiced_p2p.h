/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#include "besl_host.h"
#include "p2p_host_interface.h"

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

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

extern besl_result_t besl_p2p_init  ( p2p_workspace_t* workspace, const besl_p2p_device_detail_t* device_details );
extern besl_result_t besl_p2p_deinit( p2p_workspace_t* workspace );
extern besl_result_t besl_p2p_start ( p2p_workspace_t* workspace );
extern besl_result_t besl_p2p_stop  ( p2p_workspace_t* workspace );
extern besl_result_t besl_p2p_get_result( p2p_workspace_t* workspace );
extern besl_result_t besl_p2p_invite( p2p_workspace_t* workspace, p2p_discovered_device_t* device );
extern besl_result_t besl_p2p_get_discovered_peers( p2p_workspace_t* workspace, p2p_discovered_device_t** devices, uint8_t* device_count );

#ifdef __cplusplus
} /* extern "C" */
#endif
