/**
 ******************************************************************************
 * @file    rtc_hal.c
 * @author  Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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
#include "rtc_hal.h"
#include "stm32f10x_rtc.h"
#include "hw_config.h"
#include <stdlib.h>
#include "interrupts_hal.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static struct skew
{
  int8_t error;
  uint8_t ticks;
} skew;

static time_t HAL_RTC_Time_Last_Set = 0;

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

void HAL_RTC_Configuration(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Configure EXTI Line17(RTC Alarm) to generate an interrupt on rising edge */
    EXTI_ClearITPendingBit(EXTI_Line17);
    EXTI_InitStructure.EXTI_Line = EXTI_Line17;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* Enable the RTC Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = RTC_IRQ_PRIORITY;			//OLD: 0x01
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;								//OLD: 0x01
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Enable the RTC Alarm Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = RTCALARM_IRQ_PRIORITY;		//OLD: 0x01
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;								//OLD: 0x02
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Check if the StandBy flag is set */
    if(PWR_GetFlagStatus(PWR_FLAG_SB) != RESET)
    {
        /* System resumed from STANDBY mode */

        /* Clear StandBy flag */
        PWR_ClearFlag(PWR_FLAG_SB);

        /* Wait for RTC APB registers synchronisation */
        RTC_WaitForSynchro();

        /* No need to configure the RTC as the RTC configuration(clock source, enable,
	       prescaler,...) is kept after wake-up from STANDBY */
    }
    else
    {
        /* StandBy flag is not set */

        /* Enable LSE */
        RCC_LSEConfig(RCC_LSE_ON);

        /* Wait till LSE is ready */
        while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
        {
            //Do nothing
        }

        /* Select LSE as RTC Clock Source */
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

        /* Enable RTC Clock */
        RCC_RTCCLKCmd(ENABLE);

        /* Wait for RTC registers synchronization */
        RTC_WaitForSynchro();

        /* Wait until last write operation on RTC registers has finished */
        RTC_WaitForLastTask();

        /* Set RTC prescaler: set RTC period to 1sec */
        RTC_SetPrescaler(32767); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */

        /* Wait until last write operation on RTC registers has finished */
        RTC_WaitForLastTask();
    }

    /* Enable the RTC Second and RTC Alarm interrupt */
    RTC_ITConfig(RTC_IT_SEC | RTC_IT_ALR, ENABLE);

    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();
}

time_t HAL_RTC_Get_UnixTime(void)
{
  return (time_t)RTC_GetCounter();
}

void HAL_RTC_Set_UnixTime(time_t value)
{
    int32_t delta_error = HAL_RTC_Get_UnixTime() - value;
    if (delta_error > 127 || delta_error < -127)
    {
        // big delta, jump abruptly to the new time
        RTC_WaitForLastTask();
        RTC_SetCounter((uint32_t)value);
        RTC_WaitForLastTask();
    }
    else
    {
        // small delta, gradually skew toward the new time
        skew.error = delta_error;
        skew.ticks = 2 * abs(delta_error);
    }
    HAL_RTC_Time_Last_Set = value;
}

void HAL_RTC_Set_UnixAlarm(time_t value)
{
  RTC_ITConfig(RTC_IT_ALR, ENABLE);
  RTC_WaitForLastTask();
  /* Set the RTC Alarm */
  RTC_SetAlarm(RTC_GetCounter() + (uint32_t)value);
  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();
}

void HAL_RTC_Cancel_UnixAlarm(void)
{
    RTC_ITConfig(RTC_IT_ALR, DISABLE);
    RTC_WaitForLastTask();
    EXTI_ClearITPendingBit(EXTI_Line17);
    RTC_ClearITPendingBit(RTC_IT_ALR);
}

uint8_t HAL_RTC_Time_Is_Valid(void* reserved)
{
    uint8_t valid = 0;
    int32_t state = HAL_disable_irq();
    for (;;)
    {
        // 2001/01/01 00:00:00
        if (HAL_RTC_Get_UnixTime() < 978307200)
            break;

        if (HAL_RTC_Time_Last_Set && HAL_RTC_Get_UnixTime() < HAL_RTC_Time_Last_Set)
            break;

        valid = 1;
        break;
    }
    HAL_enable_irq(state);

    return valid;
}


/*******************************************************************************
 * Function Name  : HAL_RTC_Handler (Declared as weak in stm32_it.h)
 * Description    : This function handles RTC global interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void HAL_RTC_Handler(void)
{
    // Only intervene if we have an error to correct
    if (0 != skew.error && 0 < skew.ticks)
    {
        time_t now = HAL_RTC_Get_UnixTime();

        // By default, we set the clock 1 second forward
        time_t skew_step = 1;

        if (skew.error > 0)
        {
            // Error is positive, so we need to slow down
            if (skew.ticks / skew.error < 2)
            {
                // Don't let time go backwards!
                // Hold the clock still for a second
                skew_step--;
                skew.error--;
            }
        }
        else
        {
            // Error is negative, so we need to speed up
            if (skew.ticks / skew.error > -2)
            {
                // Skip a second forward
                skew_step++;
                skew.error++;
            }
        }

        skew.ticks--;
        HAL_RTC_Set_UnixTime(now + skew_step);
    }
}
