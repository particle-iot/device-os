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

#include "nrf_uarte.h"
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
#include "ble_hal.h"
#include "interrupts_hal.h"
#include "concurrent_hal.h"
#include "check.h"
#include "radio_common.h"
#if HAL_PLATFORM_EXTERNAL_RTC
#include "exrtc_hal.h"
#endif
#include "spark_wiring_vector.h"
#include "platform_ncp.h"

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
#if HAL_PLATFORM_MCP23S17
#include "mcp23s17.h"
#endif
#endif

using namespace particle;

namespace {

struct WakeupSourcePriorityCache {
    WakeupSourcePriorityCache() {
        memset((uint8_t*)this, 0xFF, sizeof(WakeupSourcePriorityCache));
    }

    uint32_t gpiotePriority;
    uint32_t rtc2Priority;
    uint32_t blePriority;
    uint32_t lpcompPriority;
    uint32_t usart0Priority;
    uint32_t usart1Priority;
};

constexpr uint32_t INVALID_INT_PRIORITY = 0xFFFFFFFF;

constexpr nrf_gpiote_events_t gpioteEvents[GPIOTE_CH_NUM] = {
    NRF_GPIOTE_EVENTS_IN_0,
    NRF_GPIOTE_EVENTS_IN_1,
    NRF_GPIOTE_EVENTS_IN_2,
    NRF_GPIOTE_EVENTS_IN_3,
    NRF_GPIOTE_EVENTS_IN_4,
    NRF_GPIOTE_EVENTS_IN_5,
    NRF_GPIOTE_EVENTS_IN_6,
    NRF_GPIOTE_EVENTS_IN_7
};

} /* Anonymous namespace  */

static void bumpWakeupSourcesPriority(const hal_wakeup_source_base_t* wakeupSources, WakeupSourcePriorityCache* priority, uint32_t newPriority) {
    auto source = wakeupSources;
    while (source) {
        if (source->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            if (priority->gpiotePriority == INVALID_INT_PRIORITY) {
                priority->gpiotePriority = NVIC_GetPriority(GPIOTE_IRQn);
            }
            NVIC_SetPriority(GPIOTE_IRQn, newPriority);
        } else if (source->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            if (priority->rtc2Priority == INVALID_INT_PRIORITY) {
                priority->rtc2Priority = NVIC_GetPriority(RTC2_IRQn);
            }
            NVIC_SetPriority(RTC2_IRQn, newPriority);
        } else if (source->type == HAL_WAKEUP_SOURCE_TYPE_BLE) {
            if (priority->blePriority == INVALID_INT_PRIORITY) {
                priority->blePriority = NVIC_GetPriority(SD_EVT_IRQn);
            }
            NVIC_SetPriority(SD_EVT_IRQn, newPriority);
            NVIC_EnableIRQ(SD_EVT_IRQn);
        } else if (source->type == HAL_WAKEUP_SOURCE_TYPE_LPCOMP) {
            if (priority->lpcompPriority == INVALID_INT_PRIORITY) {
                priority->lpcompPriority = NVIC_GetPriority(COMP_LPCOMP_IRQn);
            }
            NVIC_SetPriority(COMP_LPCOMP_IRQn, newPriority);
            NVIC_EnableIRQ(COMP_LPCOMP_IRQn);
        } else if (source->type == HAL_WAKEUP_SOURCE_TYPE_USART) {
            if (priority->usart0Priority == INVALID_INT_PRIORITY) {
                priority->usart0Priority = NVIC_GetPriority(UARTE0_UART0_IRQn);
            }
            NVIC_SetPriority(UARTE0_UART0_IRQn, newPriority);
            nrf_uarte_int_enable(NRF_UARTE0, NRF_UARTE_INT_RXDRDY_MASK);
        } else if (source->type == HAL_WAKEUP_SOURCE_TYPE_NETWORK) {
            auto network = reinterpret_cast<const hal_wakeup_source_network_t*>(source);
            if (!(network->flags & HAL_SLEEP_NETWORK_FLAG_INACTIVE_STANDBY)) {
                if (priority->usart1Priority == INVALID_INT_PRIORITY) {
                    priority->usart1Priority = NVIC_GetPriority(UARTE1_IRQn);
                }
                NVIC_SetPriority(UARTE1_IRQn, newPriority);
                nrf_uarte_int_enable(NRF_UARTE1, NRF_UARTE_INT_RXDRDY_MASK);
            }
        }
        source = source->next;
    }
}

static void unbumpWakeupSourcesPriority(const hal_wakeup_source_base_t* wakeupSources, WakeupSourcePriorityCache* priority) {
    auto source = wakeupSources;
    while (source) {
        if (source->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            if (priority->gpiotePriority != INVALID_INT_PRIORITY) {
                NVIC_SetPriority(GPIOTE_IRQn, priority->gpiotePriority);
                priority->gpiotePriority = INVALID_INT_PRIORITY;
            }
        } else if (source->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            if (priority->rtc2Priority != INVALID_INT_PRIORITY) {
                NVIC_SetPriority(RTC2_IRQn, priority->rtc2Priority);
                priority->rtc2Priority = INVALID_INT_PRIORITY;
            }
        } else if (source->type == HAL_WAKEUP_SOURCE_TYPE_BLE) {
            if (priority->blePriority != INVALID_INT_PRIORITY) {
                NVIC_SetPriority(SD_EVT_IRQn, priority->blePriority);
                priority->blePriority = INVALID_INT_PRIORITY;
            }
            NVIC_DisableIRQ(SD_EVT_IRQn);
        } else if (source->type == HAL_WAKEUP_SOURCE_TYPE_LPCOMP) {
            // FIXME: dirty hack, since we nowhere implemented the IRQ handler.
            nrf_lpcomp_event_clear(NRF_LPCOMP_EVENT_READY);
            nrf_lpcomp_event_clear(NRF_LPCOMP_EVENT_DOWN);
            nrf_lpcomp_event_clear(NRF_LPCOMP_EVENT_UP);
            nrf_lpcomp_event_clear(NRF_LPCOMP_EVENT_CROSS);
            NVIC_DisableIRQ(COMP_LPCOMP_IRQn);
            NVIC_ClearPendingIRQ(COMP_LPCOMP_IRQn);
            if (priority->lpcompPriority != INVALID_INT_PRIORITY) {
                NVIC_SetPriority(COMP_LPCOMP_IRQn, priority->lpcompPriority);
                priority->lpcompPriority = INVALID_INT_PRIORITY;
            }
        } else if (source->type == HAL_WAKEUP_SOURCE_TYPE_USART) {
            if (priority->usart0Priority != INVALID_INT_PRIORITY) {
                NVIC_SetPriority(UARTE0_UART0_IRQn, priority->usart0Priority);
                priority->usart0Priority = INVALID_INT_PRIORITY;
            }
            nrf_uarte_int_disable(NRF_UARTE0, NRF_UARTE_INT_RXDRDY_MASK);
        } else if (source->type == HAL_WAKEUP_SOURCE_TYPE_NETWORK) {
            auto network = reinterpret_cast<const hal_wakeup_source_network_t*>(source);
            if (!(network->flags & HAL_SLEEP_NETWORK_FLAG_INACTIVE_STANDBY)) {
                if (priority->usart1Priority != INVALID_INT_PRIORITY) {
                    NVIC_SetPriority(UARTE1_IRQn, priority->usart1Priority);
                    priority->usart1Priority = INVALID_INT_PRIORITY;
                }
                nrf_uarte_int_disable(NRF_UARTE1, NRF_UARTE_INT_RXDRDY_MASK);
            }
        }
        source = source->next;
    }
}

static int constructGpioWakeupReason(hal_wakeup_source_base_t** wakeupReason, hal_pin_t pin) {
    auto gpio = (hal_wakeup_source_gpio_t*)malloc(sizeof(hal_wakeup_source_gpio_t));
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
    return SYSTEM_ERROR_NONE;
}

static int constructRtcWakeupReason(hal_wakeup_source_base_t** wakeupReason) {
    auto rtc = (hal_wakeup_source_rtc_t*)malloc(sizeof(hal_wakeup_source_rtc_t));
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
    return SYSTEM_ERROR_NONE;
}

static int constructUsartWakeupReason(hal_wakeup_source_base_t** wakeupReason) {
    auto usart = (hal_wakeup_source_usart_t*)malloc(sizeof(hal_wakeup_source_usart_t));
    if (usart) {
        usart->base.size = sizeof(hal_wakeup_source_usart_t);
        usart->base.version = HAL_SLEEP_VERSION;
        usart->base.type = HAL_WAKEUP_SOURCE_TYPE_USART;
        usart->base.next = nullptr;
        usart->serial = HAL_USART_SERIAL1;
        *wakeupReason = reinterpret_cast<hal_wakeup_source_base_t*>(usart);
    } else {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    return SYSTEM_ERROR_NONE;
}

static int constructBleWakeupReason(hal_wakeup_source_base_t** wakeupReason) {
    auto ble = (hal_wakeup_source_base_t*)malloc(sizeof(hal_wakeup_source_base_t));
    if (ble) {
        ble->size = sizeof(hal_wakeup_source_base_t);
        ble->version = HAL_SLEEP_VERSION;
        ble->type = HAL_WAKEUP_SOURCE_TYPE_BLE;
        ble->next = nullptr;
        *wakeupReason = ble;
    } else {
        return SYSTEM_ERROR_NO_MEMORY;
    }
    return SYSTEM_ERROR_NONE;
}

static int constructLpcompWakeupReason(hal_wakeup_source_base_t** wakeupReason, hal_pin_t pin) {
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
    return SYSTEM_ERROR_NONE;
}

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

static const hal_wakeup_source_base_t* findWakeupSource(const hal_wakeup_source_base_t* wakeupSources, hal_wakeup_source_type_t type) {
    auto source = wakeupSources;
    while (source) {
        if (source->type == type) {
            return source;
        }
        source = source->next;
    }
    return nullptr;
}

static void configGpioWakeupSource(const hal_wakeup_source_base_t* wakeupSources) {
    uint32_t gpioIntenSet = 0;
    hal_pin_info_t* halPinMap = hal_pin_map();

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
#if HAL_PLATFORM_MCP23S17
    bool ioExpanderIntConfigured = false;
#endif
#endif

    auto source = wakeupSources;
    while (source) {
        if (source->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            nrf_gpio_pin_pull_t wakeupPinMode;
            nrf_gpio_pin_sense_t wakeupPinSense;
            nrf_gpiote_polarity_t wakeupPinPolarity;
            uint32_t nrfPin;
            auto gpioWakeup = reinterpret_cast<const hal_wakeup_source_gpio_t*>(source);
#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
            if (halPinMap[gpioWakeup->pin].type == HAL_PIN_TYPE_MCU) {
#endif
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
                nrfPin = NRF_GPIO_PIN_MAP(halPinMap[gpioWakeup->pin].gpio_port, halPinMap[gpioWakeup->pin].gpio_pin);
#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
            }
#if HAL_PLATFORM_MCP23S17
            else if (halPinMap[gpioWakeup->pin].type == HAL_PIN_TYPE_IO_EXPANDER && !ioExpanderIntConfigured) {
                ioExpanderIntConfigured = true;
                wakeupPinMode = NRF_GPIO_PIN_PULLUP;
                wakeupPinSense = NRF_GPIO_PIN_SENSE_LOW;
                wakeupPinPolarity = NRF_GPIOTE_POLARITY_HITOLO;
                nrfPin = NRF_GPIO_PIN_MAP(halPinMap[IOE_INT].gpio_port, halPinMap[IOE_INT].gpio_pin);
            }
#endif
            else {
                source = source->next;
                continue;
            }
#endif
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
        }
        source = source->next;
    }
    
    if (gpioIntenSet > 0) {
        uint32_t curIntenSet = NRF_GPIOTE->INTENSET;
        nrf_gpiote_int_disable(curIntenSet);

        // Clear events and interrupts
        uint32_t ioeNrfPin = 0xFFFFFFFF;
#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
#if HAL_PLATFORM_MCP23S17
        /* The GPIOTE event associated with IOE_INT is probably set after we configured
           the IO expander interrupts. Do not clear it and use it to wake up device.
           Otherwise, we will miss the interrupt triggered by IO expander. */
        ioeNrfPin = NRF_GPIO_PIN_MAP(halPinMap[IOE_INT].gpio_port, halPinMap[IOE_INT].gpio_pin);
#endif
#endif
        for (uint8_t i = 0; i < GPIOTE_CH_NUM; i++) {
            if (nrf_gpiote_event_pin_get(i) != ioeNrfPin) {
                nrf_gpiote_event_clear(gpioteEvents[i]);
            }
        }
        nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_PORT);
        NVIC_ClearPendingIRQ(GPIOTE_IRQn);
        nrf_gpiote_int_enable(gpioIntenSet);
        NVIC_EnableIRQ(GPIOTE_IRQn);
    }
}

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
#if HAL_PLATFORM_MCP23S17
static void configGpioWakeupSourceExt(const hal_wakeup_source_base_t* wakeupSources, hal_sleep_mode_t sleepMode) {
    hal_pin_info_t* halPinMap = hal_pin_map();
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
        // Disabled GPIOTE NVIC interrupt so that ISR associated with IOE_INT won't
        // be executed if interrupt occurred previous to calling __disable_irq().
        NVIC_DisableIRQ(GPIOTE_IRQn);

        // TODO: check the returned result
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

static void configRtcWakeupSource(const hal_wakeup_source_base_t* wakeupSources) {
    auto source = wakeupSources;
    while (source) {
        if (source->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            auto rtcWakeup = reinterpret_cast<const hal_wakeup_source_rtc_t*>(source);
            uint32_t ticks = rtcWakeup->ms / 125;
            // Reconfigure RTC2 for wake-up
            NVIC_ClearPendingIRQ(RTC2_IRQn);

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
            break; // Stop traversing the list.
        }
        source = source->next;
    }
}

static void configLpcompWakeupSource(const hal_wakeup_source_base_t* wakeupSources) {
    hal_pin_info_t* halPinMap = hal_pin_map();
    auto source = wakeupSources;
    while (source) {
        if (source->type == HAL_WAKEUP_SOURCE_TYPE_LPCOMP) {
            auto lpcompWakeup = reinterpret_cast<const hal_wakeup_source_lpcomp_t*>(source);
            nrf_lpcomp_config_t config = {};

            NVIC_DisableIRQ(COMP_LPCOMP_IRQn);
            NVIC_ClearPendingIRQ(COMP_LPCOMP_IRQn);

            // Reference voltage is VDD/16 * N, N ranges from 1 to 15, VDD/16 = 206mV
            constexpr uint8_t vddDiv16 = 206;
            constexpr nrf_lpcomp_ref_t refs[] = {
                NRF_LPCOMP_REF_SUPPLY_1_16, NRF_LPCOMP_REF_SUPPLY_1_8,
                NRF_LPCOMP_REF_SUPPLY_3_16, NRF_LPCOMP_REF_SUPPLY_2_8,
                NRF_LPCOMP_REF_SUPPLY_5_16, NRF_LPCOMP_REF_SUPPLY_3_8,
                NRF_LPCOMP_REF_SUPPLY_7_16, NRF_LPCOMP_REF_SUPPLY_4_8,
                NRF_LPCOMP_REF_SUPPLY_9_16, NRF_LPCOMP_REF_SUPPLY_5_8,
                NRF_LPCOMP_REF_SUPPLY_11_16, NRF_LPCOMP_REF_SUPPLY_6_8,
                NRF_LPCOMP_REF_SUPPLY_13_16, NRF_LPCOMP_REF_SUPPLY_7_8,
                NRF_LPCOMP_REF_SUPPLY_15_16
            };
            constexpr uint8_t elements = sizeof(refs) / sizeof(nrf_lpcomp_ref_t);
            uint8_t n = (lpcompWakeup->voltage + (vddDiv16 / 2)) / vddDiv16;
            if (n > elements) {
                n = elements;
            }
            if (n > 0) {
                n--;
            }
            config.reference = refs[n];

            switch (lpcompWakeup->trig) {
                case HAL_SLEEP_LPCOMP_ABOVE: config.detection = NRF_LPCOMP_DETECT_UP; break;
                case HAL_SLEEP_LPCOMP_BELOW: config.detection = NRF_LPCOMP_DETECT_DOWN; break;
                case HAL_SLEEP_LPCOMP_CROSS: config.detection = NRF_LPCOMP_DETECT_CROSS; break;
                default: return;
            }
#ifdef LPCOMP_FEATURE_HYST_PRESENT
            config.hyst = NRF_LPCOMP_HYST_50mV;
#endif
            nrf_lpcomp_configure(&config); // It will disable all interrupts.
            nrf_lpcomp_input_select(static_cast<nrf_lpcomp_input_t>(halPinMap[lpcompWakeup->pin].adc_channel));
            nrf_lpcomp_enable();
            nrf_lpcomp_task_trigger(NRF_LPCOMP_TASK_START);
            while (!nrf_lpcomp_event_check(NRF_LPCOMP_EVENT_READY));
            nrf_lpcomp_event_clear(NRF_LPCOMP_EVENT_READY);
            nrf_lpcomp_event_clear(NRF_LPCOMP_EVENT_DOWN);
            nrf_lpcomp_event_clear(NRF_LPCOMP_EVENT_UP);
            nrf_lpcomp_event_clear(NRF_LPCOMP_EVENT_CROSS);

            switch (lpcompWakeup->trig) {
                case HAL_SLEEP_LPCOMP_ABOVE: {
                    nrf_lpcomp_shorts_enable(NRF_LPCOMP_SHORT_UP_STOP_MASK);
                    nrf_lpcomp_int_enable(LPCOMP_INTENSET_UP_Msk);
                    break;
                }
                case HAL_SLEEP_LPCOMP_BELOW: {
                    nrf_lpcomp_shorts_enable(NRF_LPCOMP_SHORT_DOWN_STOP_MASK);
                    nrf_lpcomp_int_enable(LPCOMP_INTENSET_DOWN_Msk);
                    break;
                }
                case HAL_SLEEP_LPCOMP_CROSS: {
                    nrf_lpcomp_shorts_enable(NRF_LPCOMP_SHORT_CROSS_STOP_MASK);
                    nrf_lpcomp_int_enable(LPCOMP_INTENSET_CROSS_Msk);
                    break;
                }
                default: return; // It should not reach here.
            }
            NVIC_EnableIRQ(COMP_LPCOMP_IRQn);
            break; // Only one analog pin can be configured as wakeup source at a time. Stop traversing the list.
        }
        source = source->next;
    }
}

static uint32_t configUsartWakeupSource(const hal_wakeup_source_base_t* wakeupSources) {
    uint32_t intFlags = NRF_UARTE0->INTENSET;
    auto source = wakeupSources;
    while (source) {
        if (source->type == HAL_WAKEUP_SOURCE_TYPE_USART) {
            nrf_uarte_int_disable(NRF_UARTE0, NRF_UARTE_INT_ERROR_MASK |
                      NRF_UARTE_INT_RXSTARTED_MASK |
                      NRF_UARTE_INT_RXTO_MASK |
                      NRF_UARTE_INT_ENDRX_MASK |
                      NRF_UARTE_INT_TXSTARTED_MASK |
                      NRF_UARTE_INT_TXDRDY_MASK |
                      NRF_UARTE_INT_TXSTOPPED_MASK |
                      NRF_UARTE_INT_ENDTX_MASK);
            NVIC_ClearPendingIRQ(UARTE0_UART0_IRQn);
            /* It potentially has received data already leaving the thread waiting on the data still in a blocked state. */
            // nrf_uarte_int_disable(NRF_UARTE0, NRF_UARTE_INT_RXDRDY_MASK);
            // nrf_uarte_event_clear(NRF_UARTE0, NRF_UARTE_EVENT_RXDRDY);
            if (!nrf_uarte_int_enable_check(NRF_UARTE0, NRF_UARTE_INT_RXDRDY_MASK)) {
                // We're safe to clear the event if application didn't enable the RXDRDY interrupt
                nrf_uarte_event_clear(NRF_UARTE0, NRF_UARTE_EVENT_RXDRDY);
                nrf_uarte_int_enable(NRF_UARTE0, NRF_UARTE_INT_RXDRDY_MASK);
            }
            NVIC_EnableIRQ(UARTE0_UART0_IRQn);
            break; // There is only one USART available for user. Stop traversing the list.
        }
        source = source->next;
    }
    return intFlags;
}

static bool configNetworkWakeupSource(const hal_wakeup_source_base_t* wakeupSources) {
    bool ret = false;
    auto source = wakeupSources;
    while (source) {
        if (source->type == HAL_WAKEUP_SOURCE_TYPE_NETWORK) {
            auto network = reinterpret_cast<const hal_wakeup_source_network_t*>(source);
            if (!(network->flags & HAL_SLEEP_NETWORK_FLAG_INACTIVE_STANDBY)) {
                /* It potentially has received data already leaving the thread waiting on the data still in a blocked state. */
                // nrf_uarte_int_disable(NRF_UARTE1, NRF_UARTE_INT_RXDRDY_MASK);
                // nrf_uarte_event_clear(NRF_UARTE1, NRF_UARTE_EVENT_RXDRDY);
                if (!nrf_uarte_int_enable_check(NRF_UARTE1, NRF_UARTE_INT_RXDRDY_MASK)) {
                    // We're safe to clear the event if application didn't enable the RXDRDY interrupt
                    nrf_uarte_event_clear(NRF_UARTE1, NRF_UARTE_EVENT_RXDRDY);
                    nrf_uarte_int_enable(NRF_UARTE1, NRF_UARTE_INT_RXDRDY_MASK);
                } else {
                    ret = true;
                }
                NVIC_EnableIRQ(UARTE1_IRQn);
                break; // There is only one USART available for network modem. Stop traversing the list.
            }
        }
        source = source->next;
    }
    return ret;
}

static bool isWokenUpByGpio(const hal_wakeup_source_gpio_t* gpioWakeup) {
    if (!NVIC_GetPendingIRQ(GPIOTE_IRQn)) {
        return false;
    }
    hal_pin_info_t* halPinMap = hal_pin_map();
    hal_pin_t wakeupPin;

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    if (halPinMap[gpioWakeup->pin].type == HAL_PIN_TYPE_MCU) {
#endif
        wakeupPin = gpioWakeup->pin;
#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    }
#if HAL_PLATFORM_MCP23S17
    else if (halPinMap[gpioWakeup->pin].type == HAL_PIN_TYPE_IO_EXPANDER) {
        wakeupPin = IOE_INT;
    }
#endif
    else {
        return false;
    }
#endif

    if (nrf_gpiote_event_is_set(NRF_GPIOTE_EVENTS_PORT)) {
        // PORT event
        uint32_t nrfPin = NRF_GPIO_PIN_MAP(halPinMap[wakeupPin].gpio_port, halPinMap[wakeupPin].gpio_pin);
        nrf_gpio_pin_sense_t sense = nrf_gpio_pin_sense_get(nrfPin);
        if (sense != NRF_GPIO_PIN_NOSENSE) {
            uint32_t state = nrf_gpio_pin_read(nrfPin);
            if ((state && sense == NRF_GPIO_PIN_SENSE_HIGH) || (!state && sense == NRF_GPIO_PIN_SENSE_LOW)) {
                return true;
            }
        }
    } else {
        // Check IN events
        for (unsigned i = 0; i < GPIOTE_CH_NUM; ++i) {
            if (NRF_GPIOTE->EVENTS_IN[i] && nrf_gpiote_int_is_enabled(NRF_GPIOTE_INT_IN0_MASK << i)) {
                hal_pin_t pin = NRF_PIN_LOOKUP_TABLE[nrf_gpiote_event_pin_get(i)];
                if (pin == wakeupPin) {
                    return true;
                }
            }
        }
    }

    return false;
}

static bool isWokenUpByRtc() {
    return NVIC_GetPendingIRQ(RTC2_IRQn);
}

static bool isWokenUpByUsart() {
    return NVIC_GetPendingIRQ(UARTE0_UART0_IRQn);
}

static bool isWokenUpByBle() {
    return NVIC_GetPendingIRQ(SD_EVT_IRQn);
}

static bool isWokenUpByLpcomp() {
    return NVIC_GetPendingIRQ(COMP_LPCOMP_IRQn);
}

static bool isWokenUpByNetwork(const hal_wakeup_source_network_t* networkWakeup, const uint32_t ncpId) {
// TODO: More than one network interface are supported on platform.
    if (networkWakeup->flags & HAL_SLEEP_NETWORK_FLAG_INACTIVE_STANDBY) {
        return false;
    }
#if HAL_PLATFORM_CELLULAR
    if (networkWakeup->index == NETWORK_INTERFACE_CELLULAR && NVIC_GetPendingIRQ(UARTE1_IRQn)) {
        // XXX: u-blox issue where RXD pin toggles to HI-Z for ~10us about 1ms after CTS goes HIGH
        //      while modem is in UPSV=1 mode and in a low power state.  We see a low pulse due to
        //      10k pull-down on RXD. These will occur every 1.28s.  If we wake up via R510 cellular
        //      network activity, and CTS is HIGH, go back to sleep.
        // Is CTS pin HIGH? (clear pending interrupt and return false)
        if (ncpId == PLATFORM_NCP_SARA_R510) {
            hal_pin_info_t* halPinMap = hal_pin_map();
            uint32_t cts1Pin = NRF_GPIO_PIN_MAP(halPinMap[CTS1].gpio_port, halPinMap[CTS1].gpio_pin);
            uint32_t cts1State = nrf_gpio_pin_read(cts1Pin);
            if (cts1State) {
                nrf_uarte_event_clear(NRF_UARTE1, NRF_UARTE_EVENT_RXDRDY);
                nrf_uarte_int_enable(NRF_UARTE1, NRF_UARTE_INT_RXDRDY_MASK);
                NVIC_ClearPendingIRQ(UARTE1_IRQn);
                return false;
            }
        }
        return true;
    }
#endif
#if HAL_PLATFORM_WIFI
    if (networkWakeup->index == NETWORK_INTERFACE_WIFI_STA && NVIC_GetPendingIRQ(UARTE1_IRQn)) {
        return true;
    }
#endif
    return false;
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

static int validateUsartWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_usart_t* usart) {
    if (!hal_usart_is_enabled(usart->serial)) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (mode == HAL_SLEEP_MODE_HIBERNATE) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    return SYSTEM_ERROR_NONE;
}

static int validateNetworkWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_network_t* network) {
    if (!(network->flags & HAL_SLEEP_NETWORK_FLAG_INACTIVE_STANDBY)) {
#if HAL_PLATFORM_CELLULAR
        if (network->index == NETWORK_INTERFACE_CELLULAR &&
                !hal_usart_is_enabled(HAL_PLATFORM_CELLULAR_SERIAL)) {
#elif HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_NCP_SDIO
        if (network->index == NETWORK_INTERFACE_WIFI_STA &&
                !hal_usart_is_enabled(HAL_PLATFORM_WIFI_SERIAL)) {
#endif
            return SYSTEM_ERROR_INVALID_STATE;
        }
        if (mode == HAL_SLEEP_MODE_HIBERNATE) {
            return SYSTEM_ERROR_NOT_SUPPORTED;
        }
    }
    return SYSTEM_ERROR_NONE;
}

static int validateBleWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_base_t* base) {
    if (mode == HAL_SLEEP_MODE_HIBERNATE) {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
    return SYSTEM_ERROR_NONE;
}

static int validateLpcompWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_lpcomp_t* lpcomp) {
    if (hal_pin_validate_function(lpcomp->pin, PF_ADC) != PF_ADC) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    if (lpcomp->trig > HAL_SLEEP_LPCOMP_CROSS) {
        return SYSTEM_ERROR_INVALID_ARGUMENT;
    }
    return SYSTEM_ERROR_NONE;
}

static int validateWakeupSource(hal_sleep_mode_t mode, const hal_wakeup_source_base_t* base) {
    if (base->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
        return validateGpioWakeupSource(mode, reinterpret_cast<const hal_wakeup_source_gpio_t*>(base));
    } else if (base->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
        return validateRtcWakeupSource(mode, reinterpret_cast<const hal_wakeup_source_rtc_t*>(base));
    } else if (base->type == HAL_WAKEUP_SOURCE_TYPE_USART) {
        return validateUsartWakeupSource(mode, reinterpret_cast<const hal_wakeup_source_usart_t*>(base));
    } else if (base->type == HAL_WAKEUP_SOURCE_TYPE_NETWORK) {
        return validateNetworkWakeupSource(mode, reinterpret_cast<const hal_wakeup_source_network_t*>(base));
    } else if (base->type == HAL_WAKEUP_SOURCE_TYPE_BLE) {
        return validateBleWakeupSource(mode, base);
    } else if (base->type == HAL_WAKEUP_SOURCE_TYPE_LPCOMP) {
        return validateLpcompWakeupSource(mode, reinterpret_cast<const hal_wakeup_source_lpcomp_t*>(base));
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

static int enterStopBasedSleep(const hal_sleep_config_t* config, hal_wakeup_source_base_t** wakeupReason) {
    int ret = SYSTEM_ERROR_NONE;
    uint32_t ncpId = platform_primary_ncp_identifier(); // save before external flash is put to sleep

    // Detach USB
    HAL_USB_Detach();

    // Flush all USARTs
    // FIXME: no lock
    for (int usart = 0; usart < HAL_PLATFORM_USART_NUM; usart++) {
        if (hal_usart_is_enabled(static_cast<hal_usart_interface_t>(usart))) {
            hal_usart_flush(static_cast<hal_usart_interface_t>(usart));
        }
    }

    /* We neeed to configure the IO expander interrupt before disabling SPI interface. */
#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
#if HAL_PLATFORM_MCP23S17
    Mcp23s17::getInstance().interruptsSuspend();
    configGpioWakeupSourceExt(config->wakeup_sources, config->mode);
#endif
#endif

    // We need to do this before disabling systick/interrupts, otherwise
    // there is a high chance of a deadlock
    if (config->mode == HAL_SLEEP_MODE_ULTRA_LOW_POWER) {
        // Do not suspend the usarts those are featured as wakeup source.
        Vector<hal_usart_interface_t> skipUsarts;
        for (const hal_wakeup_source_base_t* src = config->wakeup_sources; src != nullptr; src = src->next) {
            if (src->type == HAL_WAKEUP_SOURCE_TYPE_NETWORK) {
                const auto networkSource = reinterpret_cast<const hal_wakeup_source_network_t*>(src);
#if HAL_PLATFORM_CELLULAR
                if (networkSource->index == NETWORK_INTERFACE_CELLULAR) {
                    skipUsarts.append(HAL_PLATFORM_CELLULAR_SERIAL);
                }
#endif
#if HAL_PLATFORM_WIFI && !HAL_PLATFORM_WIFI_NCP_SDIO
                if (networkSource->index == NETWORK_INTERFACE_WIFI_STA) {
                    skipUsarts.append(HAL_PLATFORM_WIFI_SERIAL);
                }
#endif
            } else if (src->type == HAL_WAKEUP_SOURCE_TYPE_USART) {
                const auto usartSource = reinterpret_cast<const hal_wakeup_source_usart_t*>(src);
                skipUsarts.append(usartSource->serial);
            }
        }

        for (int usart = 0; usart < HAL_PLATFORM_USART_NUM; usart++) {
            // FIXME: no lock
            // FIXME: we cannot reliably put NCP UART into sleep now without any thread-safety issues
            // We need to properly signal NCP client before going into sleep that it cannnot use
            // USART for a while.
            if (!skipUsarts.contains(static_cast<hal_usart_interface_t>(usart))) {
                hal_usart_sleep(static_cast<hal_usart_interface_t>(usart), true, nullptr);
            }
        }
        // Suspend SPIs
        for (int spi = 0; spi < HAL_PLATFORM_SPI_NUM; spi++) {
            hal_spi_acquire(static_cast<hal_spi_interface_t>(spi), nullptr);
            hal_spi_sleep(static_cast<hal_spi_interface_t>(spi), true, nullptr);
        }
        // Suspend I2Cs
        for (int i2c = 0; i2c < HAL_PLATFORM_I2C_NUM; i2c++) {
            hal_i2c_lock(static_cast<hal_i2c_interface_t>(i2c), nullptr);
            hal_i2c_sleep(static_cast<hal_i2c_interface_t>(i2c), true, nullptr);
        }
    }

    // Make sure we acquire exflash lock BEFORE going into a critical section
    hal_exflash_lock();

    // BLE events should be dealt before disabling thread scheduling.
    auto bleWakeupSource = findWakeupSource(config->wakeup_sources, HAL_WAKEUP_SOURCE_TYPE_BLE);
    bool advertising = hal_ble_gap_is_advertising(nullptr) ||
                       hal_ble_gap_is_connecting(nullptr, nullptr) ||
                       hal_ble_gap_is_connected(nullptr, nullptr);
    if (!bleWakeupSource) {
        // Make sure we acquire BLE lock BEFORE going into a critical section
        hal_ble_lock(nullptr);
        hal_ble_stack_deinit(nullptr);
        disableRadioAntenna();
    }

    // Disable thread scheduling
    os_thread_scheduling(false, nullptr);

    if (config->mode == HAL_SLEEP_MODE_ULTRA_LOW_POWER) {
        // Suspend ADC module
        hal_adc_sleep(true, nullptr);
    }

    // This will disable all but SoftDevice interrupts (by modifying NVIC->ICER)
    uint8_t st = 0;
    sd_nvic_critical_region_enter(&st);

    // Disable SysTick
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

    // Put external flash into sleep and disable QSPI peripheral
    hal_exflash_sleep(true, nullptr);

    // Suspend PWM modules after disabling systick, as RGB pins are managed there
    hal_pwm_sleep(true, nullptr);

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

    // WARNING: This is going to fix the issue that when both .ble() and .duration()
    // are specified as wakeup sources, device will run into hardfault if the following
    // check comes after __disable_irq(), such as in configRtcWakeupSource() previously.
    nrf_rtc_event_enable(NRF_RTC2, RTC_EVTEN_TICK_Msk);
    // Make sure that RTC is ticking
    // See 'TASK and EVENT jitter/delay'
    // http://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.nrf52840.ps%2Frtc.html
    while (!nrf_rtc_event_pending(NRF_RTC2, NRF_RTC_EVENT_TICK));
    nrf_rtc_event_disable(NRF_RTC2, RTC_EVTEN_TICK_Msk);
    nrf_rtc_event_clear(NRF_RTC2, NRF_RTC_EVENT_TICK);

    // Workaround for FPU anomaly
    fpu_sleep_prepare();

    /* Do not allow any ISR to be executed starting from here. */
    __disable_irq();

    // Suspend all GPIOTE interrupts
    hal_interrupt_suspend();

    configGpioWakeupSource(config->wakeup_sources);
    configRtcWakeupSource(config->wakeup_sources);
    configLpcompWakeupSource(config->wakeup_sources);
    uint32_t intFlags = configUsartWakeupSource(config->wakeup_sources);
    bool networkRxdRdy = configNetworkWakeupSource(config->wakeup_sources);

    // Masks all interrupts lower than softdevice. This allows us to be woken ONLY by softdevice
    // or GPIOTE and RTC.
    // IMPORTANT: No SoftDevice API calls are allowed until HAL_enable_irq()
    // TODO: we have bumped the SD_EVT_IRQn priority, so we can probably always call HAL_disable_irq() here.
    int hst = 0;
    if (!bleWakeupSource) {
        hst = HAL_disable_irq();
    }

    __DSB();
    __ISB();

    hal_wakeup_source_type_t wakeupSourceType = HAL_WAKEUP_SOURCE_TYPE_UNKNOWN;
    hal_pin_t wakeupPin = PIN_INVALID;
    network_interface_index netif = NETWORK_INTERFACE_ALL;

    bool exitSleepMode = false;
    while (true) {
        // Mask interrupts completely
        __disable_irq();

        // Bump the priority
        WakeupSourcePriorityCache priorityCache = {};
        bumpWakeupSourcesPriority(config->wakeup_sources, &priorityCache, 0);

        __DSB();
        __ISB();

        // Go to sleep
        __WFI();

        // Figure out the wakeup source
        auto wakeupSource = config->wakeup_sources;
        while (wakeupSource) {
            if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
                auto gpioWakeup = reinterpret_cast<hal_wakeup_source_gpio_t*>(wakeupSource);
                if (isWokenUpByGpio(gpioWakeup)) {
                    wakeupSourceType = HAL_WAKEUP_SOURCE_TYPE_GPIO;
                    wakeupPin = gpioWakeup->pin;
                    exitSleepMode = true;
                    // Only if we've figured out the wakeup pin, then we stop traversing the wakeup sources list.
                    break;
                }
            } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_RTC && isWokenUpByRtc()) {
                wakeupSourceType = HAL_WAKEUP_SOURCE_TYPE_RTC;
                exitSleepMode = true;
                break; // Stop traversing the wakeup sources list.
            } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_BLE && isWokenUpByBle()) {
                wakeupSourceType = HAL_WAKEUP_SOURCE_TYPE_BLE;
                exitSleepMode = true;
                break; // Stop traversing the wakeup sources list.
            } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_LPCOMP && isWokenUpByLpcomp()) {
                auto lpcompWakeup = reinterpret_cast<hal_wakeup_source_lpcomp_t*>(wakeupSource);
                wakeupSourceType = HAL_WAKEUP_SOURCE_TYPE_LPCOMP;
                wakeupPin = lpcompWakeup->pin;
                exitSleepMode = true;
                break; // Stop traversing the wakeup sources list.
            } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_USART && isWokenUpByUsart()) {
                wakeupSourceType = HAL_WAKEUP_SOURCE_TYPE_USART;
                exitSleepMode = true;
                break; // Stop traversing the wakeup sources list.
            } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_NETWORK) {
                auto networkWakeup = reinterpret_cast<hal_wakeup_source_network_t*>(wakeupSource);
                if (isWokenUpByNetwork(networkWakeup, ncpId)) {
                    wakeupSourceType = HAL_WAKEUP_SOURCE_TYPE_NETWORK;
                    netif = networkWakeup->index;
                    exitSleepMode = true;
                    break; // Stop traversing the wakeup sources list.
                }
            }
            wakeupSource = wakeupSource->next;
        }

        // Unbump the priority before __enable_irq().
        unbumpWakeupSourcesPriority(config->wakeup_sources, &priorityCache);

        // Unmask interrupts so that SoftDevice can still process BLE events.
        // we are still under the effect of HAL_disable_irq that masked all but SoftDevice interrupts using BASEPRI
        __enable_irq();

        /*
         * Wake-up on BLE events relies mainly on SD_EVT_IRQn interrupt, which will be disabled through NVIC (not masked)
         * when entering sd_nvic_critical_region_enter(). Normally on BLE activity we would be woken up by other high-priority
         * SoftDevice interrupts (e.g. timers, radio events etc), however SD_EVT_IRQn would be set as pending only after we
         * unmask high-priority SoftDevice interrupts (__enable_irq()) and after the appropriate SoftDevice interrupt handlers
         * finish executing. We would then immediately go back to sleep and wouldn't notice this change in SD_EVT_IRQn state.
         * As a workaround, for the duration of the sleep, we'll be bumping the priority of SD_EVT_IRQn, so that it can wake
         * us up from stop mode (WFI) as well.
         */

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

    auto source = config->wakeup_sources;
    while (source) {
        if (source->type == HAL_WAKEUP_SOURCE_TYPE_USART) {
            nrf_uarte_int_enable(NRF_UARTE0, intFlags);
            break; // There is only one USART available for user. Stop traversing the list.
        }
        source = source->next;
    }
    if (networkRxdRdy) {
        nrf_uarte_int_enable(NRF_UARTE1, NRF_UARTE_INT_RXDRDY_MASK);
    }

    nrf_lpcomp_disable();

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
    hal_interrupt_restore();

    // Re-initialize external flash
    // If we fail to re-initialize it, there is no point in continuing to wake-up
    // as the system will anyway not be functional
    int exflashResume = hal_exflash_sleep(false, nullptr);
    if (exflashResume == SYSTEM_ERROR_INVALID_STATE) {
        // We've previously failed to correctly put the QSPI flash into sleep
        // Attempt to initialize
        // Just in case uninit
        hal_exflash_uninit();
        exflashResume = hal_exflash_init();
    }
    SPARK_ASSERT(exflashResume == 0);

    // Restore PWM state
    hal_pwm_sleep(false, nullptr);

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

    if (!bleWakeupSource) {
        hal_ble_stack_init(nullptr);
        if (advertising) {
            hal_ble_gap_start_advertising(nullptr);
        }
        hal_ble_unlock(nullptr);
        enableRadioAntenna();
    }

    if (config->mode == HAL_SLEEP_MODE_ULTRA_LOW_POWER) {
        // Re-enable ADC module
        // FIXME: no lock
        hal_adc_sleep(false, nullptr);

        // Restore I2Cs
        for (int i2c = 0; i2c < HAL_PLATFORM_I2C_NUM; i2c++) {
            hal_i2c_sleep(static_cast<hal_i2c_interface_t>(i2c), false, nullptr);
            hal_i2c_unlock(static_cast<hal_i2c_interface_t>(i2c), nullptr);
        }
        // Restore SPIs
        for (int spi = 0; spi < HAL_PLATFORM_SPI_NUM; spi++) {
            hal_spi_sleep(static_cast<hal_spi_interface_t>(spi), false, nullptr);
            hal_spi_release(static_cast<hal_spi_interface_t>(spi), nullptr);
        }
        // Restore USARTs
        for (int usart = 0; usart < HAL_PLATFORM_USART_NUM; usart++) {
            // FIXME: no lock
            hal_usart_sleep(static_cast<hal_usart_interface_t>(usart), false, nullptr);
        }
    }

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
#if HAL_PLATFORM_MCP23S17
    uint8_t intStatus[2];
    Mcp23s17::getInstance().interruptsRestore(intStatus);
    hal_pin_info_t* halPinMap = hal_pin_map();
    if (halPinMap[wakeupPin].type == HAL_PIN_TYPE_IO_EXPANDER) {
        // We need to identify the exact wakeup pin attached to the IO expander.
        source = config->wakeup_sources;
        while (source) {
            if (source->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
                auto gpioWakeup = reinterpret_cast<hal_wakeup_source_gpio_t*>(source);
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
#endif

    // Enable thread scheduling
    os_thread_scheduling(true, nullptr);

    if (wakeupReason) {
        if (wakeupSourceType == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            ret = constructGpioWakeupReason(wakeupReason, wakeupPin);
        } else if (wakeupSourceType == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            ret = constructRtcWakeupReason(wakeupReason);
        } else if (wakeupSourceType == HAL_WAKEUP_SOURCE_TYPE_USART) {
            ret = constructUsartWakeupReason(wakeupReason);
        } else if (wakeupSourceType == HAL_WAKEUP_SOURCE_TYPE_BLE) {
            ret = constructBleWakeupReason(wakeupReason);
        } else if (wakeupSourceType == HAL_WAKEUP_SOURCE_TYPE_LPCOMP) {
            ret = constructLpcompWakeupReason(wakeupReason, wakeupPin);
        } else if (wakeupSourceType == HAL_WAKEUP_SOURCE_TYPE_NETWORK) {
            ret = constructNetworkWakeupReason(wakeupReason, netif);
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

    // Suspend USARTs
    for (int usart = 0; usart < HAL_PLATFORM_USART_NUM; usart++) {
        hal_usart_sleep(static_cast<hal_usart_interface_t>(usart), true, nullptr);
    }
    // Suspend SPIs
    for (int spi = 0; spi < HAL_PLATFORM_SPI_NUM; spi++) {
        hal_spi_sleep(static_cast<hal_spi_interface_t>(spi), true, nullptr);
    }
    // Suspend I2Cs
    for (int i2c = 0; i2c < HAL_PLATFORM_I2C_NUM; i2c++) {
        hal_i2c_sleep(static_cast<hal_i2c_interface_t>(i2c), true, nullptr);
    }
    // Suspend PWM modules
    hal_pwm_sleep(true, nullptr);
    // Suspend ADC module
    hal_adc_sleep(true, nullptr);

    // This will disable all but SoftDevice interrupts (by modifying NVIC->ICER)
    uint8_t st = 0;
    sd_nvic_critical_region_enter(&st);

    // Disable SysTick
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

    // Put external flash into sleep mode and disable QSPI peripheral
    hal_exflash_sleep(true, nullptr);

    // Uninit GPIOTE
    nrfx_gpiote_uninit();

    // Disable low power comparator
    nrf_lpcomp_disable();

    // Deconfigure any possible SENSE configuration
    hal_interrupt_suspend();

    // Disable GPIOTE PORT interrupts
    nrf_gpiote_int_disable(GPIOTE_INTENSET_PORT_Msk);

    // Clear any GPIOTE events
    nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_PORT);

    disableRadioAntenna();

    auto wakeupSource = config->wakeup_sources;
    while (wakeupSource) {
        if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_GPIO) {
            hal_pin_info_t* halPinMap = hal_pin_map();
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
        } else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_LPCOMP) {
            configLpcompWakeupSource(wakeupSource);
        }
#if HAL_PLATFORM_EXTERNAL_RTC
        else if (wakeupSource->type == HAL_WAKEUP_SOURCE_TYPE_RTC) {
            hal_pin_info_t* halPinMap = hal_pin_map();
            uint32_t nrfPin = NRF_GPIO_PIN_MAP(halPinMap[RTC_INT].gpio_port, halPinMap[RTC_INT].gpio_pin);
            nrf_gpio_cfg_sense_input(nrfPin, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
        }
 #endif
        wakeupSource = wakeupSource->next;
    }

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
    // At least one wakeup source should be configured for stop and ultra-low power mode.
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
