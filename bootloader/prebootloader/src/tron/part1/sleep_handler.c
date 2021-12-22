/*
 * Copyright (c) 2021 Particle Industries, Inc.  All rights reserved.
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

#include "stdbool.h"
#include "ameba_soc.h"
#include "rtl8721d.h"
#include "km0_km4_ipc.h"
#include "sleep_hal.h"
#include "check.h"

static volatile hal_sleep_config_t* sleepConfig = NULL;
static volatile uint16_t sleepReqId = KM0_KM4_IPC_INVALID_REQ_ID;
static int sleepResult = 0;

extern void km4_power_gate(void);
extern void km4_clock_gate(void);
extern void km4_clock_on(void);

static void onSleepRequestReceived(km0_km4_ipc_msg_t* msg, void* context) {
    sleepConfig = (hal_sleep_config_t*)msg->data;
    if (msg->data_len != sizeof(hal_sleep_config_t)) {
        sleepConfig = NULL;
    }
    sleepReqId = msg->req_id;
}

void sleepInit(void) {
    ConfigDebugClose = 1;
    km0_km4_ipc_on_request_received(KM0_KM4_IPC_CHANNEL_GENERIC, KM0_KM4_IPC_MSG_SLEEP, onSleepRequestReceived, NULL);
}

void sleepProcess(void) {
    // Handle sleep
    sleepResult = SYSTEM_ERROR_NONE;
    if (sleepReqId != KM0_KM4_IPC_INVALID_REQ_ID) {
        if (!sleepConfig) {
            sleepResult = SYSTEM_ERROR_BAD_DATA;
        } else {
            sleepResult = SYSTEM_ERROR_NONE;
        }
        km0_km4_ipc_send_response(KM0_KM4_IPC_CHANNEL_GENERIC, sleepReqId, &sleepResult, sizeof(sleepResult));
        if (sleepResult == SYSTEM_ERROR_NONE) {
            DCache_Invalidate((uint32_t)sleepConfig, sizeof(hal_sleep_config_t));
            DelayMs(1000);
            /*
             * We're using clock gate on KM4 for stop mode and ultra-low power mode,
             * since the power gate will deinitialize PSRAM and we have to re-initialize
             * and copy code from flash to it. Power gate is used for hibernate mode.
             */
            if (sleepConfig->mode == HAL_SLEEP_MODE_STOP || sleepConfig->mode == HAL_SLEEP_MODE_ULTRA_LOW_POWER) {
                km4_clock_gate();
                DiagPrintf("KM0: Successfully suspend KM4\n");

                km4_sleep_type = SLEEP_CG;

                SOCPS_SWRLDO_Suspend(ENABLE);
                SOCPS_SleepInit();

                SOCPS_SleepCG();

                SOCPS_SWRLDO_Suspend(DISABLE);
            } else {
                DiagPrintf("KM0: suspend KM4 using power gate\n");
                km4_power_gate();
                uint32_t temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_LP_BOOT_CFG);
                temp |= BIT_SOC_BOOT_WAKE_FROM_PS_HS;
                HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_LP_BOOT_CFG, temp);
                DiagPrintf("KM0: Successfully suspend KM4\n");

                SOCPS_DeepSleep_RAM();
                // It should not reach here
            }
            
            // Only if either stop mode or ultra-low power mode is used, we can get here
            int priMask = HAL_disable_irq();
            if (sleepConfig->mode == HAL_SLEEP_MODE_STOP || sleepConfig->mode == HAL_SLEEP_MODE_ULTRA_LOW_POWER) {
                DiagPrintf("KM0: km4_clock_on()\n");
                km4_clock_on();
            } else {
                // It should not reach here
            }
            // HAL_WRITE32(SYSTEM_CTRL_BASE_HP, REG_HS_WAKE_EVENT_STATUS1, km4_wake_event);
            SOCPS_AudioLDO(ENABLE);
            HAL_enable_irq(priMask);
        }
        sleepReqId = KM0_KM4_IPC_INVALID_REQ_ID;
        sleepConfig = NULL;
    }
}
