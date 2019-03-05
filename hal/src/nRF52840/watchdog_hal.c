/*
 ******************************************************************************
 *  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.
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
 ******************************************************************************
 */

#include "watchdog_hal.h"
#include "nrfx_wdt.h"
#include "logging.h"

static nrfx_wdt_channel_id m_channel_id;
static volatile bool watch_dog_initialized = false;

/**
 * @brief WDT events handler.
 */
static void wdt_event_handler(void)
{
    //NOTE: The max amount of time we can spend in WDT interrupt is two cycles of 32768[Hz] clock - after that, reset occurs
}

// backward compatibility for nRF52
bool HAL_watchdog_reset_flagged(void) 
{
    return false;
}

void HAL_Notify_WDT()
{
    if (watch_dog_initialized) {
        nrfx_wdt_channel_feed(m_channel_id);
    }
}

void HAL_Watchdog_Init(uint32_t timeout_tick_ms)
{
    uint32_t ret_code;

    //Configure WDT.
    nrfx_wdt_config_t config = {
        .behaviour          = (nrf_wdt_behaviour_t)NRFX_WDT_CONFIG_BEHAVIOUR,
        .reload_value       = timeout_tick_ms,
        .interrupt_priority = NRFX_WDT_CONFIG_IRQ_PRIORITY,
    };

    if (watch_dog_initialized == false)
    {
        ret_code = nrfx_wdt_init(&config, wdt_event_handler);
        SPARK_ASSERT(ret_code == NRF_SUCCESS);
        ret_code = nrfx_wdt_channel_alloc(&m_channel_id);
        SPARK_ASSERT(ret_code == NRF_SUCCESS);
        nrfx_wdt_enable();
        watch_dog_initialized = true;
    }
}
