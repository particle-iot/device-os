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

#include "platform.h"

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

typedef void (*platform_button_state_change_callback_t)( platform_button_t id, wiced_bool_t new_state );

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/


extern platform_result_t  platform_button_init( platform_button_t button );
extern platform_result_t  platform_button_deinit( platform_button_t button );
extern platform_result_t  platform_button_enable( platform_button_t button );
extern platform_result_t  platform_button_disable( platform_button_t button );
extern wiced_bool_t       platform_button_get_value( platform_button_t button );
extern platform_result_t  platform_button_register_state_change_callback( platform_button_state_change_callback_t callback );

#ifdef __cplusplus
} /* extern "C" */
#endif
