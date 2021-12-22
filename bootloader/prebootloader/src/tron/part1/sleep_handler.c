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

extern void km4_clock_gate(void);
extern void km4_clock_on(void);
extern void SOCPS_SetWakepin(uint32_t PinIdx, uint32_t Polarity);

static void onSleepRequestReceived(km0_km4_ipc_msg_t* msg, void* context) {
    sleepConfig = (hal_sleep_config_t*)msg->data;
    if (msg->data_len != sizeof(hal_sleep_config_t)) {
        sleepConfig = NULL;
    }
    sleepReqId = msg->req_id;
}

void sleepInit(void) {
    ConfigDebugClose = 0;
    km0_km4_ipc_on_request_received(KM0_KM4_IPC_CHANNEL_GENERIC, KM0_KM4_IPC_MSG_SLEEP, onSleepRequestReceived, NULL);
}

void configureDeepSleepWakeupSource(volatile hal_sleep_config_t* config) {
    // if enable AON wake event, DLPS may can't stop at WFI. so we Disable AON wake event
    SOCPS_SetWakeEvent(BIT_LP_WEVT_AON_MSK, DISABLE);
    // Enable AON timer and AON GPIO wake event
    SOCPS_SetWakeEventAON(BIT_AON_WAKE_TIM0_MSK | BIT_GPIO_WAKE_MSK, ENABLE);
    /* clear all wakeup pin first, and then enable by config */
	SOCPS_ClearWakePin();

    const hal_wakeup_source_base_t* source = config->wakeup_sources;
    while (source) {
        if (source->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            const hal_wakeup_source_rtc_t* rtcWakeup = (const hal_wakeup_source_rtc_t*)source;
            SOCPS_AONTimerCmd(DISABLE);
            SOCPS_AONTimer(rtcWakeup->ms);
            SOCPS_AONTimerCmd(ENABLE);
        } else if (source->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            const hal_wakeup_source_gpio_t* gpioWakeup = (const hal_wakeup_source_gpio_t*)source;
            uint32_t rtlPin = hal_pin_to_rtl_pin(gpioWakeup->pin);
            uint8_t wakeupPinIndex = 0xFF;
            for (uint8_t i = WAKUP_0; i <= WAKUP_3; i++) {
                for (uint8_t j = PINMUX_S0; j <= PINMUX_S2; j++) {
                    if (rtlPin == aon_wakepin[i][j]) {
                        wakeupPinIndex = i;
                        break;
                    }
                }
                if (wakeupPinIndex != 0xFF) {
                    break;
                }
            }
            // FIXME: level wakeup
            if(gpioWakeup->mode == RISING) {
				PAD_PullCtrl(rtlPin, GPIO_PuPd_DOWN);
			} else {
				PAD_PullCtrl(rtlPin, GPIO_PuPd_UP);
			}
			Pinmux_Config(rtlPin, PINMUX_FUNCTION_WAKEUP);
			SOCPS_SetWakepin(wakeupPinIndex, 1);
        }
        source = source->next;
    }
}

// Copy and paste from SOCPS_DeepSleep_RAM()
void enterDeepSleep() {
	/* pin power leakage */
	pinmap_deepsleep();
	
	/* clear wake event */
	SOCPS_ClearWakeEvent();

	/* Enable low power mode */
	FLASH_DeepPowerDown(ENABLE);

	/* set LP_LDO to 0.899V to fix RTC Calibration XTAL power leakage issue */
	uint32_t temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AON_LDO_CTRL1);
	temp &= ~(BIT_MASK_LDO_LP_ADJ << BIT_SHIFT_LDO_LP_ADJ);
	temp |= (0x09 << BIT_SHIFT_LDO_LP_ADJ); /* 0.899V, or ADC/XTAL will work abnormal in deepsleep mode */
	temp &= (~BIT_LDO_PSRAM_EN);/*Close PSRAM power, or DSLP current will increase*/
	HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_AON_LDO_CTRL1,temp);

	/* Shutdown LSPAD(no AON) at the end of DSLP flow */
	/* Fix ACUT power leakage issue */
	temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AON_PWR_CTRL);
	temp &= ~BIT_DSLP_SNOOZE_MODE_LSPAD_SHUTDOWN;
	HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_AON_PWR_CTRL, temp);
	
	/* set power mode */
	temp = HAL_READ32(SYSTEM_CTRL_BASE, REG_LP_PWRMGT_CTRL);
	temp &= ~BIT_LSYS_PMC_PMEN_SLEP;
	temp |= BIT_LSYS_PMC_PMEN_DSLP;
	HAL_WRITE32(SYSTEM_CTRL_BASE, REG_LP_PWRMGT_CTRL, temp);

	/* Wait CHIP enter deep sleep mode */
	__WFI();
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
                configureDeepSleepWakeupSource(sleepConfig);
                enterDeepSleep();
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
