/*
 * Copyright (c) 2017 Particle Industries, Inc.  All rights reserved.
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

#include <stddef.h>
#include "watchdog_hal.h"
#include "platform_config.h"
#include "hal_irq_flag.h"
#include "stm32f2xx_wwdg.h"
#include "stm32f2xx_iwdg.h"
#include "timer_hal.h"

#define HAL_WATCHDOG_WWDG (0)
#define HAL_WATCHDOG_IWDG (1)

#define HAL_WATCHDOG_WWDG_CAPABILITIES             (HAL_WATCHDOG_CAPABILITY_WINDOWED | \
                                                     HAL_WATCHDOG_CAPABILITY_RECONFIGURABLE | \
                                                     HAL_WATCHDOG_CAPABILITY_NOTIFY | \
                                                     HAL_WATCHDOG_CAPABILITY_CPU_RESET | \
                                                     HAL_WATCHDOG_CAPABILITY_STOPPABLE)
#define HAL_WATCHDOG_WWDG_MIN_PERIOD_US            (137UL) // 136.53us
#define HAL_WATCHDOG_WWDG_MAX_PERIOD_US            (69910UL) // 69.91ms

#define HAL_WATCHDOG_IWDG_CAPABILITIES             (HAL_WATCHDOG_CAPABILITY_INDEPENDENT | \
                                                     HAL_WATCHDOG_CAPABILITY_CPU_RESET |\
                                                     HAL_WATCHDOG_CAPABILITY_RECONFIGURABLE)
#define HAL_WATCHDOG_IWDG_MIN_PERIOD_US            (125UL) // 125us
#define HAL_WATCHDOG_IWDG_MAX_PERIOD_US            (32700000UL) // 32.7s

typedef struct {
    uint8_t running;
    uint32_t capabilities;
    uint32_t period_us;
    uint32_t window_us;
    uint32_t counter;
    void (*notify)(void*);
    void* notify_arg;
    uint32_t last_kick;
} hal_watchdog_runtime_info_t;

static hal_watchdog_runtime_info_t s_watchdogs[HAL_WATCHDOG_COUNT] = {};

static uint32_t next_power_of_two(uint32_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

int hal_watchdog_query(int idx, hal_watchdog_info_t* info, void* reserved) {
    if (info == NULL) {
        return -1;
    }

    if (idx == HAL_WATCHDOG_WWDG) {
        info->capabilities = info->capabilities_required = HAL_WATCHDOG_WWDG_CAPABILITIES;
        info->min_period_us = HAL_WATCHDOG_WWDG_MIN_PERIOD_US;
        info->max_period_us = HAL_WATCHDOG_WWDG_MAX_PERIOD_US;
        return 0;
    } else if (idx == HAL_WATCHDOG_IWDG) {
        info->capabilities = info->capabilities_required = HAL_WATCHDOG_IWDG_CAPABILITIES;
        info->min_period_us = HAL_WATCHDOG_IWDG_MIN_PERIOD_US;
        info->max_period_us = HAL_WATCHDOG_IWDG_MAX_PERIOD_US;
        return 0;
    }

    return -1;
}

int hal_watchdog_configure(int idx, hal_watchdog_config_t* conf, void* reserved) {
    hal_watchdog_runtime_info_t* info = &s_watchdogs[idx];

    if (idx == HAL_WATCHDOG_WWDG) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG, ENABLE);

        RCC_ClocksTypeDef clocks = {};
        RCC_GetClocksFreq(&clocks);

        static const uint32_t wwdg_max_counter_value = 63;

        // Calculate prescaler
        // prescaler = PCLK1 / (counter / timeout)
        // Calculate with counter = 63 (maximum) for the best resolution
        uint32_t prescaler = 1 + ((clocks.PCLK1_Frequency / ((wwdg_max_counter_value * 1000000UL) / conf->period_us)) - 1) / 4096;

        if (prescaler <= 1) {
            prescaler = 1;
        } else if (prescaler <= 2) {
            prescaler = 2;
        } else if (prescaler <= 4) {
            prescaler = 4;
        } else if (prescaler > 4) {
            prescaler = 8;
        }

        // Calculate counter
        // counter = timeout * clock
        uint32_t clock = clocks.PCLK1_Frequency / 4096 / prescaler;
        uint32_t counter = (conf->period_us * clock) / 1000000;
        if (counter > wwdg_max_counter_value) {
            counter = wwdg_max_counter_value;
        }

        // Calculate window counter value
        uint32_t window = (conf->period_us - conf->window_us) * clock / 1000000;
        if (window > wwdg_max_counter_value) {
            window = wwdg_max_counter_value;
        }

        switch (prescaler) {
        case 1: {
            prescaler = WWDG_Prescaler_1;
            break;
        }
        case 2: {
            prescaler = WWDG_Prescaler_2;
            break;
        }
        case 4: {
            prescaler = WWDG_Prescaler_4;
            break;
        }
        case 8: {
            prescaler = WWDG_Prescaler_8;
            break;
        }
        }

        WWDG_SetPrescaler(prescaler);
        WWDG_SetWindowValue(0x40 | window);
        WWDG_SetCounter(0x40 | counter);

        info->period_us = ((counter + 1) * 1000000) / clock;
        info->window_us = ((window + 1) * 1000000) / clock;
        info->counter = counter;

        if (conf->capabilities & HAL_WATCHDOG_CAPABILITY_NOTIFY) {
            int32_t state = HAL_disable_irq();
            if (conf->notify != NULL) {
                info->notify = conf->notify;
                info->notify_arg = conf->notify_arg;
            }
            HAL_enable_irq(state);
        }
    } else if (idx == HAL_WATCHDOG_IWDG) {
        // Calculate prescaler
        // prescaler = lsi_clock / (counter / timeout)
        static const uint32_t lsi_frequency = 32000;
        static const uint32_t iwdg_max_counter_value = 0x0fff;
        // Calculate with counter = iwdg_max_counter_value for the best resolution
        uint32_t prescaler = (lsi_frequency / ((iwdg_max_counter_value * 1000000UL) / conf->period_us));

        prescaler = next_power_of_two(prescaler);

        if (prescaler < 4) {
            prescaler = 4;
        }

        if (prescaler > 256) {
            prescaler = 256;
        }

        // Calculate counter
        // counter = timeout * clock
        uint32_t iwdg_clock = lsi_frequency / prescaler;
        uint32_t counter = (conf->period_us * iwdg_clock) / 1000000;

        switch (prescaler) {
        case 4: {
            prescaler = IWDG_Prescaler_4;
            break;
        }
        case 8: {
            prescaler = IWDG_Prescaler_8;
            break;
        }
        case 16: {
            prescaler = IWDG_Prescaler_16;
            break;
        }
        case 32: {
            prescaler = IWDG_Prescaler_32;
            break;
        }
        case 64: {
            prescaler = IWDG_Prescaler_64;
            break;
        }
        case 128: {
            prescaler = IWDG_Prescaler_128;
            break;
        }
        case 256: {
            prescaler = IWDG_Prescaler_256;
            break;
        }
        }

        IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
        if (info->running) {
            while(IWDG_GetFlagStatus(IWDG_FLAG_RVU) == SET);
        }
        IWDG_SetReload(counter);
        if (info->running) {
            while(IWDG_GetFlagStatus(IWDG_FLAG_PVU) == SET);
        }
        IWDG_SetPrescaler(prescaler);
        IWDG_ReloadCounter();

        info->period_us = (counter * 1000000) / iwdg_clock;
        info->counter = counter;
    } else {
        return -1;
    }

    info->capabilities = conf->capabilities;

    return 0;
}

int hal_watchdog_start(int idx, hal_watchdog_config_t* conf, void* reserved) {
    hal_watchdog_runtime_info_t* info = &s_watchdogs[idx];

    if (info->running == 1) {
        return -1;
    }

    if (idx == HAL_WATCHDOG_WWDG) {
        if (conf != NULL) {
            hal_watchdog_configure(idx, conf, reserved);
        }

        WWDG_ClearFlag();
        WWDG_EnableIT();
        // Enable watchdog
        WWDG->CR |= WWDG_CR_WDGA;
        // Maximum priority
        NVIC_SetPriority(WWDG_IRQn, 0);
        NVIC_EnableIRQ(WWDG_IRQn);
        info->running = 1;
    } else if (idx == HAL_WATCHDOG_IWDG) {
        if (conf != NULL) {
            hal_watchdog_configure(idx, conf, reserved);
        }
        IWDG_Enable();
        info->running = 1;
    } else {
        return -1;
    }

    return 0;
}

int hal_watchdog_stop(int idx, void* reserved) {
    if (idx != HAL_WATCHDOG_WWDG) {
        return -1;
    }

    WWDG_DeInit();

    s_watchdogs[idx].running = 0;

    return 0;
}

int hal_watchdog_get_status(int idx, hal_watchdog_status_t* status, void* reserved) {
    if (idx >= HAL_WATCHDOG_COUNT) {
        return -1;
    }

    if (status == NULL || status->size == 0) {
        return -1;
    }

    const hal_watchdog_runtime_info_t* info = &s_watchdogs[idx];

    status->running = info->running;
    status->capabilities = info->capabilities;
    status->period_us = info->period_us;
    status->window_us = info->window_us;

    return 0;
}

int hal_watchdog_kick(int idx, void* reserved) {
    if ((idx == HAL_WATCHDOG_WWDG || idx < 0) && s_watchdogs[HAL_WATCHDOG_WWDG].running) {
        WWDG_SetCounter(0x40 | s_watchdogs[HAL_WATCHDOG_WWDG].counter);
        s_watchdogs[HAL_WATCHDOG_WWDG].last_kick = HAL_Timer_Get_Milli_Seconds();
    }
    if ((idx == HAL_WATCHDOG_IWDG || idx < 0) && s_watchdogs[HAL_WATCHDOG_IWDG].running) {
        IWDG_ReloadCounter();
        s_watchdogs[HAL_WATCHDOG_IWDG].last_kick = HAL_Timer_Get_Milli_Seconds();
    }

    return 0;
}

void WWDG_IRQHandler(void) {
    // hal_watchdog_kick(HAL_WATCHDOG_WWDG, NULL);
    if (s_watchdogs[HAL_WATCHDOG_WWDG].notify != NULL) {
        s_watchdogs[HAL_WATCHDOG_WWDG].notify(s_watchdogs[HAL_WATCHDOG_WWDG].notify_arg);
    }
    WWDG_ClearFlag();
}

