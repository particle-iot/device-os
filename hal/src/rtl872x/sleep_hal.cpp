/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHAN'TABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "sleep_hal.h"
#include "gpio_hal.h"
#include "usb_hal.h"
#include "usart_hal.h"
#include "i2c_hal.h"
#include "spi_hal.h"
#include "adc_hal.h"
#include "pwm_hal.h"
#include "flash_common.h"
#include "exflash_hal.h"
#include "rtc_hal.h"
#include "timer_hal.h"
#include "interrupts_hal.h"
#include "concurrent_hal.h"
#include "ble_hal.h"
#include "check.h"
#include "radio_common.h"
#if HAL_PLATFORM_EXTERNAL_RTC
#include "exrtc_hal.h"
#endif
#include "spark_wiring_vector.h"
#include "service_debug.h"

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
#if HAL_PLATFORM_MCP23S17
#include "mcp23s17.h"
#endif
#endif
extern "C" {
#include "rtl8721d.h"
}
#include "km0_km4_ipc.h"
#include "backup_ram_hal.h"
#include "dct.h"
#include "dct_hal.h"
#include "align_util.h"
#include "delay_hal.h"

using namespace particle;

class SleepClass {
public:
    int validateSleepConfig(const hal_sleep_config_t* config) {
        CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);
        // Checks the sleep mode.
        if (config->mode == HAL_SLEEP_MODE_NONE || config->mode >= HAL_SLEEP_MODE_MAX) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        // Checks the wakeup sources
        auto wakeupSource = config->wakeup_sources;
        uint16_t valid = 0;
        while (wakeupSource) {
            if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
                CHECK(validateGpioWakeupSource(config->mode, reinterpret_cast<const hal_wakeup_source_gpio_t*>(wakeupSource)));
            } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
                CHECK(validateRtcWakeupSource(config->mode, reinterpret_cast<const hal_wakeup_source_rtc_t*>(wakeupSource)));
            } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_USART) {
                CHECK(validateUsartWakeupSource(config->mode, reinterpret_cast<const hal_wakeup_source_usart_t*>(wakeupSource)));
            } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_LPCOMP) {
                CHECK(validateLpcompWakeupSource(config->mode, reinterpret_cast<const hal_wakeup_source_lpcomp_t*>(wakeupSource)));
            } else {
                return SYSTEM_ERROR_NOT_SUPPORTED;
            }
            valid++;
            wakeupSource = wakeupSource->next;
        }
        // At least one wakeup source should be configured for stop and ultra-low power mode.
        if ((config->mode == HAL_SLEEP_MODE_STOP || config->mode == HAL_SLEEP_MODE_ULTRA_LOW_POWER) && (!config->wakeup_sources || !valid)) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        return SYSTEM_ERROR_NONE;
    }

    /*
     * We're using clock gate on KM4 for stop mode and ultra-low power mode,
     * since the power gate will deinitialize PSRAM and we have to re-initialize
     * and copy code from flash to it. Power gate is used for hibernate mode.
     */
    __attribute__((section(".ram.sleep"), optimize("O0")))
    int enter(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeupReason) {
        CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);
        memcpy(&alignedConfig_.config, config, sizeof(hal_sleep_config_t));

        bool advertising = hal_ble_gap_is_advertising(nullptr) ||
                           hal_ble_gap_is_connecting(nullptr, nullptr) ||
                           hal_ble_gap_is_connected(nullptr, nullptr);
        hal_ble_stack_deinit(nullptr);
        // The delay is essential to make sure the resources are successfully freed.
        HAL_Delay_Milliseconds(2000);

        HAL_USB_Detach();

        if (config->mode == HAL_SLEEP_MODE_HIBERNATE) {
            SYSTEM_FLAG(entered_hibernate) = 1;
            dct_write_app_data(&system_flags, DCT_SYSTEM_FLAGS_OFFSET, DCT_SYSTEM_FLAGS_SIZE);
            /* Backup (sic!) backup RAM into flash */
            hal_backup_ram_sync();
        }

        for (int usart = 0; usart < HAL_PLATFORM_USART_NUM; usart++) {
            if (hal_usart_is_enabled(static_cast<hal_usart_interface_t>(usart))) {
                hal_usart_flush(static_cast<hal_usart_interface_t>(usart));
            }
        }

        // Disable thread scheduling
        os_thread_scheduling(false, nullptr);
        // Disable SysTick
        SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

        if (notifyKm0AndSleep() != SYSTEM_ERROR_NONE) {
            SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
            os_thread_scheduling(true, nullptr);
            return SYSTEM_ERROR_INTERNAL;
        }

        int priMask = __get_PRIMASK();
        __disable_irq();
        __DSB();
        __ISB();

        hal_interrupt_suspend();

        // WARNING: any function/data being called/accessed in/after PSRAM being suspended should reside in SRAM
        ICache_Disable();
        if (config->mode == HAL_SLEEP_MODE_HIBERNATE) {
            suspendPsram(false);
        } else {
            suspendPsram(true);
        }

        DelayMs(20);
        __SEV(); // signal event, also signal to KM0
        __WFE(); // clear event, immediately exits
        __WFE(); // sleep, waiting for event
        
        // Only if either stop mode or ultra-low power mode is used, we can get here,
        // which means that the PSRAM retention is enabled, we don't need to re-initialized PSRAM
        ICache_Enable();
        resumePsram();

        // Read the int status before enabling interrupt.
        uint32_t intStatusA = HAL_READ32((uint32_t)GPIOA_BASE, 0x40);
        uint32_t intStatusB = HAL_READ32((uint32_t)GPIOB_BASE, 0x40);
        if (wakeupReason) {
            uint32_t wakeEvent = HAL_READ32(SYSTEM_CTRL_BASE, REG_HS_WAKE_EVENT_STATUS1);
            if (wakeEvent & BIT_HP_WEVT_TIMER_STS) {
                constructWakeupReason((hal_wakeup_source_rtc_t**)wakeupReason, HAL_WAKEUP_SOURCE_TYPE_RTC, nullptr);
            } else if (wakeEvent & BIT_HP_WEVT_GPIO_STS) {
                const hal_wakeup_source_base_t* source = alignedConfig_.config.wakeup_sources;
                uint16_t wakeupPin = PIN_INVALID;
                while (source) {
                    if (source->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
                        const hal_wakeup_source_gpio_t* gpioWakeup = (const hal_wakeup_source_gpio_t*)source;
                        const hal_pin_info_t* pinInfo = hal_pin_map() + gpioWakeup->pin;
                        if ((pinInfo->gpio_port == RTL_PORT_A && (intStatusA & (0x01 << pinInfo->gpio_pin)))
                            || (pinInfo->gpio_port == RTL_PORT_B && (intStatusB & (0x01 << pinInfo->gpio_pin)))) {
                            wakeupPin = gpioWakeup->pin;
                            break;
                        }
                    }
                    source = source->next;
                }
                constructWakeupReason((hal_wakeup_source_gpio_t**)wakeupReason, HAL_WAKEUP_SOURCE_TYPE_GPIO, &wakeupPin);
            }
        }

        hal_interrupt_restore();

        HAL_USB_Attach();

        if ((priMask & 1) == 0) {
            __enable_irq();
        }
        __DSB();
        __ISB();

        // Clear int status
        HAL_WRITE32((uint32_t)GPIOA_BASE, 0x4C, 0xFFFF);
        HAL_WRITE32((uint32_t)GPIOB_BASE, 0x4C, 0xFFFF);

        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
        os_thread_scheduling(true, nullptr);

        if (hal_ble_stack_init(nullptr) == SYSTEM_ERROR_NONE) {
            if (advertising) {
                hal_ble_gap_start_advertising(nullptr);
            }
        }

        return SYSTEM_ERROR_NONE;
    }

    static SleepClass* getInstance() {
        static SleepClass sleep;
        static_assert(is_member_aligned_to<32>(sleep.alignedConfig_), "alignof doesn't match");
        return &sleep;
    }

private:
    struct AlignedSleepConfig {
        hal_sleep_config_t config;
        uint32_t padding[4];
    };

    SleepClass()
            : ipcResult_(SYSTEM_ERROR_INTERNAL) {
    }
    ~SleepClass() = default;

    int validateGpioWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_gpio_t* gpio) {
        switch(gpio->mode) {
            case RISING:
            case FALLING:
            case CHANGE: {
                break;
            }
            default: {
                return SYSTEM_ERROR_INVALID_ARGUMENT;
            }
        }
        if (gpio->pin >= TOTAL_PINS) {
            return SYSTEM_ERROR_LIMIT_EXCEEDED;
        }
        if (mode == HAL_SLEEP_MODE_HIBERNATE && gpio->pin != WKP) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        return SYSTEM_ERROR_NONE;
    }

    int validateLpcompWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_lpcomp_t* lpcomp) {
        if (hal_pin_validate_function(lpcomp->pin, PF_ADC) != PF_ADC) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if (lpcomp->trig > HAL_SLEEP_LPCOMP_CROSS) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        if (mode == HAL_SLEEP_MODE_HIBERNATE) {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
        // TODO
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }

    int validateRtcWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_rtc_t* rtc) {
        if ((rtc->ms / 1000) == 0) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
        return SYSTEM_ERROR_NONE;
    }

    int validateUsartWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_usart_t* usart) {
        if (!hal_usart_is_enabled(usart->serial)) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        if (mode == HAL_SLEEP_MODE_HIBERNATE) {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
        // TODO
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }

    template<typename T> int constructWakeupReason(T** wakeupReason, hal_wakeup_source_type_t type, void* data) {
        auto wakeupSource = (T*)malloc(sizeof(T));
        if (wakeupSource) {
            wakeupSource->base.size = sizeof(T);
            wakeupSource->base.version = HAL_SLEEP_VERSION;
            wakeupSource->base.type = type;
            wakeupSource->base.next = nullptr;
            *wakeupReason = wakeupSource;
            if (type == HAL_WAKEUP_SOURCE_TYPE_GPIO && data != nullptr) {
                ((hal_wakeup_source_gpio_t*)wakeupSource)->pin = *((uint16_t*)data);
            }
        } else {
            return SYSTEM_ERROR_NO_MEMORY;
        }
        return SYSTEM_ERROR_NONE;
    }

    int notifyKm0AndSleep() {
        ipcResult_ = SYSTEM_ERROR_INTERNAL;
        DCache_CleanInvalidate((uint32_t)&alignedConfig_.config, sizeof(hal_sleep_config_t));
        int ret = km0_km4_ipc_send_request(KM0_KM4_IPC_CHANNEL_GENERIC, KM0_KM4_IPC_MSG_SLEEP, &alignedConfig_.config,
                sizeof(hal_sleep_config_t), onKm0RespReceived, this);
        if (ret != SYSTEM_ERROR_NONE || ipcResult_ != SYSTEM_ERROR_NONE) {
            return SYSTEM_ERROR_INTERNAL;
        }
        return SYSTEM_ERROR_NONE;
    }

    __attribute__((section(".ram.sleep"), optimize("O0")))
    void suspendPsram(bool retention) {
        uint32_t temp;
        if (retention) {
            uint32_t temps;
            uint8_t * psram_ca;
            uint32_t PSRAM_datard;
            uint32_t PSRAM_datawr;
            uint8_t PSRAM_CA[6];
            psram_ca = &PSRAM_CA[0];
            /* set psram enter halfsleep mode */
            PSRAM_CTRL_CA_Gen(psram_ca, (BIT11 | BIT0), PSRAM_LINEAR_TYPE, PSRAM_REG_SPACE, PSRAM_READ_TRANSACTION);
            PSRAM_CTRL_DPin_Reg(psram_ca, &PSRAM_datard, PSRAM_READ_TRANSACTION);
            PSRAM_datard |= BIT5;
            temp = PSRAM_datard & 0xff;
            temps = (PSRAM_datard >> 8) & 0xff;
            PSRAM_datawr = ((temp << 8) | (temp << 24)) | (temps | (temps << 16));
            PSRAM_CTRL_CA_Gen(psram_ca, (BIT11 | BIT0), PSRAM_LINEAR_TYPE, PSRAM_REG_SPACE, PSRAM_WRITE_TRANSACTION);
            PSRAM_CTRL_DPin_Reg(psram_ca, &PSRAM_datawr, PSRAM_WRITE_TRANSACTION);
        } else {
            temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AON_LDO_CTRL1);
            temp &= ~(BIT_LDO_PSRAM_EN);
            HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_AON_LDO_CTRL1, temp);	
        }
    }
    
    __attribute__((section(".ram.sleep"), optimize("O0")))
    void resumePsram() {
        /* dummy read to make psram exit half sleep mode */
        uint32_t temp = HAL_READ32(PSRAM_BASE, 0);
        (void)temp;
        /* wait psram exit half sleep mode */
        DelayUs(100);
        DCache_Invalidate((uint32_t)PSRAM_BASE, 32);
    }

    static void onKm0RespReceived(km0_km4_ipc_msg_t* msg, void* context) {
        if (!msg || !context) {
            return;
        }
        auto instance = (SleepClass*)context;
        if (!msg->data) {
            instance->ipcResult_ = SYSTEM_ERROR_BAD_DATA;
        } else {
            DCache_Invalidate((uint32_t)msg->data, sizeof(int));
            instance->ipcResult_ = ((int32_t*)(msg->data))[0];
        }
    }

    volatile int ipcResult_;
    AlignedSleepConfig __attribute__((aligned(32))) alignedConfig_;
};

int hal_sleep_validate_config(const hal_sleep_config_t* config, void* reserved) {
    return SleepClass::getInstance()->validateSleepConfig(config);
}

int hal_sleep_enter(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeup_source, void* reserved) {
    // Check it again just in case.
    CHECK(SleepClass::getInstance()->validateSleepConfig(config));
    return SleepClass::getInstance()->enter(config, wakeup_source);
}
