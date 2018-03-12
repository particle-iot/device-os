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

#include "wwd_rtos.h"

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************
 *                      Macros
 ******************************************************/

/* Use this macro to define an RTOS-aware interrupt handler where RTOS
 * primitives can be safely accessed.
 * Current port has no vectored interrupt controller, so single entry point
 * to all interrupts is used. And this entry is wrapped with RTOS-aware code,
 * so all interrupts are actually RTOS-aware.
 *
 * @usage:
 * WWD_RTOS_DEFINE_ISR( my_irq_handler )
 * {
 *     // Do something here
 * }
 */
#define WWD_RTOS_DEFINE_ISR( function )          PLATFORM_DEFINE_ISR( function )

/* Macro for mapping a function defined using WWD_RTOS_DEFINE_ISR
 * to an interrupt handler declared in
 * <Wiced-SDK>/WICED/platform/MCU/<Family>/platform_isr_interface.h
 *
 * @usage:
 * WWD_RTOS_MAP_ISR( my_irq, USART1_irq )
 */
#define WWD_RTOS_MAP_ISR( function, isr )        PLATFORM_MAP_ISR( function, isr )


/* Use this macro to define function which serves as ISR demuxer.
 * It is used when no vectored interrupt controller, and single
 * vector triggered for all interrupts.
 *
 * @usage:
 * WWD_RTOS_DEFINE_ISR_DEMUXER( my_irq_handler )
 * {
 *     // Do something here
 * }
 */
#define WWD_RTOS_DEFINE_ISR_DEMUXER( function )  PLATFORM_DEFINE_ISR( function )

/* Macro to declare that function is ISR demuxer.
 * Function has to be defined via WWD_RTOS_DEFINE_ISR_DEMUXER
 *
 * @usage:
 * WWD_RTOS_MAP_ISR_DEMUXER( my_irq_demuxer )
 */
#define WWD_RTOS_MAP_ISR_DEMUXER( function )     PLATFORM_MAP_ISR( function, _tx_platform_irq_demuxer )


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
