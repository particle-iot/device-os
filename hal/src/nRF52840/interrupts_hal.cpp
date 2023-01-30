/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#include "interrupts_hal.h"
#include "interrupts_irq.h"
#include "nrfx_gpiote.h"
#include "pinmap_impl.h"
#include "logging.h"
#include "nrf_nvic.h"
#include "gpio_hal.h"
#include "system_error.h"
#include "check.h"

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
#if HAL_PLATFORM_MCP23S17
#include "mcp23s17.h"
#endif
using namespace particle;
#endif

// 8 high accuracy GPIOTE channels
#define GPIOTE_CHANNEL_NUM              8
// 8 low accuracy port event channels
#define PORT_EVENT_CHANNEL_NUM          NRFX_GPIOTE_CONFIG_NUM_OF_LOW_POWER_EVENTS
// Prioritize the use of high accuracy channels automatically
#define EXTI_CHANNEL_NUM                (GPIOTE_CHANNEL_NUM + PORT_EVENT_CHANNEL_NUM)

static struct {
    uint8_t                 pin;
    HAL_InterruptCallback   interrupt_callback;
} m_exti_channels[EXTI_CHANNEL_NUM] = {{0, {nullptr, nullptr}}};

struct hal_interrupts_suspend_data_t {
    uint32_t config[GPIOTE_CH_NUM];
    uint32_t intenset;
    uint32_t pin_cnf[NUMBER_OF_PINS];
};

static hal_interrupts_suspend_data_t s_suspend_data = {};

extern uintptr_t link_interrupt_vectors_location[];
extern uintptr_t link_ram_interrupt_vectors_location[];
extern uintptr_t link_ram_interrupt_vectors_location_end;

static void gpiote_interrupt_handler(nrfx_gpiote_pin_t nrf_pin, nrf_gpiote_polarity_t action) {
    uint8_t pin = NRF_PIN_LOOKUP_TABLE[nrf_pin];
    if (pin == PIN_INVALID) {
        // Ignore
        return;
    }

    hal_pin_info_t* PIN_MAP = hal_pin_map();

    hal_interrupt_handler_t user_isr_handle = m_exti_channels[PIN_MAP[pin].exti_channel].interrupt_callback.handler;
    void *data = m_exti_channels[PIN_MAP[pin].exti_channel].interrupt_callback.data;
    if (user_isr_handle) {
        user_isr_handle(data);
    }
}

void hal_interrupt_init(void) {
    for (int i = 0; i < EXTI_CHANNEL_NUM; i++) {
        m_exti_channels[i].pin = PIN_INVALID;
    }
    nrfx_gpiote_init();
}

void hal_interrupt_uninit(void) {
    nrfx_gpiote_uninit();
}

static nrfx_gpiote_in_config_t get_gpiote_config(uint16_t pin, InterruptMode mode, bool hi_accu) {
    nrfx_gpiote_in_config_t in_config = {
        .sense = NRF_GPIOTE_POLARITY_TOGGLE,
        .pull = NRF_GPIO_PIN_NOPULL,
        .is_watcher = false,
        .hi_accuracy = hi_accu,
        .skip_gpio_setup = true,
    };

    switch (mode) {
        case CHANGE: {
            in_config.sense = NRF_GPIOTE_POLARITY_TOGGLE; 
            break;
        }
        case RISING: {
            in_config.sense = NRF_GPIOTE_POLARITY_LOTOHI; 
            break;
        }
        case FALLING: {
            in_config.sense = NRF_GPIOTE_POLARITY_HITOLO; 
            break;
        }
    }

    return in_config;
}

int hal_interrupt_attach(uint16_t pin, hal_interrupt_handler_t handler, void* data, InterruptMode mode, hal_interrupt_extra_configuration_t* config) {
    hal_pin_info_t* PIN_MAP = hal_pin_map();

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    if (PIN_MAP[pin].type == HAL_PIN_TYPE_MCU) {
#endif
        uint8_t nrf_pin = NRF_GPIO_PIN_MAP(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin);

        nrfx_gpiote_in_config_t in_config;

        in_config = get_gpiote_config(pin, mode, true);
        uint32_t err_code = nrfx_gpiote_in_init(nrf_pin, &in_config, gpiote_interrupt_handler);
        if (err_code == NRFX_ERROR_NO_MEM) {
            // High accuracy channels have been used up, use low accuracy channels
            in_config = get_gpiote_config(pin, mode, false);
            err_code = nrfx_gpiote_in_init(nrf_pin, &in_config, gpiote_interrupt_handler);
        }

        if (err_code == NRF_SUCCESS) {
            // Add interrupt handler
            for (int i = 0; i < EXTI_CHANNEL_NUM; i++) {
                if (m_exti_channels[i].pin == PIN_INVALID) {
                    m_exti_channels[i].pin = pin;
                    m_exti_channels[i].interrupt_callback.handler = handler;
                    m_exti_channels[i].interrupt_callback.data = data;
                    PIN_MAP[pin].exti_channel = i;

                    break;
                }
            }
        } else if (err_code == NRFX_ERROR_INVALID_STATE) {
            if (!config || (config && !(config->keepHandler))) {
                // This pin is used by GPIOTE, Change interrupt handler if necessary
                for (int i = 0; i < EXTI_CHANNEL_NUM; i++) {
                    if (m_exti_channels[i].pin == pin) {
                        m_exti_channels[i].interrupt_callback.handler = handler;
                        m_exti_channels[i].interrupt_callback.data = data;

                        break;
                    }
                }
            }
        } else if (err_code == NRFX_ERROR_NO_MEM) {
            // All channels have been used up
            return SYSTEM_ERROR_NO_MEMORY;
        }

        nrfx_gpiote_in_event_enable(nrf_pin, true);
        return SYSTEM_ERROR_NONE;
#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    }
#if HAL_PLATFORM_MCP23S17
    else if (PIN_MAP[pin].type == HAL_PIN_TYPE_IO_EXPANDER) {
        return Mcp23s17::getInstance().attachPinInterrupt(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin, mode, static_cast<particle::Mcp23s17InterruptCallback>(handler), data);
    }
#endif
    else {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
#endif
}

int hal_interrupt_detach(uint16_t pin) {
    return hal_interrupt_detach_ext(pin, 0, NULL);
}

int hal_interrupt_detach_ext(uint16_t pin, uint8_t keepHandler, void* reserved) {
    hal_pin_info_t* PIN_MAP = hal_pin_map();

#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    if (PIN_MAP[pin].type == HAL_PIN_TYPE_MCU) {
#endif
        if (PIN_MAP[pin].exti_channel == EXTI_CHANNEL_NONE) {
            return SYSTEM_ERROR_INVALID_STATE;
        }

        uint8_t nrf_pin = NRF_GPIO_PIN_MAP(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin);
        nrfx_gpiote_in_event_disable(nrf_pin);
        if (keepHandler) {
            // Only disable events for this pin and keep handlers
            return SYSTEM_ERROR_NONE;
        }
        nrfx_gpiote_in_uninit(nrf_pin);

        m_exti_channels[PIN_MAP[pin].exti_channel].pin = PIN_INVALID;
        m_exti_channels[PIN_MAP[pin].exti_channel].interrupt_callback.handler = NULL;
        m_exti_channels[PIN_MAP[pin].exti_channel].interrupt_callback.data = NULL;
        PIN_MAP[pin].exti_channel = EXTI_CHANNEL_NONE;
        hal_pin_set_function(pin, PF_NONE);
        return SYSTEM_ERROR_NONE;
#if HAL_PLATFORM_IO_EXTENSION && MODULE_FUNCTION != MOD_FUNC_BOOTLOADER
    }
#if HAL_PLATFORM_MCP23S17
    else if (PIN_MAP[pin].type == HAL_PIN_TYPE_IO_EXPANDER) {
        return Mcp23s17::getInstance().detachPinInterrupt(PIN_MAP[pin].gpio_port, PIN_MAP[pin].gpio_pin);
    } 
#endif
    else {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }
#endif
}

void hal_interrupt_enable_all(void) {
    sd_nvic_EnableIRQ(GPIOTE_IRQn);
}

void hal_interrupt_disable_all(void) {
    sd_nvic_DisableIRQ(GPIOTE_IRQn);
}

void hal_interrupt_suspend(void)
{
    // Save NRF_GPIOTE->CONFIG[], NRF_GPIOTE->INTENSET
    for (unsigned i = 0; i < GPIOTE_CH_NUM; ++i) {
        // NOTE: we are not modifying CONFIG here
        s_suspend_data.config[i] = NRF_GPIOTE->CONFIG[i];
    }
    s_suspend_data.intenset = NRF_GPIOTE->INTENSET;

    // Save pin configuration (including sense)
    for (uint32_t i = 0; i < NUMBER_OF_PINS; i++) {
        auto nrf_pin = i;
        auto port = nrf_gpio_pin_port_decode(&nrf_pin);
        s_suspend_data.pin_cnf[i] = port->PIN_CNF[nrf_pin];
        // NOTE: we are resetting sense configuration
        nrf_gpio_cfg_sense_set(i, NRF_GPIO_PIN_NOSENSE);
    }
}

void hal_interrupt_restore(void)
{
    // Restore pin configuration for all the pins
    for (uint32_t i = 0; i < NUMBER_OF_PINS; i++) {
        auto nrf_pin = i;
        auto port = nrf_gpio_pin_port_decode(&nrf_pin);
        port->PIN_CNF[nrf_pin] = s_suspend_data.pin_cnf[i];
    }

    // Restore NRF_GPIOTE->CONFIG[], NRF_GPIOTE->INTENSET
    for (unsigned i = 0; i < GPIOTE_CH_NUM; ++i) {
        NRF_GPIOTE->CONFIG[i] = s_suspend_data.config[i];
    }
    uint32_t intenset = NRF_GPIOTE->INTENSET;
    nrf_gpiote_int_disable(intenset ^ s_suspend_data.intenset);
    nrf_gpiote_int_enable(s_suspend_data.intenset);
}

int hal_interrupt_set_direct_handler(IRQn_Type irqn, hal_interrupt_direct_handler_t handler, uint32_t flags, void* reserved) {
    if (irqn < NonMaskableInt_IRQn || irqn > SPIM3_IRQn) {
        return 1;
    }

    int32_t state = HAL_disable_irq();
    volatile uint32_t* isrs = (volatile uint32_t*)&link_ram_interrupt_vectors_location;

    if (handler == NULL && (flags & HAL_INTERRUPT_DIRECT_FLAG_RESTORE)) {
        // Restore
        HAL_Core_Restore_Interrupt(irqn);
    } else {
        isrs[IRQN_TO_IDX(irqn)] = (uint32_t)handler;
    }

    if (flags & HAL_INTERRUPT_DIRECT_FLAG_DISABLE) {
        // Disable
        sd_nvic_DisableIRQ(irqn);
    } else if (flags & HAL_INTERRUPT_DIRECT_FLAG_ENABLE) {
        sd_nvic_EnableIRQ(irqn);
    }

    HAL_enable_irq(state);

    return 0;
}
