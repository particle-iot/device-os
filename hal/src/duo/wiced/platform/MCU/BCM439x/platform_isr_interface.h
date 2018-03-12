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
 * Declares ISR prototypes for BCM439x MCU family
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
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

extern void NMIException        ( void );
extern void HardFaultException  ( void );
extern void MemManageException  ( void );
extern void BusFaultException   ( void );
extern void UsageFaultException ( void );
extern void SVC_irq             ( void );
extern void DebugMonitor        ( void );
extern void PENDSV_irq          ( void );
extern void SYSTICK_irq         ( void );
extern void Reserved016_irq     ( void );
extern void Reserved017_irq     ( void );
extern void Reserved018_irq     ( void );
extern void Reserved019_irq     ( void );
extern void Reserved020_irq     ( void );
extern void Reserved021_irq     ( void );
extern void Reserved022_irq     ( void );
extern void Reserved023_irq     ( void );
extern void Reserved024_irq     ( void );
extern void Reserved025_irq     ( void );
extern void Reserved026_irq     ( void );
extern void Reserved027_irq     ( void );
extern void Reserved028_irq     ( void );
extern void Reserved029_irq     ( void );
extern void Reserved030_irq     ( void );
extern void PTU1_irq            ( void );
extern void DmaDoneInt_irq      ( void );
extern void Reserved033_irq     ( void );
extern void Reserved034_irq     ( void );
extern void WAKEUP_irq          ( void );
extern void GPIOA_BANK0_irq     ( void );
extern void Reserved037_irq     ( void );
extern void Reserved038_irq     ( void );
extern void Reserved039_irq     ( void );
extern void Reserved040_irq     ( void );
extern void GPIOA_BANK1_irq     ( void );
extern void Reserved042_irq     ( void );
extern void Reserved043_irq     ( void );
extern void Reserved044_irq     ( void );
extern void Reserved045_irq     ( void );
extern void Reserved046_irq     ( void );
extern void Reserved047_irq     ( void );
extern void Reserved048_irq     ( void );
extern void Reserved049_irq     ( void );
extern void Reserved050_irq     ( void );
extern void Reserved051_irq     ( void );
extern void Reserved052_irq     ( void );
extern void Reserved053_irq     ( void );
extern void Reserved054_irq     ( void );
extern void Reserved055_irq     ( void );
extern void Reserved056_irq     ( void );
extern void Reserved057_irq     ( void );
extern void Reserved058_irq     ( void );
extern void Reserved059_irq     ( void );
extern void Reserved060_irq     ( void );
extern void WL2APPS_irq         ( void );
extern void WlanReady_irq       ( void );
extern void Reserved063_irq     ( void );
extern void PTU2_irq            ( void );

#ifdef __cplusplus
} /* extern "C" */
#endif

