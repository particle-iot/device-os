/**
 ******************************************************************************
 * @file    stm32_it.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    31-Oct-2014
 * @brief   Main Interrupt Service Routines.
 ******************************************************************************
  Copyright (c) 2013-14 Spark Labs, Inc.  All rights reserved.

  Copyright 2012 STMicroelectronics
  http://www.st.com/software_license_agreement_liberty_v2

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32_it.h"
#include "hw_config.h"
#include "usb_conf.h"
#include "usb_core.h"
#include "usbd_core.h"
#include "usbd_cdc_core.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
//HAL Interrupt Handlers defined in xxx_hal.c files
void HAL_SysTick_Handler(void) __attribute__ ((weak));

/* Extern variables and function prototypes ----------------------------------*/
extern __IO uint16_t BUTTON_DEBOUNCED_TIME[];

void linkme(void)
{
}

/******************************************************************************/
/*             Cortex-M Processor Exceptions Handlers                         */
/******************************************************************************/

/**
 * @brief   This function handles NMI exception.
 * @param  None
 * @retval None
 */
void NMI_Handler(void)
{
}

/**
 * @brief  This function handles Hard Fault exception.
 * @param  None
 * @retval None
 */
void HardFault_Handler(void)
{
    /* Go to infinite loop when Hard Fault exception occurs */
    while (1)
    {
    }
}

/**
 * @brief  This function handles Memory Manage exception.
 * @param  None
 * @retval None
 */
void MemManage_Handler(void)
{
    /* Go to infinite loop when Memory Manage exception occurs */
    while (1)
    {
    }
}

/**
 * @brief  This function handles Bus Fault exception.
 * @param  None
 * @retval None
 */
void BusFault_Handler(void)
{
    /* Go to infinite loop when Bus Fault exception occurs */
    while (1)
    {
    }
}

/**
 * @brief  This function handles Usage Fault exception.
 * @param  None
 * @retval None
 */
void UsageFault_Handler(void)
{
    /* Go to infinite loop when Usage Fault exception occurs */
    while (1)
    {
    }
}

/**
 * @brief  This function handles SVCall exception.
 * @param  None
 * @retval None
 */
void SVC_Handler(void)
{
}

/**
 * @brief  This function handles Debug Monitor exception.
 * @param  None
 * @retval None
 */
void DebugMon_Handler(void)
{
}

/**
 * @brief  This function handles PendSVC exception.
 * @param  None
 * @retval None
 */
void PendSV_Handler(void)
{
}

/**
 * @brief  This function handles SysTick Handler.
 * @param  None
 * @retval None
 */
void SysTick_Handler(void)
{
    System1MsTick();

    if (TimingDelay != 0x00)
    {
        TimingDelay--;
    }

    if(HAL_SysTick_Handler)
    {
        HAL_SysTick_Handler();
    }
}

/**
 * @brief  This function handles EXTI2 Handler.
 * @param  None
 * @retval None
 */
void EXTI2_IRQHandler(void)
{
    if (EXTI_GetITStatus(BUTTON1_EXTI_LINE ) != RESET)
    {
        /* Clear the EXTI line pending bit */
        EXTI_ClearITPendingBit(BUTTON1_EXTI_LINE );

        BUTTON_DEBOUNCED_TIME[BUTTON1] = 0x00;

        /* Disable BUTTON1 Interrupt */
        BUTTON_EXTI_Config(BUTTON1, DISABLE);

        /* Enable TIM2 CC1 Interrupt */
        TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);
    }
}

/**
 * @brief  This function handles TIM2 Handler.
 * @param  None
 * @retval None
 */
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_CC1) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_CC1);

        if (BUTTON_GetState(BUTTON1) == BUTTON1_PRESSED)
        {
            BUTTON_DEBOUNCED_TIME[BUTTON1] += BUTTON_DEBOUNCE_INTERVAL;
        }
        else
        {
            /* Disable TIM2 CC1 Interrupt */
            TIM_ITConfig(TIM2, TIM_IT_CC1, DISABLE);

            /* Enable BUTTON1 Interrupt */
            BUTTON_EXTI_Config(BUTTON1, ENABLE);
        }
    }
}

/******************************************************************************/
/*                 STM32Fxxx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32fxxx.s).                                               */
/******************************************************************************/

/**
 * @brief  This function handles PPP interrupt request.
 * @param  None
 * @retval None
 */
/*void PPP_IRQHandler(void)
{
}*/
