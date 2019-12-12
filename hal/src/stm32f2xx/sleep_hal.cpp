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

#if HAL_PLATFORM_SLEEP_2_0

#include "stm32f2xx.h"
#include "gpio_hal.h"
#include "rtc_hal.h"
#include "usb_hal.h"
#include "usart_hal.h"
#include "check.h"
#include "interrupts_hal.h"
#include "spark_wiring_vector.h"

using spark::Vector;

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

static int validateRtcWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_rtc_t* rtc) {
    if (rtc->ms == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    return SYSTEM_ERROR_NONE;
}

static int validateNetworkWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_network_t* network) {
    if (mode == HAL_SLEEP_MODE_HIBERNATE) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    return SYSTEM_ERROR_NONE;
}

static int validateWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_base_t* base) {
    if (base->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
        return validateGpioWakeupSource(mode, reinterpret_cast<const hal_wakeup_source_gpio_t*>(base));
    } else if (base->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
        return validateRtcWakeupSource(mode, reinterpret_cast<const hal_wakeup_source_rtc_t*>(base));
    } else if (base->type == HAL_WAKEUP_SOURCE_TYPE_NETWORK) {
        return validateNetworkWakeupSource(mode, reinterpret_cast<const hal_wakeup_source_network_t*>(base));
    }
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

static uint32_t enabledWakeupSources(hal_wakeup_source_base_t* base) {
    uint32_t types = HAL_WAKEUP_SOURCE_TYPE_UNKNOWN;
    if (!base) {
        return types;
    }
    while (base) {
        types |= base->type;
        base = base->next;
    }
    return types;
}

static int enterStopMode(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeup_source) {
    int ret = SYSTEM_ERROR_NONE;
    Vector<uint16_t> pins;
    Vector<InterruptMode> modes;
    long seconds = 0;
    uint32_t wakeupSourceTypes = enabledWakeupSources(config->wakeup_sources);

    auto wakeupSource = config->wakeup_sources;
    while (wakeupSource) {
        if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            if (!pins.append(reinterpret_cast<hal_wakeup_source_gpio_t*>(wakeupSource)->pin)) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
            if (!modes.append(reinterpret_cast<hal_wakeup_source_gpio_t*>(wakeupSource)->mode)) {
                return SYSTEM_ERROR_NO_MEMORY;
            }
        } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            seconds = reinterpret_cast<hal_wakeup_source_rtc_t*>(wakeupSource)->ms / 1000;
        }
        wakeupSource = wakeupSource->next;
    }

    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

    // Detach USB
    HAL_USB_Detach();

    // Flush all USARTs
    for (int usart = 0; usart < TOTAL_USARTS; usart++) {
        if (HAL_USART_Is_Enabled(static_cast<HAL_USART_Serial>(usart))) {
            HAL_USART_Flush_Data(static_cast<HAL_USART_Serial>(usart));
        }
    }

    int32_t state = HAL_disable_irq();

    // Suspend all EXTI interrupts
    HAL_Interrupts_Suspend();

    if (wakeupSourceTypes & HAL_WAKEUP_SOURCE_TYPE_GPIO) {
        for (int i = 0; i < pins.size(); i++) {
            PinMode wakeUpPinMode = INPUT;
            /* Set required pinMode based on edgeTriggerMode */
            switch(modes[i]) {
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

            HAL_Pin_Mode(pins[i], wakeUpPinMode);
            HAL_InterruptExtraConfiguration irqConf = {0};
            irqConf.version = HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION_2;
            irqConf.IRQChannelPreemptionPriority = 0;
            irqConf.IRQChannelSubPriority = 0;
            irqConf.keepHandler = 1;
            irqConf.keepPriority = 1;
            HAL_Interrupts_Attach(pins[i], NULL, NULL, modes[i], &irqConf);
        }
    }

    // Configure RTC wake-up
    if (wakeupSourceTypes & HAL_WAKEUP_SOURCE_TYPE_RTC) {
        /*
         * - To wake up from the Stop mode with an RTC alarm event, it is necessary to:
         * - Configure the EXTI Line 17 to be sensitive to rising edges (Interrupt
         * or Event modes) using the EXTI_Init() function.
         *
         */
        HAL_RTC_Cancel_UnixAlarm();
        HAL_RTC_Set_UnixAlarm((time_t) seconds);

        // Connect RTC to EXTI line
        EXTI_InitTypeDef EXTI_InitStructure = {0};
        EXTI_InitStructure.EXTI_Line = EXTI_Line17;
        EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
        EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
        EXTI_InitStructure.EXTI_LineCmd = ENABLE;
        EXTI_Init(&EXTI_InitStructure);
    }

    {
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
    }

    if ((wakeupSourceTypes & HAL_WAKEUP_SOURCE_TYPE_RTC) && NVIC_GetPendingIRQ(RTC_Alarm_IRQn)) {
        if (wakeup_source) {
            hal_wakeup_source_rtc_t* rtc_wakeup = (hal_wakeup_source_rtc_t*)malloc(sizeof(hal_wakeup_source_rtc_t));
            if (!rtc_wakeup) {
                ret = SYSTEM_ERROR_NO_MEMORY;
            } else {
                rtc_wakeup->base.size = sizeof(hal_wakeup_source_rtc_t);
                rtc_wakeup->base.version = HAL_SLEEP_VERSION;
                rtc_wakeup->base.type = HAL_WAKEUP_SOURCE_TYPE_RTC;
                rtc_wakeup->base.next = nullptr;
                rtc_wakeup->ms = 0;
                *wakeup_source = reinterpret_cast<hal_wakeup_source_base_t*>(rtc_wakeup);
            }
        }
    } else if (wakeupSourceTypes & HAL_WAKEUP_SOURCE_TYPE_GPIO) {
        Hal_Pin_Info* PIN_MAP = HAL_Pin_Map();
        for (auto pin : pins) {
            if (EXTI_GetITStatus(PIN_MAP[pin].gpio_pin) != RESET) {
                if (wakeup_source) {
                    hal_wakeup_source_gpio_t* gpio_wakeup = (hal_wakeup_source_gpio_t*)malloc(sizeof(hal_wakeup_source_gpio_t));
                    if (!gpio_wakeup) {
                        ret = SYSTEM_ERROR_NO_MEMORY;
                    } else {
                        gpio_wakeup->base.size = sizeof(hal_wakeup_source_gpio_t);
                        gpio_wakeup->base.version = HAL_SLEEP_VERSION;
                        gpio_wakeup->base.type = HAL_WAKEUP_SOURCE_TYPE_GPIO;
                        gpio_wakeup->base.next = nullptr;
                        gpio_wakeup->pin = pin;
                        *wakeup_source = reinterpret_cast<hal_wakeup_source_base_t*>(gpio_wakeup);
                    }
                }
                break;
            }
        }
    }

    if (wakeupSourceTypes & HAL_WAKEUP_SOURCE_TYPE_GPIO) {
        for (auto pin : pins) {
            /* Detach the Interrupt pin */
            HAL_Interrupts_Detach_Ext(pin, 1, NULL);
        }
    }

    if (wakeupSourceTypes & HAL_WAKEUP_SOURCE_TYPE_RTC) {
        // No need to detach RTC Alarm from EXTI, since it will be detached in HAL_Interrupts_Restore()
        // RTC Alarm should be canceled to avoid entering HAL_RTCAlarm_Handler or if we were woken up by pin
        HAL_RTC_Cancel_UnixAlarm();
    }

    // Restore
    HAL_Interrupts_Restore();

    // Successfully exited STOP mode
    HAL_enable_irq(state);

    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

    HAL_USB_Attach();

    return ret;
}

static int enterHibernateMode(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeup_source) {
    long seconds = 0;
    uint32_t wakeupSourceTypes = enabledWakeupSources(config->wakeup_sources);

    auto wakeupSource = config->wakeup_sources;
    while (wakeupSource) {
        if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            seconds = reinterpret_cast<hal_wakeup_source_rtc_t*>(wakeupSource)->ms / 1000;
        }
        wakeupSource = wakeupSource->next;
    }

    // Configure RTC wake-up
    if (wakeupSourceTypes & HAL_WAKEUP_SOURCE_TYPE_RTC) {
        HAL_RTC_Cancel_UnixAlarm();
        HAL_RTC_Set_UnixAlarm((time_t) seconds);
    }

    if (wakeupSourceTypes & HAL_WAKEUP_SOURCE_TYPE_GPIO) {
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
    if (config->mode == HAL_SLEEP_MODE_ULTRA_LOW_POWER) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }

    // Checks the wakeup sources
    auto wakeupSource = config->wakeup_sources;
    // For backward compatibility, Gen2 platforms can disable WKP pin.
    // if (!wakeupSource) {
    //     return SYSTEM_ERROR_INVALID_ARGUMENT;
    // }
    while (wakeupSource) {
        CHECK(validateWakeupSource(config->mode, wakeupSource));
        wakeupSource = wakeupSource->next;
    }

    return SYSTEM_ERROR_NONE;
}

int hal_sleep_enter(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeup_source, void* reserved) {
    CHECK_TRUE(config, SYSTEM_ERROR_INVALID_ARGUMENT);

    // Check it again just in case.
    CHECK(hal_sleep_validate_config(config, nullptr));

    int ret = SYSTEM_ERROR_NONE;

    switch (config->mode) {
        case HAL_SLEEP_MODE_STOP: {
            ret = enterStopMode(config, wakeup_source);
            break;
        }
        case HAL_SLEEP_MODE_ULTRA_LOW_POWER: {
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

#endif // HAL_PLATFORM_SLEEP_2_0
