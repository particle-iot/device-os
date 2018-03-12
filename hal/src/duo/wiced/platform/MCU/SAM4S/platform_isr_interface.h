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
 * Declares ISR prototypes for STM32F2xx MCU family
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

extern void NMIException       (void);
extern void HardFaultException (void);
extern void MemManageException (void);
extern void BusFaultException  (void);
extern void UsageFaultException(void);
extern void SVC_irq            (void);
extern void DebugMonitor       (void);
extern void PENDSV_irq         (void);
extern void SYSTICK_irq        (void);
extern void SUPPLY_CTRL_irq    (void);
extern void RESET_CTRL_irq     (void);
extern void RTC_irq            (void);
extern void RTT_irq            (void);
extern void WDT_irq            (void);
extern void PMC_irq            (void);
extern void EEFC_irq           (void);
extern void UART0_irq          (void);
extern void UART1_irq          (void);
extern void SMC_irq            (void);
extern void PIO_CTRL_A_irq     (void);
extern void PIO_CTRL_B_irq     (void);
extern void PIO_CTRL_C_irq     (void);
extern void USART0_irq         (void);
extern void USART1_irq         (void);
extern void MCI_irq            (void);
extern void TWI0_irq           (void);
extern void TWI1_irq           (void);
extern void SPI_irq            (void);
extern void SSC_irq            (void);
extern void TC0_irq            (void);
extern void TC1_irq            (void);
extern void TC2_irq            (void);
extern void TC3_irq            (void);
extern void TC4_irq            (void);
extern void TC5_irq            (void);
extern void ADC_irq            (void);
extern void DAC_irq            (void);
extern void PWM_irq            (void);
extern void CRCCU_ir           (void);
extern void AC_irq             (void);
extern void USB_irq            (void);

#ifdef __cplusplus
} /* extern "C" */
#endif

