/**
 ******************************************************************************
 * @file    timer_hal.c
 * @authors Satish Nair, Brett Walach
 * @version V1.0.0
 * @date    12-Sept-2014
 * @brief
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "hw_config.h"
#include "core_hal.h"
#include "timer_hal.h"
#include "syshealth_hal.h"


/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
struct timer_info
{
    uint32_t RCC_APB1Periph;
    TIM_TypeDef* TIMx;
    hal_irq_t hal_irq;
    IRQn_Type IRQn;
};

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static const struct timer_info timers[] =
{
    [HAL_Timer2] = {.RCC_APB1Periph = RCC_APB1Periph_TIM2, .TIMx = TIM2, .hal_irq = SysInterrupt_TIM2_IRQ, .IRQn = TIM2_IRQn},
    [HAL_Timer3] = {.RCC_APB1Periph = RCC_APB1Periph_TIM3, .TIMx = TIM3, .hal_irq = SysInterrupt_TIM3_IRQ, .IRQn = TIM3_IRQn},
    [HAL_Timer4] = {.RCC_APB1Periph = RCC_APB1Periph_TIM4, .TIMx = TIM4, .hal_irq = SysInterrupt_TIM4_IRQ, .IRQn = TIM4_IRQn},
    [HAL_Timer5] = {.RCC_APB1Periph = RCC_APB1Periph_TIM5, .TIMx = TIM5, .hal_irq = SysInterrupt_TIM5_IRQ, .IRQn = TIM5_IRQn},
    [HAL_Timer7] = {.RCC_APB1Periph = RCC_APB1Periph_TIM7, .TIMx = TIM7, .hal_irq = SysInterrupt_TIM7_IRQ, .IRQn = TIM7_IRQn}
};

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/*
 * @brief Should return the number of microseconds since the processor started up.
 */
system_tick_t HAL_Timer_Get_Micro_Seconds(void)
{
    return (DWT->CYCCNT / SYSTEM_US_TICKS);
}

/*
 * @brief Should return the number of milliseconds since the processor started up.
 */
system_tick_t HAL_Timer_Get_Milli_Seconds(void)
{
    // review - wiced_time_get_time()) ??
    return GetSystem1MsTick();
}

void HAL_Timer_Start(HAL_Timer timer, uint32_t period, const HAL_InterruptCallback* callback)
{
    struct timer_info selected_timer = timers[timer];

    // enable timer peripheral
    RCC_APB1PeriphClockCmd(selected_timer.RCC_APB1Periph, ENABLE);

    // configure timer
    TIM_TimeBaseInitTypeDef timerInitStructure;
    TIM_TimeBaseStructInit(&timerInitStructure);
    timerInitStructure.TIM_Prescaler = (uint16_t)(SystemCoreClock / 2000) - 1;     //To get TIM counter clock = 1KHz;
    timerInitStructure.TIM_Period = period;
    TIM_TimeBaseInit(selected_timer.TIMx, &timerInitStructure);

    // enable timer
    TIM_Cmd(selected_timer.TIMx, ENABLE);

    // config timer interrupt
    TIM_ITConfig(selected_timer.TIMx, TIM_IT_Update, ENABLE);

    // hook irq handler
    HAL_Set_System_Interrupt_Handler(selected_timer.hal_irq, callback, NULL, NULL);

    // configure IRQ
    NVIC_InitTypeDef nvicStructure;
    nvicStructure.NVIC_IRQChannel = selected_timer.IRQn;
    nvicStructure.NVIC_IRQChannelPreemptionPriority = 10;
    nvicStructure.NVIC_IRQChannelSubPriority = 0;
    nvicStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvicStructure);
}

void HAL_Timer_Stop(HAL_Timer timer)
{
    // TODO: test better

    struct timer_info selected_timer = timers[timer];

    // disable counter
    TIM_Cmd(selected_timer.TIMx, DISABLE);

    // // disable interrupt
    NVIC_InitTypeDef nvicStructure;
    nvicStructure.NVIC_IRQChannel = selected_timer.IRQn;
    nvicStructure.NVIC_IRQChannelCmd = DISABLE;
    NVIC_Init(&nvicStructure);

    // disable timer peripheral
    TIM_DeInit(selected_timer.TIMx);
}
