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

extern "C" {
#include "ameba_soc.h"
#include "rtl8721d.h"
}
#include "km0_km4_ipc.h"
#include "sleep_hal.h"
#include "check.h"
#include "simple_pool_allocator.h"
#include "scope_guard.h"
#include "sleep_handler.h"

extern "C" void km4_clock_gate(void);
extern "C" void km4_clock_on(void);
extern "C" void SOCPS_SetWakepin(uint32_t PinIdx, uint32_t Polarity);

namespace {

// Note: We need to backup the sleep configuration on the KM0 side
// The KM4 SRAM is suspended after KM4 is clock gated
class SleepConfigShadow {
public:
    SleepConfigShadow()
            : shadow_(nullptr) {
    }
    ~SleepConfigShadow() {}

    int init(hal_sleep_config_t* config) {
        CHECK_TRUE(config != nullptr, SYSTEM_ERROR_INVALID_ARGUMENT);
        int ret = SYSTEM_ERROR_INTERNAL;

        SCOPE_GUARD ({
            if (ret != SYSTEM_ERROR_NONE) {
                reset();
            }
        });

        shadow_ = (hal_sleep_config_t*)staticPool_.alloc(sizeof(hal_sleep_config_t));
        CHECK_TRUE(shadow_, SYSTEM_ERROR_NO_MEMORY);
        memcpy(shadow_, (const void*)config, sizeof(hal_sleep_config_t));
        shadow_->wakeup_sources = nullptr; // config->wakeup_sources is now pointing to KM4 SRAM, we cannot assign it to shadow_.

        const hal_wakeup_source_base_t* source = config->wakeup_sources;
        while (source) {
            if (source->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
                CHECK(copyWakeupSource((const hal_wakeup_source_rtc_t*)source));
            } else if (source->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
                CHECK(copyWakeupSource((const hal_wakeup_source_gpio_t*)source));
            }
            source = source->next;
        }
        return ret = SYSTEM_ERROR_NONE;
    }

    void reset() {
        if (!shadow_) {
            return;
        }
        auto wakeupSource = shadow_->wakeup_sources;
        while (wakeupSource) {
            auto next = wakeupSource->next;
            staticPool_.free(wakeupSource);
            wakeupSource = next;
        }
        staticPool_.free(shadow_);
        shadow_ = nullptr;
    }

    hal_sleep_config_t* config() {
        return shadow_;
    }

private:
    
    template<typename T> int copyWakeupSource(const T* source) {
        auto wakeupSource = (T*)staticPool_.alloc(sizeof(T));
        CHECK_TRUE(wakeupSource, SYSTEM_ERROR_NO_MEMORY);
        DCache_Invalidate((uint32_t)source, sizeof(T));
        memcpy(wakeupSource, source, sizeof(T));
        wakeupSource->base.next = shadow_->wakeup_sources;
        shadow_->wakeup_sources = reinterpret_cast<hal_wakeup_source_base_t*>(wakeupSource);
        return SYSTEM_ERROR_NONE;
    }

    static uint8_t __attribute__((aligned(4))) staticBuffer_[1024];
    static AtomicSimpleStaticPool staticPool_;
    hal_sleep_config_t* shadow_;
};

uint8_t SleepConfigShadow::staticBuffer_[1024];
AtomicSimpleStaticPool SleepConfigShadow::staticPool_(staticBuffer_, sizeof(staticBuffer_));

volatile hal_sleep_config_t* sleepConfig = nullptr;
volatile uint16_t sleepReqId = KM0_KM4_IPC_INVALID_REQ_ID;
int32_t __attribute__((aligned(32))) sleepResult[8]; // 32 bytes for Dcache requirement
uint32_t sleepDuration = 0;
uint32_t sleepStart = 0;

SleepConfigShadow sleepConfigShadow;

void onSleepRequestReceived(km0_km4_ipc_msg_t* msg, void* context) {
    sleepConfig = (hal_sleep_config_t*)msg->data;
    if (msg->data_len != sizeof(hal_sleep_config_t)) {
        sleepConfig = nullptr;
    }
    sleepReqId = msg->req_id;
}

void configureSleepWakeupSource(const hal_sleep_config_t* config) {
    sleepDuration = 0;

#if HAL_PLATFORM_IO_EXTENSION && HAL_PLATFORM_MCP23S17
    bool ioExpanderIntConfigured = false;
#endif
    const hal_wakeup_source_base_t* source = config->wakeup_sources;
    while (source) {
        if (source->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            const hal_wakeup_source_rtc_t* rtcWakeup = (const hal_wakeup_source_rtc_t*)source;
            sleepDuration += rtcWakeup->ms;
            SOCPS_AONTimerCmd(DISABLE);
            SOCPS_AONTimer(rtcWakeup->ms);
            SOCPS_AONTimerCmd(ENABLE);
            sleepStart = SYSTIMER_GetPassTime(0);
        } else if (source->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            const hal_wakeup_source_gpio_t* gpioWakeup = (const hal_wakeup_source_gpio_t*)source;
            uint32_t rtlPin;
            InterruptMode mode;
#if HAL_PLATFORM_IO_EXTENSION && HAL_PLATFORM_MCP23S17
            const hal_pin_info_t* halPinMap = hal_pin_map();
            if (halPinMap[gpioWakeup->pin].type == HAL_PIN_TYPE_MCU)
#endif
            {
                rtlPin = hal_pin_to_rtl_pin(gpioWakeup->pin);
                mode = gpioWakeup->mode;
            }
#if HAL_PLATFORM_IO_EXTENSION && HAL_PLATFORM_MCP23S17
            else if (halPinMap[gpioWakeup->pin].type == HAL_PIN_TYPE_IO_EXPANDER && !ioExpanderIntConfigured) {
                ioExpanderIntConfigured = true;
                rtlPin = hal_pin_to_rtl_pin(IOE_INT);
                mode = FALLING;
            } else {
                source = source->next;
                continue;
            }
#endif
            GPIO_InitTypeDef  GPIO_InitStruct = {};
            GPIO_InitStruct.GPIO_Pin = rtlPin;
            GPIO_InitStruct.GPIO_Mode = GPIO_Mode_INT;
            GPIO_InitStruct.GPIO_ITTrigger = GPIO_INT_Trigger_EDGE;
            if (mode == RISING) {
                GPIO_InitStruct.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_HIGH;
            } else if (mode == FALLING) {
                GPIO_InitStruct.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW;
            } else { // mode == CHANGE
                GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
                GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
                GPIO_Init(&GPIO_InitStruct);
                uint32_t currLevel = GPIO_ReadDataBit(rtlPin);
                if (currLevel) {
                    GPIO_InitStruct.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_LOW;
                } else {
                    GPIO_InitStruct.GPIO_ITPolarity = GPIO_INT_POLARITY_ACTIVE_HIGH;
                }
                GPIO_InitStruct.GPIO_Mode = GPIO_Mode_INT;
            }
            GPIO_Init(&GPIO_InitStruct);
            GPIO_INTConfig(rtlPin, ENABLE);
        }
        source = source->next;
    }
}

void configureDeepSleepWakeupSource(const hal_sleep_config_t* config) {
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
            GPIO_InitTypeDef  GPIO_InitStruct = {};
            GPIO_InitStruct.GPIO_Pin = rtlPin;
            GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
            GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
            GPIO_Init(&GPIO_InitStruct);
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
            uint8_t polarity = 0;
            if (gpioWakeup->mode == RISING) {
				PAD_PullCtrl(rtlPin, GPIO_PuPd_DOWN);
                polarity = 1;
			} else if (gpioWakeup->mode == FALLING) {
				PAD_PullCtrl(rtlPin, GPIO_PuPd_UP);
                polarity = 0;
			} else { // gpioWakeup->mode == CHANGE
                uint32_t currLevel = GPIO_ReadDataBit(rtlPin);
                if (currLevel) {
                    PAD_PullCtrl(rtlPin, GPIO_PuPd_UP);
                    polarity = 0;
                } else {
                    PAD_PullCtrl(rtlPin, GPIO_PuPd_DOWN);
                    polarity = 1;
                }
            }
			Pinmux_Config(rtlPin, PINMUX_FUNCTION_WAKEUP);
			SOCPS_SetWakepin(wakeupPinIndex, polarity);
        }
        source = source->next;
    }
}

// Copy and paste from SOCPS_DeepSleep_RAM()
void enterDeepSleep() {
    // There is a user LED connected on D7, which is PA27 (SWD-DAT). There is an internal
    // pull-up resister on this I/O, which will turn on the user LED when enter the hibernate mode.
#if PLATFORM_ID == PLATFORM_P2
    if (HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SWD_PMUX_EN) & BIT_LSYS_SWD_PMUX_EN) {
        // Disable SWD
        Pinmux_Swdoff();
        // Configure it as input pulldown
        GPIOA_BASE->PORT[0].DDR &= (~(1 << 27));
        PAD_PullCtrl(27, GPIO_PuPd_DOWN);
    }
#endif

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

} // anonymous namespace


void sleepInit(void) {
    km0_km4_ipc_on_request_received(KM0_KM4_IPC_CHANNEL_GENERIC, KM0_KM4_IPC_MSG_SLEEP, onSleepRequestReceived, nullptr);
}

void sleepProcess(void) {
    // Handle sleep
    sleepResult[0] = SYSTEM_ERROR_NONE;
    if (sleepReqId != KM0_KM4_IPC_INVALID_REQ_ID) {
        if (!sleepConfig) {
            sleepResult[0] = SYSTEM_ERROR_BAD_DATA;
        } else {
            DCache_Invalidate((uint32_t)sleepConfig, sizeof(hal_sleep_config_t));
            if (sleepConfigShadow.init((hal_sleep_config_t*)sleepConfig) != SYSTEM_ERROR_NONE) {
                sleepResult[0] = SYSTEM_ERROR_NO_MEMORY;
            } else {
                sleepResult[0] = SYSTEM_ERROR_NONE;
            }
        }
        
        DCache_CleanInvalidate((uint32_t)sleepResult, sizeof(sleepResult));
        km0_km4_ipc_send_response(KM0_KM4_IPC_CHANNEL_GENERIC, sleepReqId, sleepResult, sizeof(sleepResult));
        if (sleepResult[0] == SYSTEM_ERROR_NONE) {
            /*
             * We're using clock gate on KM4 for stop mode and ultra-low power mode,
             * since the power gate will deinitialize PSRAM and we have to re-initialize
             * and copy code from flash to it. Power gate is used for hibernate mode.
             */
            auto config = sleepConfigShadow.config();
            if (config->mode == HAL_SLEEP_MODE_HIBERNATE) {
                configureDeepSleepWakeupSource(config);
                enterDeepSleep();
                // It should not reach here
            } else {
                // Copy and paste from km4_tickless_ipc_int()
                km4_sleep_type = SLEEP_CG;
                km4_clock_gate();

                // Copy and paste from freertos_pre_sleep_processing(), make KM0 enter clock gate state
                SOCPS_SWRLDO_Suspend(ENABLE);
                SOCPS_SleepInit();
                configureSleepWakeupSource(config);

#if PLATFORM_ID == PLATFORM_P2
                // There is a user LED connected on D7, which is PA27 (SWD-DAT). There is an internal
                // pull-up resister on this I/O, which will turn on the user LED when enter the stop/ulp mode.
                bool swdEnabled = false;
                if (HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SWD_PMUX_EN) & BIT_LSYS_SWD_PMUX_EN) {
                    // Disable SWD
                    Pinmux_Swdoff();
                    // Configure it as input pulldown
                    GPIOA_BASE->PORT[0].DDR &= (~(1 << 27));
                    PAD_PullCtrl(27, GPIO_PuPd_DOWN);
                    swdEnabled = true;
                }
#endif

                SOCPS_SleepCG();
                SOCPS_SWRLDO_Suspend(DISABLE);

                SOCPS_AONTimerCmd(DISABLE);

#if PLATFORM_ID == PLATFORM_P2
                if (swdEnabled) {
                    PAD_PullCtrl(27, GPIO_PuPd_UP);
                    uint32_t temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_SWD_PMUX_EN);	
                    temp |= BIT_LSYS_SWD_PMUX_EN;
                    HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_SWD_PMUX_EN, temp);
                }
#endif

                // Figure out the wakeup reason
                uint32_t wakeReason = 0;
                uint32_t sleepEnd = SYSTIMER_GetPassTime(0);
                if ((sleepDuration > 0) && ((sleepEnd - sleepStart) >= sleepDuration)) {
                    wakeReason |= BIT_HP_WEVT_TIMER_STS;
                }
                uint32_t intStatusA = HAL_READ32((uint32_t)GPIOA_BASE, 0x40);
                uint32_t intStatusB = HAL_READ32((uint32_t)GPIOB_BASE, 0x40);
                if (intStatusA != 0 || intStatusB != 0) {
                    wakeReason |= BIT_HP_WEVT_GPIO_STS;
                    // Let KM4 figure out the wakeup pin
                }

                sleepConfigShadow.reset();

                int priMask = HAL_disable_irq();
                // Copy and paste from km4_resume()
                km4_clock_on();
                HAL_WRITE32(SYSTEM_CTRL_BASE_HP, REG_HS_WAKE_EVENT_STATUS1, wakeReason);
                SOCPS_AudioLDO(ENABLE);
                HAL_enable_irq(priMask);
            }
        }
        sleepReqId = KM0_KM4_IPC_INVALID_REQ_ID;
        sleepConfig = nullptr;
    }
}
