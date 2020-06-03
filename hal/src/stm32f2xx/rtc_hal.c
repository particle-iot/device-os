/**
 ******************************************************************************
 * @file    rtc_hal.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    18-Dec-2014
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

// For some reason on some platforms this is not defined
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif // _DEFAULT_SOURCE

#include "rtc_hal.h"
#include "stm32f2xx_rtc.h"
#include "hw_config.h"
#include "interrupts_hal.h"
#include "debug.h"
#include "system_error.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static time_t HAL_RTC_Time_Last_Set = 0;
/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

static hal_rtc_alarm_handler s_alarm_handler = NULL;
static void* s_alarm_handler_context = NULL;
static volatile bool s_alarm_fired = false;


void setRTCTime(RTC_TimeTypeDef* RTC_TimeStructure, RTC_DateTypeDef* RTC_DateStructure)
{
    /* Configure the RTC time register */
    if(RTC_SetTime(RTC_Format_BIN, RTC_TimeStructure) == ERROR)
    {
        /* RTC Set Time failed */
    }

    /* Configure the RTC date register */
    if(RTC_SetDate(RTC_Format_BIN, RTC_DateStructure) == ERROR)
    {
        /* RTC Set Date failed */
    }
}

/* Set date/time to 2000/01/01 00:00:00 */
void HAL_RTC_Initialize_UnixTime()
{
    RTC_TimeTypeDef RTC_TimeStructure;
    RTC_DateTypeDef RTC_DateStructure;

    /* Get calendar_time time struct values */
    RTC_TimeStructure.RTC_Hours = 0;
    RTC_TimeStructure.RTC_Minutes = 0;
    RTC_TimeStructure.RTC_Seconds = 0;

    /* Get calendar_time date struct values */
    RTC_DateStructure.RTC_WeekDay = 6;
    RTC_DateStructure.RTC_Date = 1;
    RTC_DateStructure.RTC_Month = 0;
    RTC_DateStructure.RTC_Year = 0;

    setRTCTime(&RTC_TimeStructure, &RTC_DateStructure);
}


void hal_rtc_init(void)
{
    RTC_InitTypeDef RTC_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    __IO uint32_t AsynchPrediv = 0x7F, SynchPrediv = 0xFF;

    /* Configure EXTI Line17(RTC Alarm) to generate an interrupt on rising edge */
    EXTI_ClearITPendingBit(EXTI_Line17);
    EXTI_InitStructure.EXTI_Line = EXTI_Line17;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* Enable the RTC Alarm Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = RTC_Alarm_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = RTC_Alarm_IRQ_PRIORITY;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Check if the StandBy flag is set */
    if(PWR_GetFlagStatus(PWR_FLAG_SB) != RESET)
    {
        /* System resumed from STANDBY mode */

        /* Wait for RTC APB registers synchronisation */
        RTC_WaitForSynchro();

        /* Clear the RTC Alarm Flag */
        RTC_ClearFlag(RTC_FLAG_ALRAF);

        /* Clear the EXTI Line 17 Pending bit (Connected internally to RTC Alarm) */
        EXTI_ClearITPendingBit(EXTI_Line17);

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

        /* RTC register configuration done only once */
        if (RTC_ReadBackupRegister(RTC_BKP_DR0) != 0xC1C1)
        {
            /* Configure the RTC data register and RTC prescaler */
            RTC_InitStructure.RTC_AsynchPrediv = AsynchPrediv;
            RTC_InitStructure.RTC_SynchPrediv = SynchPrediv;
            RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;

            /* Check on RTC init */
            if (RTC_Init(&RTC_InitStructure) != ERROR)
            {
                        /* Configure RTC Date and Time Registers if not set - Fixes #480, #580 */
                        /* Set date/time to 2000/01/01 00:00:00 */
                        HAL_RTC_Initialize_UnixTime();

                        /* Indicator for the RTC configuration */
                        RTC_WriteBackupRegister(RTC_BKP_DR0, 0xC1C1);
            }
        }
    }
}

int hal_rtc_get_time(struct timeval* tv, void* reserved) {
    if (!tv) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    RTC_TimeTypeDef RTC_TimeStructure;
    RTC_DateTypeDef RTC_DateStructure;

    /* Get the current Time and Date */
    RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
    RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);

    struct tm calendar_time = {0};

    /* Set calendar_time time struct values */
    calendar_time.tm_hour = RTC_TimeStructure.RTC_Hours;
    calendar_time.tm_min = RTC_TimeStructure.RTC_Minutes;
    calendar_time.tm_sec = RTC_TimeStructure.RTC_Seconds;

    /* Set calendar_time date struct values */
    calendar_time.tm_wday = RTC_DateStructure.RTC_WeekDay;
    calendar_time.tm_mday = RTC_DateStructure.RTC_Date;
    calendar_time.tm_mon = RTC_DateStructure.RTC_Month-1;
    // STM32F2 year is only 2-digit (00 - 99)
    calendar_time.tm_year = RTC_DateStructure.RTC_Year + 100;
    calendar_time.tm_isdst = -1;

    tv->tv_sec = mktime(&calendar_time);
    tv->tv_usec = 0;
    return 0;
}

int hal_rtc_set_time(const struct timeval* tv, void* reserved) {
    if (!tv) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    RTC_TimeTypeDef RTC_TimeStructure;
    RTC_DateTypeDef RTC_DateStructure;

    struct tm calendar_time = {};
    if (!gmtime_r(&tv->tv_sec, &calendar_time)) {
        return SYSTEM_ERROR_INTERNAL;
    }

    /* Get calendar_time time struct values */
    RTC_TimeStructure.RTC_Hours = calendar_time.tm_hour;
    RTC_TimeStructure.RTC_Minutes = calendar_time.tm_min;
    RTC_TimeStructure.RTC_Seconds = calendar_time.tm_sec;

    /* Get calendar_time date struct values */
    RTC_DateStructure.RTC_WeekDay = calendar_time.tm_wday;
    RTC_DateStructure.RTC_Date = calendar_time.tm_mday;
    RTC_DateStructure.RTC_Month = calendar_time.tm_mon+1;
    // STM32F2 year is only 2-digit (00 - 99)
    RTC_DateStructure.RTC_Year = calendar_time.tm_year % 100;

    int32_t state = HAL_disable_irq();
    setRTCTime(&RTC_TimeStructure, &RTC_DateStructure);
    HAL_RTC_Time_Last_Set = tv->tv_sec;
    HAL_enable_irq(state);

    return 0;
}

int hal_rtc_set_alarm(const struct timeval* tv, uint32_t flags, hal_rtc_alarm_handler handler, void* context, void* reserved) {
    if (!tv) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    struct timeval alarm = *tv;
    if (flags & HAL_RTC_ALARM_FLAG_IN) {
        struct timeval now;
        int r = hal_rtc_get_time(&now, NULL);
        if (r) {
            return r;
        }
        timeradd(&now, tv, &alarm);
    }

    struct tm alarm_time_tm;
    if (!gmtime_r(&alarm.tv_sec, &alarm_time_tm)) {
        return SYSTEM_ERROR_INTERNAL;
    }

    int32_t state = HAL_disable_irq();
    /* Disable the Alarm A */
    RTC_AlarmCmd(RTC_Alarm_A, DISABLE);

    s_alarm_handler_context = context;
    s_alarm_handler = handler;

    RTC_AlarmTypeDef RTC_AlarmStructure;
    RTC_AlarmStructure.RTC_AlarmTime.RTC_Hours = alarm_time_tm.tm_hour;
    RTC_AlarmStructure.RTC_AlarmTime.RTC_Minutes = alarm_time_tm.tm_min;
    RTC_AlarmStructure.RTC_AlarmTime.RTC_Seconds = alarm_time_tm.tm_sec;
    RTC_AlarmStructure.RTC_AlarmDateWeekDay = alarm_time_tm.tm_mday;
    RTC_AlarmStructure.RTC_AlarmDateWeekDaySel = RTC_AlarmDateWeekDaySel_Date;
    RTC_AlarmStructure.RTC_AlarmMask = RTC_AlarmMask_None;

    /* Configure the RTC Alarm A register */
    RTC_SetAlarm(RTC_Format_BIN, RTC_Alarm_A, &RTC_AlarmStructure);

    /* Enable the RTC Alarm A Interrupt */
    RTC_ITConfig(RTC_IT_ALRA, ENABLE);

    /* Enable the Alarm  A */
    RTC_AlarmCmd(RTC_Alarm_A, ENABLE);

    /* Clear RTC Alarm Flag */
    RTC_ClearFlag(RTC_FLAG_ALRAF);
    s_alarm_fired = false;
    HAL_enable_irq(state);

    struct timeval now;
    int r = hal_rtc_get_time(&now, NULL);
    if (r) {
        return r;
    }

    // Check if for some reason the alarm date is already in the past
    // and hasn't fired
    if (alarm.tv_sec < now.tv_sec && !s_alarm_fired) {
        hal_rtc_cancel_alarm();
        return SYSTEM_ERROR_TIMEOUT;
    }

    return 0;
}

void hal_rtc_cancel_alarm(void) {
    RTC_AlarmCmd(RTC_Alarm_A, DISABLE);
    RTC_ClearFlag(RTC_FLAG_ALRAF);
    RTC_WaitForSynchro();
    RTC_ClearITPendingBit(RTC_IT_ALRA);
    EXTI_ClearITPendingBit(EXTI_Line17);

    s_alarm_handler = NULL;
    s_alarm_handler_context = NULL;
    s_alarm_fired = false;
}

void RTC_Alarm_irq(void)
{
    if(RTC_GetITStatus(RTC_IT_ALRA) != RESET)
    {
        s_alarm_fired = true;
        if (s_alarm_handler) {
            s_alarm_handler(s_alarm_handler_context);
        }

        /* Clear EXTI line17 pending bit */
        EXTI_ClearITPendingBit(EXTI_Line17);

        /* Check if the Wake-Up flag is set */
        if(PWR_GetFlagStatus(PWR_FLAG_WU) != RESET)
        {
            /* Clear Wake Up flag */
            PWR_ClearFlag(PWR_FLAG_WU);
        }

        /* Clear RTC Alarm interrupt pending bit */
        RTC_ClearITPendingBit(RTC_IT_ALRA);
    }
}

bool hal_rtc_time_is_valid(void* reserved) {
    bool valid = false;

    struct timeval tv;
    if (hal_rtc_get_time(&tv, NULL)) {
        return valid;
    }

    int32_t state = HAL_disable_irq();
    for (;;)
    {
        RTC_DateTypeDef RTC_DateStructure;
        RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);

        if (!(RTC->ISR & RTC_ISR_INITS))
            break;

        if (RTC_DateStructure.RTC_Year == 0)
            break;

        if (HAL_RTC_Time_Last_Set && tv.tv_sec < HAL_RTC_Time_Last_Set)
            break;

        valid = true;
        break;
    }
    HAL_enable_irq(state);

    return valid;
}

void hal_rtc_set_unixtime_deprecated(time32_t value) {
    struct timeval tv = {
        .tv_sec = value,
        .tv_usec = 0
    };
    hal_rtc_set_time(&tv, NULL);
}

time32_t hal_rtc_get_unixtime_deprecated(void) {
    struct timeval tv = {};
    hal_rtc_get_time(&tv, NULL);
    return (time32_t)tv.tv_sec;
}
