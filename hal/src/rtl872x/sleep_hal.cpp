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
#define REALTEK_AMBD_SDK
#include "rtl8721d_pinmux_defines.h"
#include "rtl8721d.h"
}
#include "km0_km4_ipc.h"
#include "backup_ram_hal.h"
#include "dct.h"
#include "dct_hal.h"
#include "align_util.h"
#include "delay_hal.h"

using namespace particle;

extern uintptr_t platform_psram_start;
extern uintptr_t platform_psram_size;

namespace {

uint32_t psramSleepMarker = 0;

} // anonymous

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

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
#if HAL_PLATFORM_MCP23S17
    void configGpioWakeupSourceExt(const hal_wakeup_source_base_t* wakeupSources, hal_sleep_mode_t sleepMode) {
        const hal_pin_info_t* halPinMap = hal_pin_map();
        auto source = wakeupSources;
        bool config = false;
        uint8_t gpIntEn[2] = {0, 0};
        uint8_t intCon[2] = {0, 0};
        uint8_t defVal[2] = {0, 0};

        while (source) {
            if (source->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
                auto gpioWakeup = reinterpret_cast<const hal_wakeup_source_gpio_t*>(source);
                if (halPinMap[gpioWakeup->pin].type == HAL_PIN_TYPE_IO_EXPANDER) {
                    config = true;
                    switch(gpioWakeup->mode) {
                        case RISING: {
                            intCon[halPinMap[gpioWakeup->pin].gpio_port] |= (0x01 << halPinMap[gpioWakeup->pin].gpio_pin);
                            break;
                        }
                        case FALLING: {
                            intCon[halPinMap[gpioWakeup->pin].gpio_port] |= (0x01 << halPinMap[gpioWakeup->pin].gpio_pin);
                            defVal[halPinMap[gpioWakeup->pin].gpio_port] |= (0x01 << halPinMap[gpioWakeup->pin].gpio_pin);
                            break;
                        }
                        case CHANGE:
                        default: {
                            break;
                        }
                    }
                    gpIntEn[halPinMap[gpioWakeup->pin].gpio_port] |= (0x01 << halPinMap[gpioWakeup->pin].gpio_pin);
                }
            }
            source = source->next;
        }

        if (config) {
            Mcp23s17::getInstance().writeRegister(Mcp23s17::INTCON_ADDR[0], intCon[0]);
            Mcp23s17::getInstance().writeRegister(Mcp23s17::DEFVAL_ADDR[0], defVal[0]);
            Mcp23s17::getInstance().writeRegister(Mcp23s17::GPINTEN_ADDR[0], gpIntEn[0]);
            Mcp23s17::getInstance().writeRegister(Mcp23s17::INTCON_ADDR[1], intCon[1]);
            Mcp23s17::getInstance().writeRegister(Mcp23s17::DEFVAL_ADDR[1], defVal[1]);
            Mcp23s17::getInstance().writeRegister(Mcp23s17::GPINTEN_ADDR[1], gpIntEn[1]);
        }
    }
#endif // HAL_PLATFORM_MCP23S17
#endif // HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER

    /*
     * We're using clock gate on KM4 for stop mode and ultra-low power mode,
     * since the power gate will deinitialize PSRAM and we have to re-initialize
     * and copy code from flash to it. Power gate is used for hibernate mode.
     */
    __attribute__((section(".ram.sleep")))
    int enter(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeupReason) {
        CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);
        memcpy(&alignedConfig_.config, config, sizeof(hal_sleep_config_t));

        bool bleInitialized = hal_ble_is_initialized(nullptr);
        bool advertising = hal_ble_gap_is_advertising(nullptr) ||
                           hal_ble_gap_is_connecting(nullptr, nullptr) ||
                           hal_ble_gap_is_connected(nullptr, nullptr);
        if (bleInitialized) {
            hal_ble_stack_deinit(nullptr);
            // The delay is essential to make sure the resources are successfully freed.
            // FIXME: Is this still needed? hal_ble_stack_deinit() should wait
            // for deinitialization to complete?
            // Leaving as-is for now, but should be reassessed. We are postponing sleep
            // by 2 seconds all the time.
            HAL_Delay_Milliseconds(2000);
        }

        HAL_USB_Detach();

        if (config->mode == HAL_SLEEP_MODE_HIBERNATE) {
            /* Backup (sic!) backup RAM into flash */
            hal_backup_ram_sync(nullptr);
        }

        for (int usart = 0; usart < HAL_PLATFORM_USART_NUM; usart++) {
            if (hal_usart_is_enabled(static_cast<hal_usart_interface_t>(usart))) {
                hal_usart_flush(static_cast<hal_usart_interface_t>(usart));
            }
        }

        hal_interrupt_suspend();

        /* We need to configure the IO expander interrupt before disabling SPI interface. */
#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
#if HAL_PLATFORM_MCP23S17
        Mcp23s17::getInstance().interruptsSuspend();
        configGpioWakeupSourceExt(config->wakeup_sources, config->mode);
#endif
#endif

        // Disable thread scheduling
        os_thread_scheduling(false, nullptr);
        // Disable SysTick
        SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

        if (notifyKm0AndSleep() != SYSTEM_ERROR_NONE) {
            SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
            os_thread_scheduling(true, nullptr);
            return SYSTEM_ERROR_INTERNAL;
        }

        // NOTE: previously checked primask here, no need as if we are going into sleep
        // with disabled interrupts we have much bigger problems
        __disable_irq();

        auto mode = config->mode;

        psramSleepMarker = HAL_READ32(PSRAM_BASE, 0);

        // Clean == flush
        SCB_CleanInvalidateDCache_by_Addr((uint32_t*)&platform_psram_start, (int32_t)(&platform_psram_size));
        // IMPORTANT: DCache and ICache need to be disabled
        SCB_DisableDCache();
        SCB_DisableICache();
        __ISB();
        __DSB();


        // WARNING: any function/data being called/accessed in/after PSRAM being suspended should reside in SRAM
        if (mode == HAL_SLEEP_MODE_HIBERNATE) {
            suspendPsram(false);
        } else {
            suspendPsram(true);
        }

        __SEV(); // signal event, also signal to KM0
        __WFE(); // clear event, immediately exits
        __WFE(); // sleep, waiting for event

        // Only if either stop mode or ultra-low power mode is used, we can get here,
        // which means that the PSRAM retention is enabled, we don't need to re-initialized PSRAM
        resumePsram();

        // Re-enable and invalidate caches
        SCB_EnableICache();
        SCB_InvalidateICache();
        SCB_EnableDCache();
        SCB_InvalidateDCache();
        __ISB();
        __DSB();

        hal_pin_info_t* halPinMap = hal_pin_map();
        hal_pin_t wakeupPin = PIN_INVALID;
        hal_wakeup_source_type_t wakeupSourceType = HAL_WAKEUP_SOURCE_TYPE_UNKNOWN;

        // Read the int status before enabling interrupt.
        uint32_t intStatusA = HAL_READ32((uint32_t)GPIOA_BASE, 0x40);
        uint32_t intStatusB = HAL_READ32((uint32_t)GPIOB_BASE, 0x40);

        uint32_t wakeEvent = HAL_READ32(SYSTEM_CTRL_BASE, REG_HS_WAKE_EVENT_STATUS1);
        if (wakeEvent & BIT_HP_WEVT_TIMER_STS) {
            wakeupSourceType = HAL_WAKEUP_SOURCE_TYPE_RTC;
        } else if (wakeEvent & BIT_HP_WEVT_GPIO_STS) {
            const hal_wakeup_source_base_t* source = alignedConfig_.config.wakeup_sources;
            while (source) {
                if (source->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
                    const hal_wakeup_source_gpio_t* gpioWakeup = (const hal_wakeup_source_gpio_t*)source;
                    hal_pin_info_t* pinInfo = nullptr;
#if HAL_PLATFORM_IO_EXTENSION && HAL_PLATFORM_MCP23S17
                    if (halPinMap[gpioWakeup->pin].type == HAL_PIN_TYPE_MCU)
#endif
                    {
                        pinInfo = halPinMap + gpioWakeup->pin;
                    }
#if HAL_PLATFORM_IO_EXTENSION && HAL_PLATFORM_MCP23S17
                    else if (halPinMap[gpioWakeup->pin].type == HAL_PIN_TYPE_IO_EXPANDER) {
                        pinInfo = halPinMap + IOE_INT;
                    } else {
                        source = source->next;
                        continue;
                    }
#endif
                    if ((pinInfo->gpio_port == RTL_PORT_A && (intStatusA & (0x01 << pinInfo->gpio_pin)))
                        || (pinInfo->gpio_port == RTL_PORT_B && (intStatusB & (0x01 << pinInfo->gpio_pin)))) {
                        wakeupPin = gpioWakeup->pin;
                        wakeupSourceType = HAL_WAKEUP_SOURCE_TYPE_GPIO;
                        break;
                    }
                }
                source = source->next;
            }
        }

        hal_interrupt_restore();

#if HAL_PLATFORM_IO_EXTENSION && HAL_PLATFORM_MCP23S17
        uint8_t intStatus[2];
        Mcp23s17::getInstance().interruptsRestore(intStatus);
        if (halPinMap[wakeupPin].type == HAL_PIN_TYPE_IO_EXPANDER) {
            // We need to identify the exact wakeup pin attached to the IO expander.
            const hal_wakeup_source_base_t* source = alignedConfig_.config.wakeup_sources;
            while (source) {
                if (source->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
                    auto gpioWakeup = reinterpret_cast<const hal_wakeup_source_gpio_t*>(source);
                    uint8_t bitMask = 0x01 << halPinMap[gpioWakeup->pin].gpio_pin;
                    uint8_t port = halPinMap[gpioWakeup->pin].gpio_port;
                    if (halPinMap[gpioWakeup->pin].type == HAL_PIN_TYPE_IO_EXPANDER) {
                        if (intStatus[port] & bitMask) {
                            wakeupPin = gpioWakeup->pin;
                            break;
                        }
                    }
                }
                source = source->next;
            }
        }
#endif

        __enable_irq();
        __DSB();
        __ISB();

        // Clear int status
        HAL_WRITE32((uint32_t)GPIOA_BASE, 0x4C, 0xFFFF);
        HAL_WRITE32((uint32_t)GPIOB_BASE, 0x4C, 0xFFFF);

        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
        os_thread_scheduling(true, nullptr);

        // NOTE: USB stack, BLE stack, malloc require interrupts and thread scheduling to be enabled.
        HAL_USB_Attach();

        int ret = SYSTEM_ERROR_NONE;
        if (wakeupReason) {
            if (wakeupSourceType == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
                ret = constructWakeupReason((hal_wakeup_source_gpio_t**)wakeupReason, HAL_WAKEUP_SOURCE_TYPE_GPIO, &wakeupPin);
            } else if (wakeupSourceType == HAL_WAKEUP_SOURCE_TYPE_RTC) {
                ret = constructWakeupReason((hal_wakeup_source_rtc_t**)wakeupReason, HAL_WAKEUP_SOURCE_TYPE_RTC, nullptr);
            } else {
                ret = SYSTEM_ERROR_INTERNAL;
            }
        }

        if (bleInitialized) {
            if (hal_ble_stack_init(nullptr) == SYSTEM_ERROR_NONE && advertising) {
                hal_ble_gap_start_advertising(nullptr);
            }
        }

        return ret;
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

    bool isAonPin(hal_pin_t pin) {
        uint16_t rtlPin = hal_pin_to_rtl_pin(pin);
        // PA_21 is also a wakeup source, but it's excluded below since it's
        // only available on MSoM TX2 which is an internal pin for NCP.
        if ((rtlPin >= _PA_12 && rtlPin <= _PA_20) ||
             rtlPin == _PA_25 || rtlPin == _PA_26) {
            return true;
        }
        return false;
    }

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
        if (mode == HAL_SLEEP_MODE_HIBERNATE && !isAonPin(gpio->pin)) {
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

    __attribute__((section(".ram.sleep")))
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

            // Disable memory access from CPU
            // PSRAM_DEV->CSR |= BIT_PSRAM_MEM_IDLE;
            // while(!(PSRAM_DEV->CSR & BIT_PSRAM_MEM_IDLE));

            // Wait for half-sleep entry just in case
            DelayUs(200);
        } else {
            temp = HAL_READ32(SYSTEM_CTRL_BASE_LP, REG_AON_LDO_CTRL1);
            temp &= ~(BIT_LDO_PSRAM_EN);
            HAL_WRITE32(SYSTEM_CTRL_BASE_LP, REG_AON_LDO_CTRL1, temp);
        }
        __ISB();
        __DSB();
    }

    __attribute__((section(".ram.sleep")))
    void resumePsram() {
        // Disable memory access from CPU
        PSRAM_DEV->CSR |= BIT_PSRAM_MEM_IDLE;
        while(!(PSRAM_DEV->CSR & BIT_PSRAM_MEM_IDLE));
        uint8_t PSRAM_CA[6];
        // FIXME: No idea how to align addresses properly here to correctly read things out, reading beginning of PSRAM seems to work though
        PSRAM_CTRL_CA_Gen(PSRAM_CA, 0, PSRAM_LINEAR_TYPE, PSRAM_MEM_SPACE, PSRAM_READ_TRANSACTION);
        for (int i = 0; i < 10; i++) {
            uint32_t temp = 0xaabbccdd;
            PSRAM_DEV->DPDR = 0;
            PSRAM_DEV->CMD_DPIN_NDGE = (PSRAM_CA[0] | (PSRAM_CA[2] << 8) | (PSRAM_CA[4] << 16));
            PSRAM_DEV->CMD_DPIN = (PSRAM_CA[1] | (PSRAM_CA[3] << 8) | (PSRAM_CA[5] << 16));

            PSRAM_DEV->CSR &= (~BIT_PSRAM_DPIN_MODE);
            PSRAM_DEV->CSR |= PSRAM_DPIN_READ_MODE;

            PSRAM_DEV->CCR = BIT_PSRAM_DPIN;
            // 150us should be enough, but we'll anyway retry a few times just in case
            for (int j = 0; j < 150; j++) {
                if (PSRAM_DEV->CCR & BIT_PSRAM_DPIN) {
                    break;
                }
                DelayUs(1);
            }
            if (PSRAM_DEV->CCR & BIT_PSRAM_DPIN) {
                PSRAM_DEV->DPDRI = 0;
                temp = PSRAM_DEV->DPDR;
                if (temp == psramSleepMarker) {
                    break;
                }
            } else {
                PSRAM_DEV->CCR &= ~BIT_PSRAM_DPIN;
            }
        }

        // Enable memory access from CPU
        PSRAM_DEV->CSR &= (~BIT_PSRAM_MEM_IDLE);
        while(PSRAM_DEV->CSR & BIT_PSRAM_MEM_IDLE);
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
