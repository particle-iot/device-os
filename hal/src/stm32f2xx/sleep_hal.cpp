/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

#include <malloc.h>
#include "stm32f2xx.h"
#include "gpio_hal.h"
#include "pwm_hal.h"
#include "i2c_hal.h"
#include "spi_hal.h"
#include "adc_hal.h"
#include "rtc_hal.h"
#include "usb_hal.h"
#include "usart_hal.h"
#include "interrupts_hal.h"
#include "check.h"


// anonymous namespace
namespace {

constexpr uint16_t EXTI9_5_BITS_MASK = 0x03E0;
constexpr uint16_t EXTI15_10_BITS_MASK = 0xFC00;

constexpr uint8_t extiChannelNum = 16;

constexpr uint8_t GPIO_IRQn[extiChannelNum] = {
    EXTI0_IRQn,     //0
    EXTI1_IRQn,     //1
    EXTI2_IRQn,     //2
    EXTI3_IRQn,     //3
    EXTI4_IRQn,     //4
    EXTI9_5_IRQn,   //5
    EXTI9_5_IRQn,   //6
    EXTI9_5_IRQn,   //7
    EXTI9_5_IRQn,   //8
    EXTI9_5_IRQn,   //9
    EXTI15_10_IRQn, //10
    EXTI15_10_IRQn, //11
    EXTI15_10_IRQn, //12
    EXTI15_10_IRQn, //13
    EXTI15_10_IRQn, //14
    EXTI15_10_IRQn  //15
};

// Bitmask
uint16_t extiPriorityBumped = 0x0000;

};

static int constructGpioWakeupReason(hal_wakeup_source_base_t** wakeupReason, pin_t pin) {
    if (wakeupReason) {
        hal_wakeup_source_gpio_t* gpio = (hal_wakeup_source_gpio_t*)malloc(sizeof(hal_wakeup_source_gpio_t));
        if (gpio) {
            gpio->base.size = sizeof(hal_wakeup_source_gpio_t);
            gpio->base.version = HAL_SLEEP_VERSION;
            gpio->base.type = HAL_WAKEUP_SOURCE_TYPE_GPIO;
            gpio->base.next = nullptr;
            gpio->pin = pin;
            *wakeupReason = reinterpret_cast<hal_wakeup_source_base_t*>(gpio);
        } else {
            return SYSTEM_ERROR_NO_MEMORY;
        }
    }
    return SYSTEM_ERROR_NONE;
}

static int constructRtcWakeupReason(hal_wakeup_source_base_t** wakeupReason) {
    if (wakeupReason) {
        hal_wakeup_source_rtc_t* rtc = (hal_wakeup_source_rtc_t*)malloc(sizeof(hal_wakeup_source_rtc_t));
        if (rtc) {
            rtc->base.size = sizeof(hal_wakeup_source_rtc_t);
            rtc->base.version = HAL_SLEEP_VERSION;
            rtc->base.type = HAL_WAKEUP_SOURCE_TYPE_RTC;
            rtc->base.next = nullptr;
            rtc->ms = 0;
            *wakeupReason = reinterpret_cast<hal_wakeup_source_base_t*>(rtc);
        } else {
            return SYSTEM_ERROR_NO_MEMORY;
        }
    }
    return SYSTEM_ERROR_NONE;
}

static int constructAnalogWakeupReason(hal_wakeup_source_base_t** wakeupReason, pin_t pin) {
    if (wakeupReason) {
        auto lpcomp = (hal_wakeup_source_lpcomp_t*)malloc(sizeof(hal_wakeup_source_lpcomp_t));
        if (lpcomp) {
            lpcomp->base.size = sizeof(hal_wakeup_source_lpcomp_t);
            lpcomp->base.version = HAL_SLEEP_VERSION;
            lpcomp->base.type = HAL_WAKEUP_SOURCE_TYPE_LPCOMP;
            lpcomp->base.next = nullptr;
            lpcomp->pin = pin;
            *wakeupReason = reinterpret_cast<hal_wakeup_source_base_t*>(lpcomp);
        } else {
            return SYSTEM_ERROR_NO_MEMORY;
        }
    }
    return SYSTEM_ERROR_NONE;
}

static int constructUsartWakeupReason(hal_wakeup_source_base_t** wakeupReason, hal_usart_interface_t serial) {
    if (wakeupReason) {
        auto usart = (hal_wakeup_source_usart_t*)malloc(sizeof(hal_wakeup_source_usart_t));
        if (usart) {
            usart->base.size = sizeof(hal_wakeup_source_usart_t);
            usart->base.version = HAL_SLEEP_VERSION;
            usart->base.type = HAL_WAKEUP_SOURCE_TYPE_USART;
            usart->base.next = nullptr;
            usart->serial = serial;
            *wakeupReason = reinterpret_cast<hal_wakeup_source_base_t*>(usart);
        } else {
            return SYSTEM_ERROR_NO_MEMORY;
        }
    }
    return SYSTEM_ERROR_NONE;
}

#if HAL_PLATFORM_CELLULAR
static int constructNetworkWakeupReason(hal_wakeup_source_base_t** wakeupReason, network_interface_index index) {
    auto network = (hal_wakeup_source_network_t*)malloc(sizeof(hal_wakeup_source_network_t));
    if (network) {
        network->base.size = sizeof(hal_wakeup_source_base_t);
        network->base.version = HAL_SLEEP_VERSION;
        network->base.type = HAL_WAKEUP_SOURCE_TYPE_NETWORK;
        network->base.next = nullptr;
        network->index = index;
        *wakeupReason = reinterpret_cast<hal_wakeup_source_base_t*>(network);
    } else {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    return SYSTEM_ERROR_NONE;
}
#endif

static int configGpioWakeupSource(const hal_wakeup_source_base_t* wakeupSources, uint8_t* extiPriorities) {
    auto source = wakeupSources;
    while (source) {
        if (source->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            auto gpioWakeup = reinterpret_cast<const hal_wakeup_source_gpio_t*>(source);
            PinMode wakeUpPinMode = INPUT;
            /* Set required pinMode based on edgeTriggerMode */
            switch(gpioWakeup->mode) {
                case RISING: {
                    wakeUpPinMode = INPUT_PULLDOWN;
                    break;
                }
                case FALLING: {
                    wakeUpPinMode = INPUT_PULLUP;
                    break;
                }
                case CHANGE:
                default: {
                    wakeUpPinMode = INPUT;
                    break;
                }
            }
            HAL_Pin_Mode(gpioWakeup->pin, wakeUpPinMode);
            HAL_InterruptExtraConfiguration irqConf = {};
            irqConf.version = HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION_2;
            irqConf.IRQChannelPreemptionPriority = 0;
            irqConf.IRQChannelSubPriority = 0;
            irqConf.keepHandler = 1;
            irqConf.keepPriority = 1;
            HAL_Interrupts_Attach(gpioWakeup->pin, nullptr, nullptr, gpioWakeup->mode, &irqConf);
            
            Hal_Pin_Info* pinMap = HAL_Pin_Map();
            uint8_t pinSource = pinMap[gpioWakeup->pin].gpio_pin_source;
            if (!(extiPriorityBumped >> pinSource) & 0x0001) {
                extiPriorities[pinSource] = NVIC_GetPriority(static_cast<IRQn_Type>(GPIO_IRQn[pinSource]));
                if (pinSource <= 4) {
                    extiPriorityBumped |= (0x0001 << pinSource);
                }
                if (pinSource >= 5 && pinSource <= 9) {
                    extiPriorityBumped |= EXTI9_5_BITS_MASK;
                } else if (pinSource >= 10 && pinSource <= 15) {
                    extiPriorityBumped |= EXTI15_10_BITS_MASK;
                }
            }
            NVIC_SetPriority(static_cast<IRQn_Type>(GPIO_IRQn[pinSource]), 1);
        }
        source = source->next;
    }
    return SYSTEM_ERROR_NONE;
}

static int configAnalogWakeupSource(const hal_wakeup_source_base_t* wakeupSources, uint8_t* configured) {
    auto source = wakeupSources;
    while (source) {
        if (source->type == HAL_WAKEUP_SOURCE_TYPE_LPCOMP) {
            uint8_t hysteresis = 24; // 20mV * (4095 / 3300)
            auto analogWakeup = reinterpret_cast<const hal_wakeup_source_lpcomp_t*>(source);
            *configured = true;
            Hal_Pin_Info* pinMap = HAL_Pin_Map();
            uint16_t currVol = hal_adc_read(analogWakeup->pin);
            hal_adc_sleep(true, nullptr); // Suspend ADC
            if (pinMap[analogWakeup->pin].pin_mode != AN_INPUT) {
                HAL_Pin_Mode(analogWakeup->pin, AN_INPUT);
            }
            NVIC_DisableIRQ(ADC_IRQn);
            NVIC_ClearPendingIRQ(ADC_IRQn);

            RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
            /* ADC Common Init */
            ADC_CommonInitTypeDef ADC_CommonInitStructure = {};
            ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
            ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
            ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
            ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
            ADC_CommonInit(&ADC_CommonInitStructure);

            // ADC1 configuration
            ADC_InitTypeDef ADC_InitStructure = {};
            ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b; // 12-bits
            ADC_InitStructure.ADC_ScanConvMode = DISABLE;
            ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
            ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
            ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
            ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
            ADC_InitStructure.ADC_NbrOfConversion = 1;
            ADC_Init(ADC1, &ADC_InitStructure);

            // ADC1 regular channel configuration
            ADC_RegularChannelConfig(ADC1, pinMap[analogWakeup->pin].adc_channel, 1, ADC_SampleTime_480Cycles);

            // Analog watchdog configuration
            ADC_AnalogWatchdogCmd(ADC1, ADC_AnalogWatchdog_SingleRegEnable);
            ADC_AnalogWatchdogSingleChannelConfig(ADC1, pinMap[analogWakeup->pin].adc_channel);
            uint16_t th = (uint16_t)((analogWakeup->voltage * 4095) / 3300);
            th &= 0x00000FFF;
            switch (analogWakeup->trig) {
                case HAL_SLEEP_LPCOMP_ABOVE: {
                    ADC_AnalogWatchdogThresholdsConfig(ADC1, th, 0);
                    break;
                }
                case HAL_SLEEP_LPCOMP_BELOW: {
                    ADC_AnalogWatchdogThresholdsConfig(ADC1, 4095, th);
                    break;
                }
                case HAL_SLEEP_LPCOMP_CROSS: {
                    if (currVol >= th) {
                        if (th < hysteresis) {
                            hysteresis = th;
                        }
                        ADC_AnalogWatchdogThresholdsConfig(ADC1, 4095, th - hysteresis);
                    } else {
                        if ((th + hysteresis) > 0xFFF) {
                            hysteresis = 0xFFF - th;
                        }
                        ADC_AnalogWatchdogThresholdsConfig(ADC1, th + hysteresis, 0);
                    }
                    break;
                }
                default: return SYSTEM_ERROR_INVALID_ARGUMENT; // It shouldn't reach here.
            }

            // Interrupt configuration
            ADC_ClearFlag(ADC1, ADC_FLAG_AWD);
            ADC_ITConfig(ADC1, ADC_IT_AWD, ENABLE);

            //select NVIC channel to configure
            NVIC_InitTypeDef NVIC_InitStructure = {};
            NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;
            NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
            NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
            NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
            NVIC_Init(&NVIC_InitStructure);
            
            ADC_Cmd(ADC1, ENABLE);
            ADC_SoftwareStartConv(ADC1);
            break;
        }
        source = source->next;
    }
    return SYSTEM_ERROR_NONE;
}

static int configRtcWakeupSource(const hal_wakeup_source_base_t* wakeupSources) {
    auto source = wakeupSources;
    while (source) {
        if (source->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            auto rtcWakeup = reinterpret_cast<const hal_wakeup_source_rtc_t*>(source);
            long seconds = rtcWakeup->ms / 1000;

            /*
             * - To wake up from the Stop mode with an RTC alarm event, it is necessary to:
             * - Configure the EXTI Line 17 to be sensitive to rising edges (Interrupt
             * or Event modes) using the EXTI_Init() function.
             *
             */
            hal_rtc_cancel_alarm();

            // Connect RTC to EXTI line
            EXTI_InitTypeDef extiInitStructure = {};
            extiInitStructure.EXTI_Line = EXTI_Line17;
            extiInitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
            extiInitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
            extiInitStructure.EXTI_LineCmd = ENABLE;
            EXTI_Init(&extiInitStructure);

            NVIC_InitTypeDef nvicInitStructure;
            nvicInitStructure.NVIC_IRQChannel = RTC_Alarm_IRQn;
            nvicInitStructure.NVIC_IRQChannelPreemptionPriority = 1;
            nvicInitStructure.NVIC_IRQChannelSubPriority = 0;
            nvicInitStructure.NVIC_IRQChannelCmd = ENABLE;
            NVIC_Init(&nvicInitStructure);

            struct timeval tv = {
                .tv_sec = seconds,
                .tv_usec = 0
            };
            CHECK(hal_rtc_set_alarm(&tv, HAL_RTC_ALARM_FLAG_IN, nullptr, nullptr, nullptr));
        }
        source = source->next;
    }
    return SYSTEM_ERROR_NONE;
}

static int configUsartWakeupSource(const hal_wakeup_source_base_t* wakeupSources, uint8_t* configured) {
    auto source = wakeupSources;
    while (source) {
        if (source->type == HAL_WAKEUP_SOURCE_TYPE_USART) {
            *configured = 1;
            auto usartWakeup = reinterpret_cast<const hal_wakeup_source_usart_t*>(source);
            
            // Disabled unwanted interrupts and bump the interrupt priority
            NVIC_InitTypeDef NVIC_InitStructure;
            switch (usartWakeup->serial) {
                case HAL_USART_SERIAL1: {
                    NVIC_DisableIRQ(USART1_IRQn);
                    NVIC_ClearPendingIRQ(USART1_IRQn);
                    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
                    USART_ITConfig(USART1, USART_IT_TXE, DISABLE); // TXE pending bit is cleared only by a write to the USART_DR register
                    break;
                }
                case HAL_USART_SERIAL2: {
                    NVIC_DisableIRQ(USART2_IRQn);
                    NVIC_ClearPendingIRQ(USART2_IRQn);
                    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
                    USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
                    break;
                }
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
                case HAL_USART_SERIAL4: {
                    NVIC_DisableIRQ(UART4_IRQn);
                    NVIC_ClearPendingIRQ(UART4_IRQn);
                    NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
                    USART_ITConfig(UART4, USART_IT_TXE, DISABLE);
                    break;
                }
                case HAL_USART_SERIAL5: {
                    NVIC_DisableIRQ(UART5_IRQn);
                    NVIC_ClearPendingIRQ(UART5_IRQn);
                    NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
                    USART_ITConfig(UART5, USART_IT_TXE, DISABLE);
                    break;
                }
#endif
                default: return SYSTEM_ERROR_INVALID_ARGUMENT; // It should not reach here.
            }
            NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
            NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
            NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
            NVIC_Init(&NVIC_InitStructure);
        }
        source = source->next;
    }
    return SYSTEM_ERROR_NONE;
}

static int configNetworkWakeupSource(const hal_wakeup_source_base_t* wakeupSources, uint8_t* configured) {
#if HAL_PLATFORM_CELLULAR
    auto source = wakeupSources;
    while (source) {
        if (source->type == HAL_WAKEUP_SOURCE_TYPE_NETWORK) {
            auto network = reinterpret_cast<const hal_wakeup_source_network_t*>(source);
            if (!(network->flags & HAL_SLEEP_NETWORK_FLAG_INACTIVE_STANDBY)) {
                *configured = 1;
                // We don't need to bump the interrupt priority, since the priority has been set to 0 in electronserialpipe_hal.c
                // We don't disable any USART interrupt here to make sure the modem driver behaves correctly.
                break; // There is only one USART available for network modem. Stop traversing the list.
            }
        }
        source = source->next;
    }
    return SYSTEM_ERROR_NONE;
#else
    return SYSTEM_ERROR_NOT_SUPPORTED;
#endif
}

static int validateGpioWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_gpio_t* gpio) {
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
    if (mode == HAL_SLEEP_MODE_STOP) {
        if (gpio->pin >= TOTAL_PINS) {
            return SYSTEM_ERROR_LIMIT_EXCEEDED;
        }
    } else if (mode == HAL_SLEEP_MODE_HIBERNATE) {
        // Only the WKP pin can be used to wake up device from hibernate mode.
        if (gpio->pin != WKP) {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
    }
    return SYSTEM_ERROR_NONE;
}

static int validateAnalogWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_lpcomp_t* analog) {
    if (HAL_Validate_Pin_Function(analog->pin, PF_ADC) != PF_ADC) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (analog->trig > HAL_SLEEP_LPCOMP_CROSS) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (mode != HAL_SLEEP_MODE_STOP) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    return SYSTEM_ERROR_NONE;
}

static int validateRtcWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_rtc_t* rtc) {
    if (rtc->ms < 1000) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    return SYSTEM_ERROR_NONE;
}

static int validateNetworkWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_network_t* network) {
#if HAL_PLATFORM_CELLULAR
    if (!(network->flags & HAL_SLEEP_NETWORK_FLAG_INACTIVE_STANDBY)) {
        if (!(USART3->CR1 & USART_CR1_UE)) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        if (mode != HAL_SLEEP_MODE_STOP) {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
    }
#endif

#if HAL_PLATFORM_WIFI
    if (!(network->flags & HAL_SLEEP_NETWORK_FLAG_INACTIVE_STANDBY)) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
#endif
    return SYSTEM_ERROR_NONE;
}

static int validateUsartWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_usart_t* usart) {
    if (!hal_usart_is_enabled(usart->serial)) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (mode != HAL_SLEEP_MODE_STOP) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    return SYSTEM_ERROR_NONE;
}

static int validateWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_base_t* base) {
    if (base->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
        return validateGpioWakeupSource(mode, reinterpret_cast<const hal_wakeup_source_gpio_t*>(base));
    } else if (base->type == HAL_WAKEUP_SOURCE_TYPE_LPCOMP) {
        return validateAnalogWakeupSource(mode, reinterpret_cast<const hal_wakeup_source_lpcomp_t*>(base));
    } else if (base->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
        return validateRtcWakeupSource(mode, reinterpret_cast<const hal_wakeup_source_rtc_t*>(base));
    } else if (base->type == HAL_WAKEUP_SOURCE_TYPE_NETWORK) {
        return validateNetworkWakeupSource(mode, reinterpret_cast<const hal_wakeup_source_network_t*>(base));
    } else if (base->type == HAL_WAKEUP_SOURCE_TYPE_USART) {
        return validateUsartWakeupSource(mode, reinterpret_cast<const hal_wakeup_source_usart_t*>(base));
    }
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

static void enterPlatformSleepMode(uint8_t highAccuracy) {
    int basePri = __get_BASEPRI();
    __set_BASEPRI(2 << (8 - __NVIC_PRIO_BITS));

    if (!highAccuracy) {
        /* Select HSE as system clock source */
        RCC_SYSCLKConfig(RCC_SYSCLKSource_HSE);
        /* Wait till HSE is used as system clock source */
        while (RCC_GetSYSCLKSource() != 0x04);

        /* Disable PLL */
        RCC_PLLCmd(DISABLE);
    }

    __DSB();
    __WFI();
    __NOP();
    __ISB();

    if (!highAccuracy) {
        /* Enable PLL */
        RCC_PLLCmd(ENABLE);
        while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

        /* Select PLL as system clock source */
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
        /* Wait till PLL is used as system clock source */
        while (RCC_GetSYSCLKSource() != 0x08);
    }

    __set_BASEPRI(basePri);
}

static void enterPlatformStopMode() {
    /* Request to enter STOP mode with regulator in low power mode */
    PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);

    /* At this stage the system has resumed from STOP mode */
    /* Enable HSE, PLL and select PLL as system clock source after wake-up from STOP */

    // FIXME: Re-enter stop mode if device isn't woken up by the configured wakeup sources.

    /* Enable HSE */
    RCC_HSEConfig(RCC_HSE_ON);

    /* Wait till HSE is ready */
    if (RCC_WaitForHSEStartUp() != SUCCESS) {
        /* If HSE startup fails try to recover by system reset */
        NVIC_SystemReset();
    }

    /* Enable PLL */
    RCC_PLLCmd(ENABLE);

    /* Wait till PLL is ready */
    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

    /* Select PLL as system clock source */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

    /* Wait till PLL is used as system clock source */
    while (RCC_GetSYSCLKSource() != 0x08);

    hal_rtc_internal_exit_sleep();
}

static int enterStopBasedSleep(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeupReason) {
    int ret = SYSTEM_ERROR_NONE;

    // We need to do this before disabling systick/interrupts, otherwise
    // there is a high chance of a deadlock
    if (config->mode == HAL_SLEEP_MODE_ULTRA_LOW_POWER) {
        // Suspend USARTs
        for (int usart = 0; usart < HAL_PLATFORM_USART_NUM; usart++) {
            // FIXME: no locking
            hal_usart_sleep(static_cast<hal_usart_interface_t>(usart), true, nullptr);
        }
        // Suspend SPIs
        for (int spi = 0; spi < HAL_PLATFORM_SPI_NUM; spi++) {
#if HAL_PLATFORM_SPI_HAL_THREAD_SAFETY
            hal_spi_acquire(static_cast<hal_spi_interface_t>(spi), nullptr);
#endif // HAL_PLATFORM_SPI_HAL_THREAD_SAFETY
            hal_spi_sleep(static_cast<hal_spi_interface_t>(spi), true, nullptr);
        }
        // Suspend I2Cs
        for (int i2c = 0; i2c < HAL_PLATFORM_I2C_NUM; i2c++) {
            hal_i2c_lock(static_cast<hal_i2c_interface_t>(i2c), nullptr);
            hal_i2c_sleep(static_cast<hal_i2c_interface_t>(i2c), true, nullptr);
        }

        // Suspend ADC module
        // FIXME: no locking
        hal_adc_sleep(true, nullptr);
    }

    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

    // After disabling systick, as it controls RGB pins
    if (config->mode == HAL_SLEEP_MODE_ULTRA_LOW_POWER) {
        hal_pwm_sleep(true, nullptr);
    }

    // Detach USB
    HAL_USB_Detach();

    if (config->mode != HAL_SLEEP_MODE_ULTRA_LOW_POWER) {
        // Flush all USARTs
        for (int usart = 0; usart < HAL_PLATFORM_USART_NUM; usart++) {
            if (hal_usart_is_enabled(static_cast<hal_usart_interface_t>(usart))) {
                hal_usart_flush(static_cast<hal_usart_interface_t>(usart));
            }
        }
    }

    int32_t state = HAL_disable_irq();

    // Suspend all EXTI interrupts
    HAL_Interrupts_Suspend();

    Hal_Pin_Info* halPinMap = HAL_Pin_Map();

    uint8_t extiPriority[extiChannelNum];

    configGpioWakeupSource(config->wakeup_sources, extiPriority);
    uint8_t analogWakeup = 0;
    configAnalogWakeupSource(config->wakeup_sources, &analogWakeup);
    uint8_t usartWakeup = 0;
    configUsartWakeupSource(config->wakeup_sources, &usartWakeup);
    uint8_t networkWakeup = 0;
    configNetworkWakeupSource(config->wakeup_sources, &networkWakeup);

    bool useWfi = config->mode == HAL_SLEEP_MODE_STOP && (analogWakeup || usartWakeup || networkWakeup);
    if (!useWfi) {
        hal_rtc_internal_enter_sleep();
    }
    configRtcWakeupSource(config->wakeup_sources);

    if (useWfi) {
        if (usartWakeup || networkWakeup) {
            enterPlatformSleepMode(true);
        } else {
            enterPlatformSleepMode(false);
        }
    } else {
        enterPlatformStopMode();
    }

    auto wakeupSource = config->wakeup_sources;
    while (wakeupSource) {
        if (NVIC_GetPendingIRQ(RTC_Alarm_IRQn) && wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            ret = constructRtcWakeupReason(wakeupReason);
            break; // Stop traversing the wakeup sources list.
        } else if ((NVIC_GetPendingIRQ(EXTI0_IRQn) || NVIC_GetPendingIRQ(EXTI1_IRQn) ||
                    NVIC_GetPendingIRQ(EXTI2_IRQn) || NVIC_GetPendingIRQ(EXTI3_IRQn) ||
                    NVIC_GetPendingIRQ(EXTI4_IRQn) || NVIC_GetPendingIRQ(EXTI9_5_IRQn) || NVIC_GetPendingIRQ(EXTI15_10_IRQn)) &&
                    wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            auto gpioWakeup = reinterpret_cast<hal_wakeup_source_gpio_t*>(wakeupSource);
            if (EXTI_GetITStatus(halPinMap[gpioWakeup->pin].gpio_pin) != RESET) {
                ret = constructGpioWakeupReason(wakeupReason, gpioWakeup->pin);
                break; // Stop traversing the wakeup sources list.
            }
        } else if (NVIC_GetPendingIRQ(ADC_IRQn) && wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_LPCOMP) {
            auto analogWakeup = reinterpret_cast<hal_wakeup_source_lpcomp_t*>(wakeupSource);
            ret = constructAnalogWakeupReason(wakeupReason, analogWakeup->pin);
            break; // Stop traversing the wakeup sources list.
        } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_USART) {
            auto usartWakeup = reinterpret_cast<hal_wakeup_source_usart_t*>(wakeupSource);
            if (   (NVIC_GetPendingIRQ(USART1_IRQn) && usartWakeup->serial == HAL_USART_SERIAL1)
                || (NVIC_GetPendingIRQ(USART2_IRQn) && usartWakeup->serial == HAL_USART_SERIAL2)
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION // Electron
                || (NVIC_GetPendingIRQ(UART4_IRQn) && usartWakeup->serial == HAL_USART_SERIAL4)
                || (NVIC_GetPendingIRQ(UART5_IRQn) && usartWakeup->serial == HAL_USART_SERIAL5)
#endif
                ) {
                ret = constructUsartWakeupReason(wakeupReason, usartWakeup->serial);
                break; // Stop traversing the wakeup sources list.
            }
        }
        else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_NETWORK) {
#if HAL_PLATFORM_CELLULAR
            auto networkWakeup = reinterpret_cast<hal_wakeup_source_network_t*>(wakeupSource);
            if (NVIC_GetPendingIRQ(USART3_IRQn) && networkWakeup->index == NETWORK_INTERFACE_CELLULAR && !(networkWakeup->flags & HAL_SLEEP_NETWORK_FLAG_INACTIVE_STANDBY)) {
                ret = constructNetworkWakeupReason(wakeupReason, networkWakeup->index);
                break; // Stop traversing the wakeup sources list.
            }
#endif
        }
        wakeupSource = wakeupSource->next;
    }

    if (analogWakeup) {
        ADC_ITConfig(ADC1, ADC_IT_AWD, DISABLE);
        ADC_Cmd(ADC1, DISABLE);
        NVIC_DisableIRQ(ADC_IRQn);
        NVIC_ClearPendingIRQ(ADC_IRQn);
        hal_adc_sleep(false, nullptr); // Restore ADC configuration
    }

    // FIXME: Should we enter sleep again if there is no pending IRQn in corresponding to the wakeup source?

    wakeupSource = config->wakeup_sources;
    while (wakeupSource) {
        if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            NVIC_InitTypeDef nvicInitStructure;
            nvicInitStructure.NVIC_IRQChannel = RTC_Alarm_IRQn;
            nvicInitStructure.NVIC_IRQChannelPreemptionPriority = RTC_Alarm_IRQ_PRIORITY;
            nvicInitStructure.NVIC_IRQChannelSubPriority = 0;
            nvicInitStructure.NVIC_IRQChannelCmd = ENABLE;
            NVIC_Init(&nvicInitStructure);
            // No need to detach RTC Alarm from EXTI, since it will be detached in HAL_Interrupts_Restore()
            // RTC Alarm should be canceled to avoid entering HAL_RTCAlarm_Handler or if we were woken up by pin
            hal_rtc_cancel_alarm();
        } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            auto gpioWakeup = reinterpret_cast<hal_wakeup_source_gpio_t*>(wakeupSource);
            HAL_Interrupts_Detach_Ext(gpioWakeup->pin, 1, nullptr);
            uint8_t pinSource = halPinMap[gpioWakeup->pin].gpio_pin_source;
            if ((extiPriorityBumped >> pinSource) & 0x0001) {
                NVIC_SetPriority(static_cast<IRQn_Type>(GPIO_IRQn[pinSource]), extiPriority[pinSource]);
                if (pinSource <= 4) {
                    extiPriorityBumped &= ~(0x0001 << pinSource);
                }
                if (pinSource >= 5 && pinSource <= 9) {
                    extiPriorityBumped &= ~EXTI9_5_BITS_MASK;
                } else if (pinSource >= 10 && pinSource <= 15) {
                    extiPriorityBumped &= ~EXTI15_10_BITS_MASK;
                }
            }
        } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_USART) {
            auto usartWakeup = reinterpret_cast<hal_wakeup_source_usart_t*>(wakeupSource);
            // Enabled unwanted interrupts and unbump the interrupt priority
            NVIC_InitTypeDef NVIC_InitStructure;
            switch (usartWakeup->serial) {
                case HAL_USART_SERIAL1: {
                    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
                    USART_ITConfig(USART1, USART_IT_TXE, ENABLE); // TXE pending bit is cleared only by a write to the USART_DR register
                    break;
                }
                case HAL_USART_SERIAL2: {
                    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
                    USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
                    break;
                }
#if PLATFORM_ID == PLATFORM_ELECTRON_PRODUCTION
                case HAL_USART_SERIAL4: {
                    NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
                    USART_ITConfig(UART4, USART_IT_TXE, ENABLE);
                    break;
                }
                case HAL_USART_SERIAL5: {
                    NVIC_InitStructure.NVIC_IRQChannel = UART5_IRQn;
                    USART_ITConfig(UART5, USART_IT_TXE, ENABLE);
                    break;
                }
#endif
                default: return SYSTEM_ERROR_INVALID_ARGUMENT; // It should not reach here.
            }
            NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7; // The priority is the same as what we set in usart_hal.c
            NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
            NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
            NVIC_Init(&NVIC_InitStructure);
        } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_NETWORK) {
#if HAL_PLATFORM_CELLULAR
            auto network = reinterpret_cast<const hal_wakeup_source_network_t*>(wakeupSource);
            if (!(network->flags & HAL_SLEEP_NETWORK_FLAG_INACTIVE_STANDBY)) {
                // We don't need to unbump the priority
            }
#endif
        }
        wakeupSource = wakeupSource->next;
    }

    // Restore
    HAL_Interrupts_Restore();

    if (config->mode == HAL_SLEEP_MODE_ULTRA_LOW_POWER) {
        hal_pwm_sleep(false, nullptr);
        // Restore ADC state
        hal_adc_sleep(false, nullptr);
        // Restore I2Cs
        for (int i2c = 0; i2c < HAL_PLATFORM_I2C_NUM; i2c++) {
            hal_i2c_sleep(static_cast<hal_i2c_interface_t>(i2c), false, nullptr);
            hal_i2c_unlock(static_cast<hal_i2c_interface_t>(i2c), nullptr);
        }
        // Restore SPIs
        for (int spi = 0; spi < HAL_PLATFORM_SPI_NUM; spi++) {
            hal_spi_sleep(static_cast<hal_spi_interface_t>(spi), false, nullptr);
#if HAL_PLATFORM_SPI_HAL_THREAD_SAFETY
            hal_spi_release(static_cast<hal_spi_interface_t>(spi), nullptr);
#endif // HAL_PLATFORM_SPI_HAL_THREAD_SAFETY
        }
        // Restore USARTs
        for (int usart = 0; usart < HAL_PLATFORM_USART_NUM; usart++) {
            // FIXME: no lock
            hal_usart_sleep(static_cast<hal_usart_interface_t>(usart), false, nullptr);
        }
    }

    // Successfully exited STOP mode
    HAL_enable_irq(state);

    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    // We need to syncup the time before exiting this function by triggering the 1ms SysTick interrupt,
    // Otherwise, if device is woken up from sleep mode (defined in ST's reference manual) followed by
    // invoking the delay() function somewhere, it won't work properly, since the DWT is still ticking
    // during the sleep mode.
    while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));

    HAL_USB_Attach();

    return ret;
}

static int enterHibernateMode(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeup_source) {
    hal_rtc_internal_enter_sleep();

    bool enableWkpPin = false;
    auto wakeupSource = config->wakeup_sources;
    while (wakeupSource) {
        if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            long seconds = reinterpret_cast<hal_wakeup_source_rtc_t*>(wakeupSource)->ms / 1000;

            struct timeval tv = {
                .tv_sec = seconds,
                .tv_usec = 0
            };
            hal_rtc_cancel_alarm();
            CHECK(hal_rtc_set_alarm(&tv, HAL_RTC_ALARM_FLAG_IN, nullptr, nullptr, nullptr));
        } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            enableWkpPin = true;
        }
        wakeupSource = wakeupSource->next;
    }

    if (enableWkpPin) {
        /* Enable WKUP pin */
        PWR_WakeUpPinCmd(ENABLE);
    } else {
        /* Disable WKUP pin */
        PWR_WakeUpPinCmd(DISABLE);
    }

    /* Request to enter STANDBY mode */
    PWR_EnterSTANDBYMode();

    /* Following code will not be reached */
    while(1);

    return SYSTEM_ERROR_NONE;
}

int hal_sleep_validate_config(const hal_sleep_config_t* config, void* reserved) {
    // Checks the sleep mode.
    if (config->mode == HAL_SLEEP_MODE_NONE || config->mode >= HAL_SLEEP_MODE_MAX) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    // Checks the wakeup sources
    auto wakeupSource = config->wakeup_sources;
    uint16_t valid = 0;
    while (wakeupSource) {
        CHECK(validateWakeupSource(config->mode, wakeupSource));
        if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_NETWORK) {
            auto network = reinterpret_cast<const hal_wakeup_source_network_t*>(wakeupSource);
            if (!(network->flags & HAL_SLEEP_NETWORK_FLAG_INACTIVE_STANDBY)) {
                valid++;
            }
        } else {
            valid++;
        }
        wakeupSource = wakeupSource->next;
    }
    // At least one wakeup source should be configured for stop mode.
    if ((config->mode == HAL_SLEEP_MODE_STOP || config->mode == HAL_SLEEP_MODE_ULTRA_LOW_POWER) && (!config->wakeup_sources || !valid)) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }

    return SYSTEM_ERROR_NONE;
}

int hal_sleep_enter(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeup_source, void* reserved) {
    CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);

    // Check it again just in case.
    CHECK(hal_sleep_validate_config(config, nullptr));

    int ret = SYSTEM_ERROR_NONE;

    switch (config->mode) {
        case HAL_SLEEP_MODE_STOP:
        case HAL_SLEEP_MODE_ULTRA_LOW_POWER: {
            ret = enterStopBasedSleep(config, wakeup_source);
            break;
        }
        case HAL_SLEEP_MODE_HIBERNATE: {
            ret = enterHibernateMode(config, wakeup_source);
            break;
        }
        default: {
            ret = SYSTEM_ERROR_NOT_SUPPORTED;
            break;
        }
    }

    return ret;
}
