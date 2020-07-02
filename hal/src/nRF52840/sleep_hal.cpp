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
#include <nrfx_types.h>
#include <nrf_mbr.h>
#include <nrf_sdm.h>
#include <nrf_sdh.h>
#include <nrf_rtc.h>
#include <nrf_lpcomp.h>
#include <nrfx_gpiote.h>
#include <nrf_drv_clock.h>
#include <nrf_pwm.h>
#include "usb_hal.h"
#include "usart_hal.h"
#include "flash_common.h"
#include "exflash_hal.h"
#include "rtc_hal.h"
#include "timer_hal.h"
#include "ble_hal.h"
#include "interrupts_hal.h"
#include "concurrent_hal.h"
#include "check.h"
#if HAL_PLATFORM_EXTERNAL_RTC
#include "exrtc_hal.h"
#endif

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
    if (gpio->pin >= TOTAL_PINS) {
        return SYSTEM_ERROR_LIMIT_EXCEEDED;
    }
    return SYSTEM_ERROR_NONE;
}

static int validateRtcWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_rtc_t* rtc) {
    if (rtc->ms == 0) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (mode == HAL_SLEEP_MODE_HIBERNATE) {
#if HAL_PLATFORM_EXTERNAL_RTC
        if ((rtc->ms / 1000) == 0) {
            return SYSTEM_ERROR_INVALID_ARGUMENT;
        }
#else
        return SYSTEM_ERROR_NOT_SUPPORTED;
#endif
    }
    return SYSTEM_ERROR_NONE;
}

static int validateNetworkWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_network_t* network) {
    // FIXME: this is actually not implemented
    return SYSTEM_ERROR_NONE;
}

static int validateBleWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_base_t* base) {
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
    } else if (base->type == HAL_WAKEUP_SOURCE_TYPE_BLE) {
        return validateBleWakeupSource(mode, base);
    }
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

static void fpu_sleep_prepare(void) {
    uint32_t fpscr;
    fpscr = __get_FPSCR();
    /*
     * Clear FPU exceptions.
     * Without this step, the FPU interrupt is marked as pending,
     * preventing system from sleeping. Exceptions cleared:
     * - IOC - Invalid Operation cumulative exception bit.
     * - DZC - Division by Zero cumulative exception bit.
     * - OFC - Overflow cumulative exception bit.
     * - UFC - Underflow cumulative exception bit.
     * - IXC - Inexact cumulative exception bit.
     * - IDC - Input Denormal cumulative exception bit.
     */
    __set_FPSCR(fpscr & ~0x9Fu);
    __DMB();
    NVIC_ClearPendingIRQ(FPU_IRQn);

    /*__
     * Assert no critical FPU exception is signaled:
     * - IOC - Invalid Operation cumulative exception bit.
     * - DZC - Division by Zero cumulative exception bit.
     * - OFC - Overflow cumulative exception bit.
     */
    SPARK_ASSERT((fpscr & 0x07) == 0);
}

static int enterStopMode(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeupReason) {
    int ret = SYSTEM_ERROR_NONE;

    // Detach USB
    HAL_USB_Detach();

    // Flush all USARTs
    for (int usart = 0; usart < TOTAL_USARTS; usart++) {
        if (HAL_USART_Is_Enabled(static_cast<HAL_USART_Serial>(usart))) {
            HAL_USART_Flush_Data(static_cast<HAL_USART_Serial>(usart));
        }
    }

    // Make sure we acquire exflash lock BEFORE going into a critical section
    hal_exflash_lock();

    // BLE events should be dealt before disabling thread scheduling.
    bool bleEnabled = false;
    auto wakeupSource = config->wakeup_sources;
    while (wakeupSource) {
        if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_BLE) {
            bleEnabled = true;
            break;
        }
        wakeupSource = wakeupSource->next;
    }
    bool advertising = hal_ble_gap_is_advertising(nullptr); // We can now only restore advertising after exiting sleep mode.
    if (!bleEnabled) {
        // Make sure we acquire BLE lock BEFORE going into a critical section
        hal_ble_lock(nullptr);
        hal_ble_stack_deinit(nullptr);
    }

    // Disable thread scheduling
    os_thread_scheduling(false, nullptr);

    // This will disable all but SoftDevice interrupts (by modifying NVIC->ICER)
    uint8_t st = 0;
    sd_nvic_critical_region_enter(&st);

    // Disable SysTick
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

    // Put external flash into sleep and disable QSPI peripheral
    hal_exflash_special_command(HAL_EXFLASH_SPECIAL_SECTOR_NONE, HAL_EXFLASH_COMMAND_SLEEP, nullptr, nullptr, 0);
    hal_exflash_uninit();

    // Reducing power consumption
    // Suspend all PWM instance remembering their state
    bool pwmState[4] = {
        (bool)(NRF_PWM0->ENABLE & PWM_ENABLE_ENABLE_Msk),
        (bool)(NRF_PWM1->ENABLE & PWM_ENABLE_ENABLE_Msk),
        (bool)(NRF_PWM2->ENABLE & PWM_ENABLE_ENABLE_Msk),
        (bool)(NRF_PWM3->ENABLE & PWM_ENABLE_ENABLE_Msk),
    };
    nrf_pwm_disable(NRF_PWM0);
    nrf_pwm_disable(NRF_PWM1);
    nrf_pwm_disable(NRF_PWM2);
    nrf_pwm_disable(NRF_PWM3);

    // _Attempt_ to disable HFCLK. This may not succeed, resulting in a higher current consumption
    // FIXME
    bool hfclkResume = false;
    if (nrf_drv_clock_hfclk_is_running()) {
        hfclkResume = true;

        // Temporarily enable SoftDevice API interrupts just in case
        uint32_t basePri = __get_BASEPRI();
        // We are also allowing IRQs with priority = 5
        // FIXME: because that's what OpenThread used for its SWI3
        __set_BASEPRI(_PRIO_APP_LOW << (8 - __NVIC_PRIO_BITS));
        sd_nvic_critical_region_exit(st);
        {
            nrf_drv_clock_hfclk_release();
            // while (nrf_drv_clock_hfclk_is_running());
        }
        // And disable again
        sd_nvic_critical_region_enter(&st);
        __set_BASEPRI(basePri);
    }

    // Remember current microsecond counter
    uint64_t microsBeforeSleep = hal_timer_micros(nullptr);
    // Disable hal_timer (RTC2)
    hal_timer_deinit(nullptr);

    // Make sure LFCLK is running
    nrf_drv_clock_lfclk_request(nullptr);
    while (!nrf_drv_clock_lfclk_is_running());

    // Configure RTC2 to count at 125ms interval. This should allow us to sleep
    // (2^24 - 1) * 125ms = 24 days
    NVIC_SetPriority(RTC2_IRQn, 6);
    // Make sure that the RTC is stopped and cleared
    nrf_rtc_task_trigger(NRF_RTC2, NRF_RTC_TASK_STOP);
    nrf_rtc_task_trigger(NRF_RTC2, NRF_RTC_TASK_CLEAR);
    __DSB();
    __ISB();

    // For some reason even though the RTC should be stopped by now,
    // the prescaler is still read-only. So, we loop here to make sure that the
    // prescaler settings does take effect.
    static const uint32_t prescaler = 4095;
    while (rtc_prescaler_get(NRF_RTC2) != prescaler) {
        nrf_rtc_prescaler_set(NRF_RTC2, prescaler);
        __DSB();
        __ISB();
    }
    // Start RTC2
    nrf_rtc_task_trigger(NRF_RTC2, NRF_RTC_TASK_START);
    __DSB();
    __ISB();

    // Workaround for FPU anomaly
    fpu_sleep_prepare();

    // Suspend all GPIOTE interrupts
    HAL_Interrupts_Suspend();

    uint32_t gpiotePriority = 0;
    uint32_t rtc2Priority = 0;

    uint32_t gpioIntenSet = 0;
    Hal_Pin_Info* halPinMap = HAL_Pin_Map();

    wakeupSource = config->wakeup_sources;
    while (wakeupSource) {
        if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            nrf_gpio_pin_pull_t wakeupPinMode;
            nrf_gpio_pin_sense_t wakeupPinSense;
            nrf_gpiote_polarity_t wakeupPinPolarity;
            auto gpioWakeup = reinterpret_cast<hal_wakeup_source_gpio_t*>(wakeupSource);
            switch(gpioWakeup->mode) {
                case RISING: {
                    wakeupPinMode = NRF_GPIO_PIN_PULLDOWN;
                    wakeupPinSense = NRF_GPIO_PIN_SENSE_HIGH;
                    wakeupPinPolarity = NRF_GPIOTE_POLARITY_LOTOHI;
                    break;
                }
                case FALLING: {
                    wakeupPinMode = NRF_GPIO_PIN_PULLUP;
                    wakeupPinSense = NRF_GPIO_PIN_SENSE_LOW;
                    wakeupPinPolarity = NRF_GPIOTE_POLARITY_HITOLO;
                    break;
                }
                case CHANGE:
                default: {
                    wakeupPinMode = NRF_GPIO_PIN_NOPULL;
                    wakeupPinPolarity = NRF_GPIOTE_POLARITY_TOGGLE;
                    break;
                }
            }
            // For any pin that is not currently configured in GPIOTE with IN event
            // we are going to use low power PORT events
            uint32_t nrfPin = NRF_GPIO_PIN_MAP(halPinMap[gpioWakeup->pin].gpio_port, halPinMap[gpioWakeup->pin].gpio_pin);
            // Set pin mode
            nrf_gpio_cfg_input(nrfPin, wakeupPinMode);
            bool usePortEvent = true;
            for (unsigned i = 0; i < GPIOTE_CH_NUM; ++i) {
                if ((nrf_gpiote_event_pin_get(i) == nrfPin) && nrf_gpiote_int_is_enabled(NRF_GPIOTE_INT_IN0_MASK << i)) {
                    // We have to use IN event for this pin in order to successfully execute interrupt handler
                    usePortEvent = false;
                    nrf_gpiote_event_configure(i, nrfPin, wakeupPinPolarity);
                    gpioIntenSet |= NRF_GPIOTE_INT_IN0_MASK << i;
                    break;
                }
            }
            if (usePortEvent) {
                // Use PORT for this pin
                if (wakeupPinMode == NRF_GPIO_PIN_NOPULL) {
                    // Read current state, choose sense accordingly
                    // Dummy read just in case
                    (void)nrf_gpio_pin_read(nrfPin);
                    uint32_t cur_state = nrf_gpio_pin_read(nrfPin);
                    if (cur_state) {
                        nrf_gpio_cfg_sense_set(nrfPin, NRF_GPIO_PIN_SENSE_LOW);
                    } else {
                        nrf_gpio_cfg_sense_set(nrfPin, NRF_GPIO_PIN_SENSE_HIGH);
                    }
                } else {
                    nrf_gpio_cfg_sense_input(nrfPin, wakeupPinMode, wakeupPinSense);
                }
                gpioIntenSet |= NRF_GPIOTE_INT_PORT_MASK;
            }
        } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            auto rtcWakeup = reinterpret_cast<hal_wakeup_source_rtc_t*>(wakeupSource);
            uint32_t ticks = rtcWakeup->ms / 125;
            // Reconfigure RTC2 for wake-up
            NVIC_ClearPendingIRQ(RTC2_IRQn);

            nrf_rtc_event_clear(NRF_RTC2, NRF_RTC_EVENT_TICK);
            nrf_rtc_event_enable(NRF_RTC2, RTC_EVTEN_TICK_Msk);
            // Make sure that RTC is ticking
            // See 'TASK and EVENT jitter/delay'
            // http://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.nrf52840.ps%2Frtc.html
            while (!nrf_rtc_event_pending(NRF_RTC2, NRF_RTC_EVENT_TICK));

            nrf_rtc_event_disable(NRF_RTC2, RTC_EVTEN_TICK_Msk);
            nrf_rtc_event_clear(NRF_RTC2, NRF_RTC_EVENT_TICK);

            nrf_rtc_event_clear(NRF_RTC2, NRF_RTC_EVENT_COMPARE_0);
            nrf_rtc_event_disable(NRF_RTC2, RTC_EVTEN_COMPARE0_Msk);

            // Configure CC0
            uint32_t counter = nrf_rtc_counter_get(NRF_RTC2);
            uint32_t cc = counter + ticks;
            NVIC_EnableIRQ(RTC2_IRQn);
            nrf_rtc_cc_set(NRF_RTC2, 0, cc);
            nrf_rtc_event_clear(NRF_RTC2, NRF_RTC_EVENT_COMPARE_0);
            nrf_rtc_int_enable(NRF_RTC2, NRF_RTC_INT_COMPARE0_MASK);
            nrf_rtc_event_enable(NRF_RTC2, RTC_EVTEN_COMPARE0_Msk);
        }
        wakeupSource = wakeupSource->next;
    }

    if (gpioIntenSet > 0) {
        // Disable unnecessary GPIOTE interrupts
        uint32_t curIntenSet = NRF_GPIOTE->INTENSET;
        nrf_gpiote_int_disable(curIntenSet ^ gpioIntenSet);
        nrf_gpiote_int_enable(gpioIntenSet);
        // Clear events and interrupts
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_IN_0);
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_IN_1);
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_IN_2);
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_IN_3);
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_IN_4);
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_IN_5);
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_IN_6);
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_IN_7);
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_PORT);
        NVIC_ClearPendingIRQ(GPIOTE_IRQn);
        NVIC_EnableIRQ(GPIOTE_IRQn);
    }

    // Masks all interrupts lower than softdevice. This allows us to be woken ONLY by softdevice
    // or GPIOTE and RTC.
    // IMPORTANT: No SoftDevice API calls are allowed until HAL_enable_irq()
    int hst = 0;
    if (!bleEnabled) {
        hst = HAL_disable_irq();
    }

    __DSB();
    __ISB();

    hal_wakeup_source_type_t wakeupSourceType = HAL_WAKEUP_SOURCE_TYPE_UNKNOWN;
    pin_t wakeupPin = PIN_INVALID;

    bool exitSleepMode = false;
    while (true) {
        // Mask interrupts completely
        __disable_irq();

        // Bump the priority。
        wakeupSource = config->wakeup_sources;
        while (wakeupSource) {
            if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
                // Remember the current priority before bumping the priority
                gpiotePriority = NVIC_GetPriority(GPIOTE_IRQn);
                NVIC_SetPriority(GPIOTE_IRQn, 0);
            } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
                // Remember the current priority before bumping the priority
                rtc2Priority = NVIC_GetPriority(RTC2_IRQn);
                NVIC_SetPriority(RTC2_IRQn, 0);
            }
            wakeupSource = wakeupSource->next;
        }

        __DSB();
        __ISB();

        // Go to sleep
        __WFI();

        // Figure out the wakeup source
        wakeupSource = config->wakeup_sources;
        while (wakeupSource) {
            if (NVIC_GetPendingIRQ(GPIOTE_IRQn) && wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
                auto gpioWakeup = reinterpret_cast<hal_wakeup_source_gpio_t*>(wakeupSource);
                if (nrf_gpiote_event_is_set(NRF_GPIOTE_EVENTS_PORT)) {
                    // PORT event
                    uint32_t nrfPin = NRF_GPIO_PIN_MAP(halPinMap[gpioWakeup->pin].gpio_port, halPinMap[gpioWakeup->pin].gpio_pin);
                    nrf_gpio_pin_sense_t sense = nrf_gpio_pin_sense_get(nrfPin);
                    if (sense != NRF_GPIO_PIN_NOSENSE) {
                        uint32_t state = nrf_gpio_pin_read(nrfPin);
                        if ((state && sense == NRF_GPIO_PIN_SENSE_HIGH) || (!state && sense == NRF_GPIO_PIN_SENSE_LOW)) {
                            wakeupPin = gpioWakeup->pin;
                        }
                    }
                } else {
                    // Check IN events
                    for (unsigned i = 0; i < GPIOTE_CH_NUM && wakeupPin == PIN_INVALID; ++i) {
                        if (NRF_GPIOTE->EVENTS_IN[i] && nrf_gpiote_int_is_enabled(NRF_GPIOTE_INT_IN0_MASK << i)) {
                            pin_t pin = NRF_PIN_LOOKUP_TABLE[nrf_gpiote_event_pin_get(i)];
                            if (pin == gpioWakeup->pin) {
                                wakeupPin = gpioWakeup->pin;
                                break; // This break the for() loop only.
                            }
                        }
                    }
                }
                // Figured out the wakeup pin
                if (wakeupPin != PIN_INVALID) {
                    wakeupSourceType = HAL_WAKEUP_SOURCE_TYPE_GPIO;
                    exitSleepMode = true;
                    // Only if we've figured out the wakeup pin, then we stop traversing the wakeup sources list.
                    break;
                }
            } else if (NVIC_GetPendingIRQ(RTC2_IRQn) && wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
                wakeupSourceType = HAL_WAKEUP_SOURCE_TYPE_RTC;
                exitSleepMode = true;
                break; // Stop traversing the wakeup sources list.
            } else if (NVIC_GetPendingIRQ(SD_EVT_IRQn) && wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_BLE) {
                wakeupSourceType = HAL_WAKEUP_SOURCE_TYPE_BLE;
                exitSleepMode = true;
                break; // Stop traversing the wakeup sources list.
            }
            wakeupSource = wakeupSource->next;
        }

        // Unbump the priority before __enable_irq().
        wakeupSource = config->wakeup_sources;
        while (wakeupSource) {
            if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
                NVIC_SetPriority(GPIOTE_IRQn, gpiotePriority);
            } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
                NVIC_SetPriority(RTC2_IRQn, rtc2Priority);
            }
            wakeupSource = wakeupSource->next;
        }

        // Unmask interrupts so that SoftDevice can still process BLE events.
        // we are still under the effect of HAL_disable_irq that masked all but SoftDevice interrupts using BASEPRI
        __enable_irq();

        // Exit the while(true) loop to exit from sleep mode.
        if (exitSleepMode) {
            break;
        }
    }

    // Restore HFCLK
    if (hfclkResume) {
        // Temporarily enable SoftDevice API interrupts
        uint32_t basePri = __get_BASEPRI();
        __set_BASEPRI(_PRIO_SD_LOWEST << (8 - __NVIC_PRIO_BITS));
        {
            nrf_drv_clock_hfclk_request(nullptr);
            while (!nrf_drv_clock_hfclk_is_running()) {
                ;
            }
        }
        // And disable again
        __set_BASEPRI(basePri);
    }

    // Count the number of microseconds we've slept and reconfigure hal_timer (RTC2)
    uint64_t microsAftereSleep = (uint64_t)nrf_rtc_counter_get(NRF_RTC2) * 125000;
    // Stop RTC2
    nrf_rtc_task_trigger(NRF_RTC2, NRF_RTC_TASK_STOP);
    nrf_rtc_task_trigger(NRF_RTC2, NRF_RTC_TASK_CLEAR);
    // Reconfigure it hal_timer_init() and apply the offset
    hal_timer_init_config_t halTimerConfig = {
        .size = sizeof(hal_timer_init_config_t),
        .version = 0,
        .base_clock_offset = microsBeforeSleep + microsAftereSleep
    };
    hal_timer_init(&halTimerConfig);

    // Restore GPIOTE cionfiguration
    HAL_Interrupts_Restore();

    // Re-initialize external flash
    hal_exflash_init();

    // Re-enable PWM
    // FIXME: this is not ideal but will do for now
    {
        NRF_PWM_Type* const pwms[] = {NRF_PWM0, NRF_PWM1, NRF_PWM2, NRF_PWM3};
        for (unsigned i = 0; i < sizeof(pwms) / sizeof(pwms[0]); i++) {
            NRF_PWM_Type* pwm = pwms[i];
            if (pwmState[i]) {
                nrf_pwm_enable(pwm);
                if (nrf_pwm_event_check(pwm, NRF_PWM_EVENT_SEQEND0) || nrf_pwm_event_check(pwm, NRF_PWM_EVENT_SEQEND1)) {
                    nrf_pwm_event_clear(pwm, NRF_PWM_EVENT_SEQEND0);
                    nrf_pwm_event_clear(pwm, NRF_PWM_EVENT_SEQEND1);
                    if (pwm->SEQ[0].PTR && pwm->SEQ[0].CNT) {
                        nrf_pwm_task_trigger(pwm, NRF_PWM_TASK_SEQSTART0);
                    } else if (pwm->SEQ[1].PTR && pwm->SEQ[1].CNT) {
                        nrf_pwm_task_trigger(pwm, NRF_PWM_TASK_SEQSTART1);
                    }
                }
            }
        }
    }

    // Re-enable SysTick
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

    // This will reenable all non-SoftDevice interrupts previously disabled
    // by sd_nvic_critical_region_enter()
    sd_nvic_critical_region_exit(st);

    // Unmasks all non-softdevice interrupts
    HAL_enable_irq(hst);

    // Release LFCLK
    nrf_drv_clock_lfclk_release();

    // Re-enable USB
    HAL_USB_Attach();

    // Unlock external flash
    hal_exflash_unlock();

    if (!bleEnabled) {
        hal_ble_stack_init(nullptr);
        if (advertising) {
            hal_ble_gap_start_advertising(nullptr);
        }
        hal_ble_unlock(nullptr);
    }

    // Enable thread scheduling
    os_thread_scheduling(true, nullptr);

    if (wakeupReason) {
        if (wakeupSourceType == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            hal_wakeup_source_gpio_t* gpio = (hal_wakeup_source_gpio_t*)malloc(sizeof(hal_wakeup_source_gpio_t));
            if (gpio) {
                gpio->base.size = sizeof(hal_wakeup_source_gpio_t);
                gpio->base.version = HAL_SLEEP_VERSION;
                gpio->base.type = HAL_WAKEUP_SOURCE_TYPE_GPIO;
                gpio->base.next = nullptr;
                gpio->pin = wakeupPin;
                *wakeupReason = reinterpret_cast<hal_wakeup_source_base_t*>(gpio);
            } else {
                ret = SYSTEM_ERROR_NO_MEMORY;
            }
        } else if (wakeupSourceType == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            hal_wakeup_source_rtc_t* rtc = (hal_wakeup_source_rtc_t*)malloc(sizeof(hal_wakeup_source_rtc_t));
            if (rtc) {
                rtc->base.size = sizeof(hal_wakeup_source_rtc_t);
                rtc->base.version = HAL_SLEEP_VERSION;
                rtc->base.type = HAL_WAKEUP_SOURCE_TYPE_RTC;
                rtc->base.next = nullptr;
                rtc->ms = 0;
                *wakeupReason = reinterpret_cast<hal_wakeup_source_base_t*>(rtc);
            } else {
                ret = SYSTEM_ERROR_NO_MEMORY;
            }
        } else if (wakeupSourceType == HAL_WAKEUP_SOURCE_TYPE_BLE) {
            hal_wakeup_source_base_t* ble = (hal_wakeup_source_base_t*)malloc(sizeof(hal_wakeup_source_base_t));
            if (ble) {
                ble->size = sizeof(hal_wakeup_source_base_t);
                ble->version = HAL_SLEEP_VERSION;
                ble->type = HAL_WAKEUP_SOURCE_TYPE_BLE;
                ble->next = nullptr;
                *wakeupReason = ble;
            } else {
                ret = SYSTEM_ERROR_NO_MEMORY;
            }
        } else {
            ret = SYSTEM_ERROR_INTERNAL;
        }
    }

    return ret;
}

static int enterHibernateMode(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeupReason) {
#if HAL_PLATFORM_EXTERNAL_RTC
    auto source = config->wakeup_sources;
    while (source) {
        if (source->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            auto rtcWakeup = reinterpret_cast<const hal_wakeup_source_rtc_t*>(source);
            auto seconds = rtcWakeup->ms / 1000;
            struct timeval tv = {
                .tv_sec = seconds,
                .tv_usec = 0
            };
            CHECK(hal_exrtc_set_alarm(&tv, HAL_RTC_ALARM_FLAG_IN, nullptr, nullptr, nullptr));
        }
        source = source->next;
    }
 #endif

    // Make sure we acquire exflash lock BEFORE going into a critical section
    hal_exflash_lock();

    // Disable thread scheduling
    os_thread_scheduling(false, nullptr);

    // This will disable all but SoftDevice interrupts (by modifying NVIC->ICER)
    uint8_t st = 0;
    sd_nvic_critical_region_enter(&st);

    // Disable SysTick
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

    // Put external flash into sleep mode and disable QSPI peripheral
    hal_exflash_special_command(HAL_EXFLASH_SPECIAL_SECTOR_NONE, HAL_EXFLASH_COMMAND_SLEEP, nullptr, nullptr, 0);
    hal_exflash_uninit();

    // Uninit GPIOTE
    nrfx_gpiote_uninit();

    // Disable low power comparator
    nrf_lpcomp_disable();

    // Deconfigure any possible SENSE configuration
    HAL_Interrupts_Suspend();

    // Disable GPIOTE PORT interrupts
    nrf_gpiote_int_disable(GPIOTE_INTENSET_PORT_Msk);

    // Clear any GPIOTE events
    nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_PORT);

    auto wakeupSource = config->wakeup_sources;
    while (wakeupSource) {
        if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            Hal_Pin_Info* halPinMap = HAL_Pin_Map();
            auto gpioWakeup = reinterpret_cast<hal_wakeup_source_gpio_t*>(wakeupSource);
            nrf_gpio_pin_pull_t wakeupPinMode;
            nrf_gpio_pin_sense_t wakeupPinSense;
            switch(gpioWakeup->mode) {
                case RISING: {
                    wakeupPinMode = NRF_GPIO_PIN_PULLDOWN;
                    wakeupPinSense = NRF_GPIO_PIN_SENSE_HIGH;
                    break;
                }
                case FALLING: {
                    wakeupPinMode = NRF_GPIO_PIN_PULLUP;
                    wakeupPinSense = NRF_GPIO_PIN_SENSE_LOW;
                    break;
                }
                case CHANGE:
                default: {
                    wakeupPinMode = NRF_GPIO_PIN_NOPULL;
                    break;
                }
            }
            uint32_t nrfPin = NRF_GPIO_PIN_MAP(halPinMap[gpioWakeup->pin].gpio_port, halPinMap[gpioWakeup->pin].gpio_pin);
            // Set pin mode
            if (wakeupPinMode == NRF_GPIO_PIN_NOPULL) {
                nrf_gpio_cfg_input(nrfPin, wakeupPinMode);
                // Read current state, choose sense accordingly
                // Dummy read just in case
                (void)nrf_gpio_pin_read(nrfPin);
                uint32_t cur_state = nrf_gpio_pin_read(nrfPin);
                if (cur_state) {
                    nrf_gpio_cfg_sense_set(nrfPin, NRF_GPIO_PIN_SENSE_LOW);
                } else {
                    nrf_gpio_cfg_sense_set(nrfPin, NRF_GPIO_PIN_SENSE_HIGH);
                }
            } else {
                nrf_gpio_cfg_sense_input(nrfPin, wakeupPinMode, wakeupPinSense);
            }
        }
#if HAL_PLATFORM_EXTERNAL_RTC
        else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            Hal_Pin_Info* halPinMap = HAL_Pin_Map();
            uint32_t nrfPin = NRF_GPIO_PIN_MAP(halPinMap[RTC_INT].gpio_port, halPinMap[RTC_INT].gpio_pin);
            nrf_gpio_cfg_sense_input(nrfPin, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
        }
 #endif
        wakeupSource = wakeupSource->next;
    }

    // Disable PWM
    nrf_pwm_disable(NRF_PWM0);
    nrf_pwm_disable(NRF_PWM1);
    nrf_pwm_disable(NRF_PWM2);
    nrf_pwm_disable(NRF_PWM3);

    // RAM retention is configured on early boot in Set_System()

    SPARK_ASSERT(sd_power_system_off() == NRF_SUCCESS);
    while (1);
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
    // At least one wakeup source should be configured for stop mode.
    if (config->mode == HAL_SLEEP_MODE_STOP && !wakeupSource) {
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
