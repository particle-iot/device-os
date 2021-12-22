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
#include "check.h"
#include "radio_common.h"
#if HAL_PLATFORM_EXTERNAL_RTC
#include "exrtc_hal.h"
#endif
#include "spark_wiring_vector.h"

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
#if HAL_PLATFORM_MCP23S17
#include "mcp23s17.h"
#endif
#endif
extern "C" {
#include "rtl8721d.h"
}
#include "km0_km4_ipc.h"

using namespace particle;

class SleepClass {
public:
    int validateSleepConfig(const hal_sleep_config_t* config) {
        CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);
        return SYSTEM_ERROR_NONE;
    }

    /*
     * We're using clock gate on KM4 for stop mode and ultra-low power mode,
     * since the power gate will deinitialize PSRAM and we have to re-initialize
     * and copy code from flash to it. Power gate is used for hibernate mode.
     */
    __attribute__((section(".ram.sleep"), optimize("O0")))
    int enter(const hal_sleep_config_t* config) {
        CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);
        memcpy(&config_, config, sizeof(hal_sleep_config_t));
        DCache_Clean((uint32_t)&config_, sizeof(hal_sleep_config_t));
        // Disable thread scheduling
        os_thread_scheduling(false, nullptr);
        // Disable SysTick
        SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

        if (notifyKm0AndSleep() != SYSTEM_ERROR_NONE) {
            SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
            os_thread_scheduling(true, nullptr);
            return SYSTEM_ERROR_INTERNAL;
        }

        Cache_Enable(false);
        if (config->mode == HAL_SLEEP_MODE_HIBERNATE) {
            suspendPsram(false);
        } else {
            suspendPsram(true);
        }
        
        int priMask = __get_PRIMASK();
        __disable_irq();
        __DSB();
        __ISB();

        LOG(TRACE, "KM4: enters sleep");
        DelayMs(20);
        __SEV(); // signal event, also signal to KM0
        __WFE(); // clear event, immediately exits
        __WFE(); // sleep, waiting for event
        LOG(TRACE, "KM4: wakes up");
        
        // Only if either stop mode or ultra-low power mode is used, we can get here,
        // which means that the PSRAM retention is enabled, we don't need to re-initialized PSRAM
        Cache_Enable(true);
        resumePsram();

        if ((priMask & 1) == 0) {
            __enable_irq();
        }
        __DSB();
        __ISB();

        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
        os_thread_scheduling(true, nullptr);
        return SYSTEM_ERROR_NONE;
    }

    static SleepClass* getInstance() {
        static SleepClass sleep;
        return &sleep;
    }

private:
    SleepClass()
            : ipcResult_(SYSTEM_ERROR_INTERNAL) {
    }
    ~SleepClass() = default;

    int notifyKm0AndSleep() {
        ipcResult_ = SYSTEM_ERROR_INTERNAL;
        int ret = km0_km4_ipc_send_request(KM0_KM4_IPC_CHANNEL_GENERIC, KM0_KM4_IPC_MSG_SLEEP, &config_,
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
            instance->ipcResult_ = *((int*)msg->data);
        }
    }

    volatile int ipcResult_;
    hal_sleep_config_t config_;
};

int hal_sleep_validate_config(const hal_sleep_config_t* config, void* reserved) {
    return SleepClass::getInstance()->validateSleepConfig(config);
}

int hal_sleep_enter(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeup_source, void* reserved) {
    // Check it again just in case.
    CHECK(SleepClass::getInstance()->validateSleepConfig(config));
    return SleepClass::getInstance()->enter(config);
}
