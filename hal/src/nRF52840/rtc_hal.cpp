/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "rtc_hal.h"
#include "nrfx_rtc.h"
#include "nrf_nvic.h"
#include "interrupts_hal.h"

#define RTCx                     NRF_RTC1
#define RTCx_IRQn                RTC1_IRQn
#define RTCx_IRQHandler          RTC1_IRQHandler
#define RTCx_IRQ_Priority        APP_IRQ_PRIORITY_LOWEST
#define RTCx_TIMEOUT_SECONDS     0xFFFFFF   // This value should be set as large as possible to save power.
#define DEFAULT_UNIX_TIME        946684800  // Default date/time to 2000/01/01 00:00:00 

static volatile time_t m_unix_time;
static volatile time_t m_unix_time_alarm;
static volatile time_t m_unix_time_last_set;

extern "C" void HAL_RTCAlarm_Handler(void);

void RTCx_IRQHandler(void)
{
    if (RTCx->EVENTS_COMPARE[0])
    {
        RTCx->EVENTS_COMPARE[0] = 0;
        RTCx->TASKS_CLEAR = 1;
        
        m_unix_time += RTCx_TIMEOUT_SECONDS;
        if (m_unix_time_alarm == m_unix_time)
        {
            HAL_RTCAlarm_Handler();
        }
        else if ((m_unix_time_alarm > m_unix_time) && (m_unix_time_alarm - m_unix_time < RTCx_TIMEOUT_SECONDS))
        {
            RTCx->EVTENSET = RTC_EVTENSET_COMPARE1_Msk;
            RTCx->INTENSET = RTC_INTENSET_COMPARE1_Msk;
            RTCx->CC[1] = (m_unix_time_alarm - m_unix_time) * 8;
        }
    }

    if (RTCx->EVENTS_COMPARE[1])
    {
        RTCx->EVENTS_COMPARE[1] = 0;
        RTCx->INTENCLR = RTC_INTENCLR_COMPARE1_Msk;
        RTCx->CC[1] = 0;

        HAL_RTCAlarm_Handler();
    }
}

void HAL_RTC_Configuration(void)
{
    // Start LFCLK first, we've done this already
    
    // Set default time
    m_unix_time = DEFAULT_UNIX_TIME;
    m_unix_time_alarm = 0;
    m_unix_time_last_set = 0;

    // Configure wakeup time for RTC 
    RTCx->PRESCALER = 0xFFF;
    RTCx->EVTENSET = RTC_EVTENSET_COMPARE0_Msk;
    RTCx->INTENSET = RTC_INTENSET_COMPARE0_Msk;
    RTCx->CC[0] = RTCx_TIMEOUT_SECONDS * 8;
    RTCx->TASKS_START = 1;
    sd_nvic_SetPriority(RTCx_IRQn, RTCx_IRQ_Priority);
    sd_nvic_EnableIRQ(RTCx_IRQn);  
}

void HAL_RTC_Set_UnixTime(time_t value)
{
    int32_t state = HAL_disable_irq();
    RTCx->TASKS_CLEAR = 1;  
    m_unix_time = m_unix_time_last_set = value;
    HAL_enable_irq(state);
}

time_t HAL_RTC_Get_UnixTime(void)
{
    return m_unix_time + RTCx->COUNTER / 8;
}

void HAL_RTC_Set_UnixAlarm(time_t value)
{
    int32_t state = HAL_disable_irq();
    uint32_t rtc_counter = RTCx->COUNTER;
    m_unix_time_alarm = HAL_RTC_Get_UnixTime() + value;

    // Configure alarm time which is before next interrupt
    if (rtc_counter / 8 + value < RTCx_TIMEOUT_SECONDS)
    {
        RTCx->EVTENSET = RTC_EVTENSET_COMPARE1_Msk;
        RTCx->INTENSET = RTC_INTENSET_COMPARE1_Msk;
        RTCx->CC[1] = rtc_counter + value * 8;
    }
    HAL_enable_irq(state);
}

void HAL_RTC_Cancel_UnixAlarm(void)
{
    m_unix_time_alarm = 0;
}

uint8_t HAL_RTC_Time_Is_Valid(void* reserved)
{
    uint8_t valid = 0;
    int32_t state = HAL_disable_irq();
    for (;;)
    {
        if (!(RTCx->INTENSET & RTC_INTENSET_COMPARE0_Msk))
            break;

        if (m_unix_time_last_set && HAL_RTC_Get_UnixTime() < m_unix_time_last_set)
            break;

        valid = 1;
        break;
    }
    HAL_enable_irq(state);

    return valid;
}
