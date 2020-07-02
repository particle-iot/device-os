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

static int configGpioWakeupSource(const hal_wakeup_source_base_t* wakeupSources) {
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
            HAL_InterruptExtraConfiguration irqConf = {0};
            irqConf.version = HAL_INTERRUPT_EXTRA_CONFIGURATION_VERSION_2;
            irqConf.IRQChannelPreemptionPriority = 0;
            irqConf.IRQChannelSubPriority = 0;
            irqConf.keepHandler = 1;
            irqConf.keepPriority = 1;
            HAL_Interrupts_Attach(gpioWakeup->pin, nullptr, nullptr, gpioWakeup->mode, &irqConf);
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
            EXTI_InitTypeDef extiInitStructure = {0};
            extiInitStructure.EXTI_Line = EXTI_Line17;
            extiInitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
            extiInitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
            extiInitStructure.EXTI_LineCmd = ENABLE;
            EXTI_Init(&extiInitStructure);

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
    if (rtc->ms < 1000) {
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

static void sleep() {
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

static int enterStopMode(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeupReason) {
    int ret = SYSTEM_ERROR_NONE;

    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

    // Detach USB
    HAL_USB_Detach();

    // Flush all USARTs
    for (int usart = 0; usart < TOTAL_USARTS; usart++) {
        if (hal_usart_is_enabled(static_cast<hal_usart_interface_t>(usart))) {
            hal_usart_flush(static_cast<hal_usart_interface_t>(usart));
        }
    }

    int32_t state = HAL_disable_irq();

    // Suspend all EXTI interrupts
    HAL_Interrupts_Suspend();

    Hal_Pin_Info* halPinMap = HAL_Pin_Map();

    configGpioWakeupSource(config->wakeup_sources);
    configRtcWakeupSource(config->wakeup_sources);

    sleep();

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
        }
        wakeupSource = wakeupSource->next;
    }

    // FIXME: SHould we enter sleep again if there is no pending IRQn in corresponding to the wakeup source?

    wakeupSource = config->wakeup_sources;
    while (wakeupSource) {
        if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            // No need to detach RTC Alarm from EXTI, since it will be detached in HAL_Interrupts_Restore()
            // RTC Alarm should be canceled to avoid entering HAL_RTCAlarm_Handler or if we were woken up by pin
            hal_rtc_cancel_alarm();
        } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            auto gpioWakeup = reinterpret_cast<hal_wakeup_source_gpio_t*>(wakeupSource);
            HAL_Interrupts_Detach_Ext(gpioWakeup->pin, 1, nullptr);
        }
        wakeupSource = wakeupSource->next;
    }

    // Restore
    HAL_Interrupts_Restore();

    // Successfully exited STOP mode
    HAL_enable_irq(state);

    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

    HAL_USB_Attach();

    return ret;
}

static int enterUltraLowPowerMode(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeupReason) {
    int ret = SYSTEM_ERROR_NONE;

    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

    // Detach USB
    HAL_USB_Detach();

    // Suspend USARTs
    for (int usart = 0; usart < TOTAL_USARTS; usart++) {
        hal_usart_sleep(static_cast<hal_usart_interface_t>(usart), true, nullptr);
    }
    // Suspend SPIs
    for (int spi = 0; spi < TOTAL_SPI; spi++) {
        hal_spi_sleep(static_cast<hal_spi_interface_t>(spi), true, nullptr);
    }
    // Suspend I2Cs
    for (int i2c = 0; i2c < TOTAL_I2C; i2c++) {
        HAL_I2C_Sleep(static_cast<HAL_I2C_Interface>(i2c), true, nullptr);
    }
    hal_pwm_sleep(true, nullptr);
    // Suspend ADC module
    HAL_ADC_Sleep(true, nullptr);

    int32_t state = HAL_disable_irq();

    // Suspend all EXTI interrupts
    HAL_Interrupts_Suspend();

    Hal_Pin_Info* halPinMap = HAL_Pin_Map();

    configGpioWakeupSource(config->wakeup_sources);
    configRtcWakeupSource(config->wakeup_sources);

    sleep();

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
        }
        wakeupSource = wakeupSource->next;
    }

    // FIXME: SHould we enter sleep again if there is no pending IRQn in corresponding to the wakeup source?

    wakeupSource = config->wakeup_sources;
    while (wakeupSource) {
        if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            // No need to detach RTC Alarm from EXTI, since it will be detached in HAL_Interrupts_Restore()
            // RTC Alarm should be canceled to avoid entering HAL_RTCAlarm_Handler or if we were woken up by pin
            hal_rtc_cancel_alarm();
        } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            auto gpioWakeup = reinterpret_cast<hal_wakeup_source_gpio_t*>(wakeupSource);
            HAL_Interrupts_Detach_Ext(gpioWakeup->pin, 1, nullptr);
        }
        wakeupSource = wakeupSource->next;
    }

    // Restore
    HAL_Interrupts_Restore();

    // Successfully exited STOP mode
    HAL_enable_irq(state);

    // Restore ADC state
    HAL_ADC_Sleep(false, nullptr);
    // Restore I2Cs
    for (int i2c = 0; i2c < TOTAL_I2C; i2c++) {
        HAL_I2C_Sleep(static_cast<HAL_I2C_Interface>(i2c), false, nullptr);
    }
    // Restore SPIs
    for (int spi = 0; spi < TOTAL_SPI; spi++) {
        hal_spi_sleep(static_cast<hal_spi_interface_t>(spi), false, nullptr);
    }
    // Restore USARTs
    for (int usart = 0; usart < TOTAL_USARTS; usart++) {
        hal_usart_sleep(static_cast<hal_usart_interface_t>(usart), false, nullptr);
    }

    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

    HAL_USB_Attach();

    return ret;
}

static int enterHibernateMode(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeup_source) {
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
    // At least one wakeup source should be configured for stop mode.
    if ((config->mode == HAL_SLEEP_MODE_STOP || config->mode == HAL_SLEEP_MODE_ULTRA_LOW_POWER) && !wakeupSource) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
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
            ret = enterUltraLowPowerMode(config, wakeup_source);
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
