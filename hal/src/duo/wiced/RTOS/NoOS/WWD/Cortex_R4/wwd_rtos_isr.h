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

#include "platform_isr.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

#define WWD_RTOS_DEFINE_ISR( function )          PLATFORM_DEFINE_ISR( function )

#define WWD_RTOS_MAP_ISR( function, isr )        PLATFORM_MAP_ISR( function, isr )

#define WWD_RTOS_MAP_ISR_DEMUXER( function )     PLATFORM_MAP_ISR( function, irq_vector_external_interrupt )

#if defined( __GNUC__ )

#define WWD_RTOS_DEFINE_ISR_DEMUXER( function ) \
        void function( void ); \
        __attribute__(( interrupt, used, section(IRQ_SECTION) )) void function( void )

#elif defined ( __IAR_SYSTEMS_ICC__ )

#define WWD_RTOS_DEFINE_ISR_DEMUXER( function ) \
        __irq __root void function( void ); \
        __irq __root void function( void )
#else

#define WWD_RTOS_DEFINE_ISR_DEMUXER( function ) \
        void function( void )

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


#ifdef __cplusplus
} /* extern "C" */
#endif
