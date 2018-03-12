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
 * Defines globally accessible powersave functions
 */
#pragma once
#include "platform_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/*@-exportlocal@*/

/**
 * Hook for RTOS for entering deep sleep mode
 * @param[in] sleep_ms : period in millisecond the MCU needs to sleep
 *
 * @return period in millisecond the MCU has slept
 */
extern uint32_t platform_power_down_hook( uint32_t sleep_ms );

/**
 * Hook for RTOS to ask MCU whether it want to enter power-down mode
 *
 * @return non-zero if MCU want RTOS enter power down mode
 */
extern int platform_power_down_permission( void );

/**
 * Hook for RTOS for entering idle mode
 */
extern void platform_idle_hook( void );


/*@+exportlocal@*/

#ifdef __cplusplus
} /* extern "C" */
#endif
