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

#define RTC_ID                      1
#define RTC_IRQ_Priority            APP_IRQ_PRIORITY_LOWEST
#define RTC_TIMEOUT_SECONDS         (0xFFFFFF / 8)     // This value should be set as large as possible to save power.
#define DEFAULT_UNIX_TIME           946684800          // Default date/time to 2000/01/01 00:00:00 
#define CC_CHANNEL_TIME             0
#define CC_CHANNEL_ALARM            1

static const nrfx_rtc_t m_rtc = NRFX_RTC_INSTANCE(RTC_ID); 

static volatile time_t m_unix_time;
static volatile time_t m_unix_time_alarm;
static volatile time_t m_unix_time_last_set;

extern "C" void HAL_RTCAlarm_Handler(void);

static void rtc_handler(nrf_drv_rtc_int_type_t int_type)
{
    if (int_type == NRF_DRV_RTC_INT_COMPARE0) {
        m_unix_time += RTCx_TIMEOUT_SECONDS;
        if (m_unix_time_alarm == m_unix_time) {
            HAL_RTCAlarm_Handler();
        } else if ((m_unix_time_alarm > m_unix_time) && (m_unix_time_alarm - m_unix_time < RTCx_TIMEOUT_SECONDS)) {
            err_code = nrfx_rtc_cc_set(&m_rtc, CC_CHANNEL_TIME, rtc_counter + (m_unix_time_alarm - m_unix_time) * 8 true);
            APP_ERROR_CHECK(err_code);
        }
    } else if (int_type == NRF_DRV_RTC_INT_COMPARE1) {
        nrfx_rtc_cc_disable(&m_rtc, CC_CHANNEL_ALARM);
        HAL_RTCAlarm_Handler();
    }
}

void HAL_RTC_Configuration(void)
{
    uint32_t err_code;

    // Set default time
    m_unix_time = DEFAULT_UNIX_TIME;
    m_unix_time_alarm = 0;
    m_unix_time_last_set = 0;

    //Initialize RTC instance
    nrfx_rtc_config_t config = {                                                
        .prescaler          = 0xFFF,
        .interrupt_priority = RTC_IRQ_Priority,
        .reliable           = false,
        .tick_latency       = NRFX_RTC_US_TO_TICKS(NRFX_RTC_MAXIMUM_LATENCY_US, NRFX_RTC_DEFAULT_CONFIG_FREQUENCY)
    };

    err_code = nrfx_rtc_init(&m_rtc, &config, rtc_handler);
    APP_ERROR_CHECK(err_code);

    // Set compare channel to trigger interrupt after COMPARE_COUNTERTIME seconds
    err_code = nrfx_rtc_cc_set(&m_rtc, CC_CHANNEL_TIME, RTC_TIMEOUT_SECONDS * 8, true);
    APP_ERROR_CHECK(err_code);

    // Power on RTC instance
    nrfx_rtc_enable(&m_rtc);
}

void HAL_RTC_Set_UnixTime(time_t value)
{
    int32_t state = HAL_disable_irq();
    nrfx_rtc_counter_clear(&m_rtc);
    m_unix_time = m_unix_time_last_set = value;
    HAL_enable_irq(state);
}

time_t HAL_RTC_Get_UnixTime(void)
{
    return m_unix_time + nrfx_rtc_counter_get(&m_rtc) / 8;
}

void HAL_RTC_Set_UnixAlarm(time_t value)
{
    int32_t state = HAL_disable_irq();
    uint32_t rtc_counter = nrfx_rtc_counter_get(&m_rtc);
    m_unix_time_alarm = HAL_RTC_Get_UnixTime() + value;

    // Configure alarm time which is before next interrupt
    if (rtc_counter / 8 + value < RTCx_TIMEOUT_SECONDS) {
        err_code = nrfx_rtc_cc_set(&m_rtc, CC_CHANNEL_ALARM, rtc_counter + value * 8, true);
        APP_ERROR_CHECK(err_code);
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
