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

#include "button_hal.h"
#include <nrf_rtc.h>
#include <nrf_nvic.h>
#include "platform_config.h"
#include "interrupts_hal.h"

button_config_t HAL_Buttons[] = {
    {
        .active         = false,
        .pin            = BUTTON1_GPIO_PIN,
        .mode           = BUTTON1_GPIO_MODE,
        .pupd           = BUTTON1_GPIO_PUPD,
        .debounce_time  = 0,

        .event_in       = BUTTON1_GPIOTE_EVENT_IN,
        .event_channel  = BUTTON1_GPIOTE_EVENT_CHANNEL,
        .int_mask       = BUTTON1_GPIOTE_INT_MASK,
        .interrupt_mode = BUTTON1_GPIOTE_INTERRUPT_MODE,
        .nvic_irqn      = BUTTON1_GPIOTE_IRQn,
        .nvic_irq_prio  = BUTTON1_GPIOTE_IRQ_PRIORITY
    },
    {
        .active         = false,
        .debounce_time  = 0
    }
};

static void rtc_timer_init(void)
{
    nrf_rtc_prescaler_set(NRF_RTC1, RTC_FREQ_TO_PRESCALER(UI_TIMER_FREQUENCY));
    /* NOTE: the second argument is an event (nrf_rtc_event_t) */
    nrf_rtc_event_clear(NRF_RTC1, NRF_RTC_EVENT_TICK);
    /* NOTE: the second argument is a mask */
    nrf_rtc_event_enable(NRF_RTC1, RTC_EVTEN_TICK_Msk);
    nrf_rtc_task_trigger(NRF_RTC1, NRF_RTC_TASK_START);
}

/**
 * @brief  Configures Button GPIO, EXTI Line and DEBOUNCE Timer.
 * @param  Button: Specifies the Button to be configured.
 *   This parameter can be one of following parameters:
 *     @arg BUTTON1: Button1
 * @param  Button_Mode: Specifies Button mode.
 *   This parameter can be one of following parameters:
 *     @arg BUTTON_MODE_GPIO: Button will be used as simple IO
 *     @arg BUTTON_MODE_EXTI: Button will be connected to EXTI line with interrupt
 *                     generation capability
 * @retval None
 */
void BUTTON_Init(Button_TypeDef Button, ButtonMode_TypeDef Button_Mode)
{
    nrf_gpio_cfg(
        HAL_Buttons[Button].pin,
        HAL_Buttons[Button].mode,
        NRF_GPIO_PIN_INPUT_CONNECT,
        HAL_Buttons[Button].pupd,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE);

    rtc_timer_init();

    if (Button_Mode == BUTTON_MODE_EXTI)
    {
        /* Disable RTC0 tick Interrupt */
        nrf_rtc_int_disable(NRF_RTC1, NRF_RTC_INT_TICK_MASK);

        BUTTON_EXTI_Config(Button, ENABLE);

        NVIC_SetPriority(HAL_Buttons[Button].nvic_irqn, HAL_Buttons[Button].nvic_irq_prio);
        NVIC_ClearPendingIRQ(HAL_Buttons[Button].nvic_irqn);
        NVIC_EnableIRQ(HAL_Buttons[Button].nvic_irqn);

        /* Enable the RTC0 NVIC Interrupt */
        NVIC_SetPriority(RTC1_IRQn, RTC1_IRQ_PRIORITY);
        NVIC_ClearPendingIRQ(RTC1_IRQn);
        NVIC_EnableIRQ(RTC1_IRQn);
    }
}

void BUTTON_EXTI_Config(Button_TypeDef Button, FunctionalState NewState)
{
    nrf_gpiote_polarity_t gpiote_polar = NRF_GPIOTE_POLARITY_TOGGLE;

    switch (HAL_Buttons[Button].interrupt_mode)
    {
        case RISING:  gpiote_polar = NRF_GPIOTE_POLARITY_LOTOHI; break;
        case FALLING: gpiote_polar = NRF_GPIOTE_POLARITY_HITOLO; break;
        case CHANGE:  gpiote_polar = NRF_GPIOTE_POLARITY_TOGGLE; break;
    }

    nrf_gpiote_event_configure(HAL_Buttons[Button].event_channel,
                               HAL_Buttons[Button].pin,
                               gpiote_polar);

    nrf_gpiote_event_clear(HAL_Buttons[Button].event_in);

    nrf_gpiote_event_enable(HAL_Buttons[Button].event_channel);

    if (NewState == ENABLE)
    {
        nrf_gpiote_int_enable(HAL_Buttons[Button].int_mask);
    }
    else
    {
        nrf_gpiote_int_disable(HAL_Buttons[Button].int_mask);
    }
}

/**
 * @brief  Returns the selected Button non-filtered state.
 * @param  Button: Specifies the Button to be checked.
 *   This parameter can be one of following parameters:
 *     @arg BUTTON1: Button1
 * @retval Actual Button Pressed state.
 */
uint8_t BUTTON_GetState(Button_TypeDef Button)
{
    return nrf_gpio_pin_read(HAL_Buttons[Button].pin);
}

/**
 * @brief  Returns the selected Button Debounced Time.
 * @param  Button: Specifies the Button to be checked.
 *   This parameter can be one of following parameters:
 *     @arg BUTTON1: Button1
 * @retval Button Debounced time in millisec.
 */
uint16_t BUTTON_GetDebouncedTime(Button_TypeDef Button)
{
    return HAL_Buttons[Button].debounce_time;
}

void BUTTON_ResetDebouncedState(Button_TypeDef Button)
{
    HAL_Buttons[Button].debounce_time = 0;
}


void BUTTON_Irq_Handler(void)
{
    if (nrf_gpiote_event_is_set(HAL_Buttons[BUTTON1].event_in))
    {
        nrf_gpiote_event_clear(HAL_Buttons[BUTTON1].event_in);

        BUTTON_Check_Irq(BUTTON1);
    }
}

void BUTTON_Check_Irq(uint16_t button)
{
    HAL_Buttons[button].debounce_time = 0x00;
    HAL_Buttons[button].active = true;

    /* Disable button Interrupt */
    BUTTON_EXTI_Config(button, DISABLE);

    /* Enable RTC1 tick interrupt */
    nrf_rtc_int_enable(NRF_RTC1, NRF_RTC_INT_TICK_MASK);
}

void BUTTON_Check_State(uint16_t button, uint8_t pressed)
{
    if (BUTTON_GetState(button) == pressed)
    {
        if (!HAL_Buttons[button].active)
            HAL_Buttons[button].active = true;
        HAL_Buttons[button].debounce_time += BUTTON_DEBOUNCE_INTERVAL;
    }
    else if (HAL_Buttons[button].active)
    {
        HAL_Buttons[button].active = false;
        /* Enable button Interrupt */
        BUTTON_EXTI_Config(button, ENABLE);
    }
}

int BUTTON_Debounce()
{
    BUTTON_Check_State(BUTTON1, BUTTON1_PRESSED);

    int pressed = HAL_Buttons[BUTTON1].active;
    if (pressed == 0)
    {
        /* Disable RTC1 tick Interrupt */
        nrf_rtc_int_disable(NRF_RTC1, NRF_RTC_INT_TICK_MASK);
    }

    return pressed;
}

void BUTTON_Init_Ext()
{
    if (BUTTON_Debounce())
        nrf_rtc_int_enable(NRF_RTC1, NRF_RTC_INT_TICK_MASK);
}

uint8_t BUTTON_Is_Pressed(Button_TypeDef button)
{
    return HAL_Buttons[button].active;
}

uint16_t BUTTON_Pressed_Time(Button_TypeDef button)
{
    return HAL_Buttons[button].debounce_time;
}

void BUTTON_Uninit()
{
    NVIC_DisableIRQ(GPIOTE_IRQn);
    NVIC_ClearPendingIRQ(GPIOTE_IRQn);
    NVIC_SetPriority(GPIOTE_IRQn, 0);

    for (int i = 0; i < BUTTONn; i++)
    {
        nrf_gpiote_int_disable(HAL_Buttons[i].int_mask);
        nrf_gpiote_te_default(HAL_Buttons[i].event_channel);
    }
}