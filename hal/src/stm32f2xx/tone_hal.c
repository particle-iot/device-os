/**
 ******************************************************************************
 * @file    tone_hal.c
 * @author  Satish Nair
 * @version V1.0.0
 * @date    26-Dec-2014
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
#include "tone_hal.h"
#include "gpio_hal.h"
#include "timer_hal.h"
#include "pinmap_impl.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define TONE_TIM_COUNTER_CLOCK_FREQ 1000000 ////TIM Counter clock = 1MHz

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
extern void (*HAL_TIM1_Handler)(void);
extern void (*HAL_TIM3_Handler)(void);
extern void (*HAL_TIM4_Handler)(void);
extern void (*HAL_TIM5_Handler)(void);
#if PLATFORM_ID == 10 // Electron
extern void (*HAL_TIM8_Handler)(void);
#endif

static void Tone_TIM1_Handler(void);
static void Tone_TIM3_Handler(void);
static void Tone_TIM4_Handler(void);
static void Tone_TIM5_Handler(void);
#if PLATFORM_ID == 10 // Electron
static void Tone_TIM8_Handler(void);
#endif

void HAL_Tone_Start(uint8_t pin, uint32_t frequency, uint32_t duration)
{
    if(frequency < 20 || frequency > 20000)
    {
        return;//no tone for frequency outside of human audible range
    }

    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    TIM_OCInitTypeDef  TIM_OCInitStructure;

    uint32_t TIM_CLK = SystemCoreClock;

    HAL_Pin_Mode(pin, AF_OUTPUT_PUSHPULL);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    // TIM clock enable
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    if(PIN_MAP[pin].timer_peripheral == TIM1)
    {
        TIM_CLK = SystemCoreClock;
        GPIO_PinAFConfig(PIN_MAP[pin].gpio_peripheral, PIN_MAP[pin].gpio_pin_source, GPIO_AF_TIM1);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
        NVIC_InitStructure.NVIC_IRQChannel = TIM1_CC_IRQn;
        HAL_TIM1_Handler = Tone_TIM1_Handler;
    }
    else if(PIN_MAP[pin].timer_peripheral == TIM3)
    {
        TIM_CLK = SystemCoreClock / 2;
        GPIO_PinAFConfig(PIN_MAP[pin].gpio_peripheral, PIN_MAP[pin].gpio_pin_source, GPIO_AF_TIM3);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
        NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
        HAL_TIM3_Handler = Tone_TIM3_Handler;
    }
    else if(PIN_MAP[pin].timer_peripheral == TIM4)
    {
        TIM_CLK = SystemCoreClock / 2;
        GPIO_PinAFConfig(PIN_MAP[pin].gpio_peripheral, PIN_MAP[pin].gpio_pin_source, GPIO_AF_TIM4);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
        NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
        HAL_TIM4_Handler = Tone_TIM4_Handler;
    }
    else if(PIN_MAP[pin].timer_peripheral == TIM5)
    {
        TIM_CLK = SystemCoreClock / 2;
        GPIO_PinAFConfig(PIN_MAP[pin].gpio_peripheral, PIN_MAP[pin].gpio_pin_source, GPIO_AF_TIM5);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
        NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
        HAL_TIM5_Handler = Tone_TIM5_Handler;
    }
#if PLATFORM_ID == 10 // Electron
    else if(PIN_MAP[pin].timer_peripheral == TIM8)
    {
        TIM_CLK = SystemCoreClock;
        GPIO_PinAFConfig(PIN_MAP[pin].gpio_peripheral, PIN_MAP[pin].gpio_pin_source, GPIO_AF_TIM8);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);
        NVIC_InitStructure.NVIC_IRQChannel = TIM8_CC_IRQn;
        HAL_TIM8_Handler = Tone_TIM8_Handler;
    }
#endif

    uint16_t TIM_Prescaler = (uint16_t)(TIM_CLK / TONE_TIM_COUNTER_CLOCK_FREQ) - 1;
    uint16_t TIM_CCR = (uint16_t)(TONE_TIM_COUNTER_CLOCK_FREQ / (2 * frequency));
    int32_t timer_channel_toggle_count = -1;

    // Calculate the toggle count
    if (duration > 0)
    {
        timer_channel_toggle_count = 2 * frequency * duration / 1000;
    }

    PIN_MAP[pin].timer_ccr = TIM_CCR;
    PIN_MAP[pin].user_property = timer_channel_toggle_count;

    NVIC_Init(&NVIC_InitStructure);

    // Time base configuration
    if (PIN_MAP[pin].timer_peripheral == TIM5)
        TIM_TimeBaseStructure.TIM_Period = 0xFFFFFFFF;  //32-bit timer
    else
        TIM_TimeBaseStructure.TIM_Period = 0xFFFF;      //16-bit timer
    TIM_TimeBaseStructure.TIM_Prescaler = TIM_Prescaler;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(PIN_MAP[pin].timer_peripheral, &TIM_TimeBaseStructure);

    // Output Compare Toggle Mode configuration
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Toggle;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
    TIM_OCInitStructure.TIM_Pulse = TIM_CCR;

    if(PIN_MAP[pin].timer_ch == TIM_Channel_1)
    {
        //Since A4 and D3 share the same TIM3->Channel1, only one can work at a time
        if(pin == 14)
            PIN_MAP[3].timer_ccr = 0;
        else if(pin == 3)
            PIN_MAP[14].timer_ccr = 0;

        // Channel1 configuration
        TIM_OC1Init(PIN_MAP[pin].timer_peripheral, &TIM_OCInitStructure);
        TIM_OC1PreloadConfig(PIN_MAP[pin].timer_peripheral, TIM_OCPreload_Disable);
        TIM_ITConfig(PIN_MAP[pin].timer_peripheral, TIM_IT_CC1, ENABLE);
    }
    else if(PIN_MAP[pin].timer_ch == TIM_Channel_2)
    {
        //Since A5 and D2 share the same TIM3->Channel2, only one can work at a time
        if(pin == 15)
            PIN_MAP[2].timer_ccr = 0;
        else if(pin == 2)
            PIN_MAP[15].timer_ccr = 0;

        // Channel2 configuration
        TIM_OC2Init(PIN_MAP[pin].timer_peripheral, &TIM_OCInitStructure);
        TIM_OC2PreloadConfig(PIN_MAP[pin].timer_peripheral, TIM_OCPreload_Disable);
        TIM_ITConfig(PIN_MAP[pin].timer_peripheral, TIM_IT_CC2, ENABLE);
    }
    else if(PIN_MAP[pin].timer_ch == TIM_Channel_3)
    {
        // Channel3 configuration
        TIM_OC3Init(PIN_MAP[pin].timer_peripheral, &TIM_OCInitStructure);
        TIM_OC3PreloadConfig(PIN_MAP[pin].timer_peripheral, TIM_OCPreload_Disable);
        TIM_ITConfig(PIN_MAP[pin].timer_peripheral, TIM_IT_CC3, ENABLE);
    }
    else if(PIN_MAP[pin].timer_ch == TIM_Channel_4)
    {
        // Channel4 configuration
        TIM_OC4Init(PIN_MAP[pin].timer_peripheral, &TIM_OCInitStructure);
        TIM_OC4PreloadConfig(PIN_MAP[pin].timer_peripheral, TIM_OCPreload_Disable);
        TIM_ITConfig(PIN_MAP[pin].timer_peripheral, TIM_IT_CC4, ENABLE);
    }

    // TIM enable counter
    TIM_Cmd(PIN_MAP[pin].timer_peripheral, ENABLE);

    if((PIN_MAP[pin].timer_peripheral == TIM1) || (PIN_MAP[pin].timer_peripheral == TIM8))
    {
        /* TIM Main Output Enable - required for TIM1/TIM8 PWM output */
        TIM_CtrlPWMOutputs(PIN_MAP[pin].timer_peripheral, ENABLE);
    }
}

void HAL_Tone_Stop(uint8_t pin)
{
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    if(PIN_MAP[pin].timer_ch == TIM_Channel_1)
    {
        TIM_ITConfig(PIN_MAP[pin].timer_peripheral, TIM_IT_CC1, DISABLE);
    }
    else if(PIN_MAP[pin].timer_ch == TIM_Channel_2)
    {
        TIM_ITConfig(PIN_MAP[pin].timer_peripheral, TIM_IT_CC2, DISABLE);
    }
    else if(PIN_MAP[pin].timer_ch == TIM_Channel_3)
    {
        TIM_ITConfig(PIN_MAP[pin].timer_peripheral, TIM_IT_CC3, DISABLE);
    }
    else if(PIN_MAP[pin].timer_ch == TIM_Channel_4)
    {
        TIM_ITConfig(PIN_MAP[pin].timer_peripheral, TIM_IT_CC4, DISABLE);
    }

    TIM_CCxCmd(PIN_MAP[pin].timer_peripheral, PIN_MAP[pin].timer_ch, TIM_CCx_Disable);
    PIN_MAP[pin].timer_ccr = 0;
    PIN_MAP[pin].user_property = 0;
}

//HAL_Tone_Get_Frequency() should be called immediately after calling tone()
//"only" useful for unit-testing
uint32_t HAL_Tone_Get_Frequency(uint8_t pin)
{
    uint16_t TIM_CCR = 0;
    uint16_t Tone_Frequency = 0;

    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    if(PIN_MAP[pin].timer_ch == TIM_Channel_1)
    {
        TIM_CCR = PIN_MAP[pin].timer_peripheral->CCR1;
    }
    else if(PIN_MAP[pin].timer_ch == TIM_Channel_2)
    {
        TIM_CCR = PIN_MAP[pin].timer_peripheral->CCR2;
    }
    else if(PIN_MAP[pin].timer_ch == TIM_Channel_3)
    {
        TIM_CCR = PIN_MAP[pin].timer_peripheral->CCR3;
    }
    else if(PIN_MAP[pin].timer_ch == TIM_Channel_4)
    {
        TIM_CCR = PIN_MAP[pin].timer_peripheral->CCR4;
    }
    else
    {
        return Tone_Frequency;
    }

    Tone_Frequency = (uint16_t)(TONE_TIM_COUNTER_CLOCK_FREQ / TIM_CCR);

    return Tone_Frequency;
}

bool HAL_Tone_Is_Stopped(uint8_t pin)
{
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    if(PIN_MAP[pin].timer_ccr > 0)
    {
        return false;
    }

    return true;
}

static void Tone_TIM1_Handler(void)
{
    uint16_t capture;

    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();

    if (TIM_GetITStatus(TIM1, TIM_IT_CC2) != RESET)
    {
        TIM_ClearITPendingBit(TIM1, TIM_IT_CC2);
        capture = TIM_GetCapture2(TIM1);
        TIM_SetCompare2(TIM1, capture + PIN_MAP[19].timer_ccr);
        if(PIN_MAP[19].user_property != -1)
        {
            if (PIN_MAP[19].user_property > 0)
            {
                PIN_MAP[19].user_property -= 1;
            }
            else
            {
                HAL_Tone_Stop(19);
            }
        }
    }

    if (TIM_GetITStatus(TIM1, TIM_IT_CC3) != RESET)
    {
        TIM_ClearITPendingBit(TIM1, TIM_IT_CC3);
        capture = TIM_GetCapture3(TIM1);
        TIM_SetCompare3(TIM1, capture + PIN_MAP[18].timer_ccr);
        if(PIN_MAP[18].user_property != -1)
        {
            if (PIN_MAP[18].user_property > 0)
            {
                PIN_MAP[18].user_property -= 1;
            }
            else
            {
                HAL_Tone_Stop(18);
            }
        }
    }
}

static void Tone_TIM3_Handler(void)
{
    uint16_t capture;

    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();

    if (TIM_GetITStatus(TIM3, TIM_IT_CC1) != RESET)
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_CC1 );
        capture = TIM_GetCapture1(TIM3);
        //Since A4 and D3 share the same TIM3->Channel1, only one can work at a time
        if(PIN_MAP[14].timer_ccr)
        {
            TIM_SetCompare1(TIM3, capture + PIN_MAP[14].timer_ccr);
            if(PIN_MAP[14].user_property != -1)
            {
                if (PIN_MAP[14].user_property > 0)
                {
                    PIN_MAP[14].user_property -= 1;
                }
                else
                {
                    HAL_Tone_Stop(14);
                }
            }
        }
        else if(PIN_MAP[3].timer_ccr)
        {
            TIM_SetCompare1(TIM3, capture + PIN_MAP[3].timer_ccr);
            if(PIN_MAP[3].user_property != -1)
            {
                if (PIN_MAP[3].user_property > 0)
                {
                    PIN_MAP[3].user_property -= 1;
                }
                else
                {
                    HAL_Tone_Stop(3);
                }
            }
        }
    }

    if (TIM_GetITStatus(TIM3, TIM_IT_CC2) != RESET)
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_CC2);
        capture = TIM_GetCapture2(TIM3);
        //Since A5 and D2 share the same TIM3->Channel2, only one can work at a time
        if(PIN_MAP[15].timer_ccr)
        {
            TIM_SetCompare2(TIM3, capture + PIN_MAP[15].timer_ccr);
            if(PIN_MAP[15].user_property != -1)
            {
                if (PIN_MAP[15].user_property > 0)
                {
                    PIN_MAP[15].user_property -= 1;
                }
                else
                {
                    HAL_Tone_Stop(15);
                }
            }
        }
        else if(PIN_MAP[2].timer_ccr)
        {
            TIM_SetCompare2(TIM3, capture + PIN_MAP[2].timer_ccr);
            if(PIN_MAP[2].user_property != -1)
            {
                if (PIN_MAP[2].user_property > 0)
                {
                    PIN_MAP[2].user_property -= 1;
                }
                else
                {
                    HAL_Tone_Stop(2);
                }
            }
        }
    }

#if PLATFORM_ID == 10 // Electron
    if (TIM_GetITStatus(TIM3, TIM_IT_CC3) != RESET)
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_CC3);
        capture = TIM_GetCapture3(TIM3);
        TIM_SetCompare3(TIM3, capture + PIN_MAP[26].timer_ccr);
        if(PIN_MAP[26].user_property != -1)
        {
            if (PIN_MAP[26].user_property > 0)
            {
                PIN_MAP[26].user_property -= 1;
            }
            else
            {
                HAL_Tone_Stop(26);
            }
        }
    }

    if (TIM_GetITStatus(TIM3, TIM_IT_CC4) != RESET)
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_CC4);
        capture = TIM_GetCapture4(TIM3);
        TIM_SetCompare4(TIM3, capture + PIN_MAP[27].timer_ccr);
        if(PIN_MAP[27].user_property != -1)
        {
            if (PIN_MAP[27].user_property > 0)
            {
                PIN_MAP[27].user_property -= 1;
            }
            else
            {
                HAL_Tone_Stop(27);
            }
        }
    }
#endif
}

static void Tone_TIM4_Handler(void)
{
    uint16_t capture;

    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();

    if (TIM_GetITStatus(TIM4, TIM_IT_CC1) != RESET)
    {
        TIM_ClearITPendingBit(TIM4, TIM_IT_CC1 );
        capture = TIM_GetCapture1(TIM4);
        TIM_SetCompare1(TIM4, capture + PIN_MAP[1].timer_ccr);
        if(PIN_MAP[1].user_property != -1)
        {
            if (PIN_MAP[1].user_property > 0)
            {
                PIN_MAP[1].user_property -= 1;
            }
            else
            {
                HAL_Tone_Stop(1);
            }
        }
    }

    if (TIM_GetITStatus(TIM4, TIM_IT_CC2) != RESET)
    {
        TIM_ClearITPendingBit(TIM4, TIM_IT_CC2);
        capture = TIM_GetCapture2(TIM4);
        TIM_SetCompare2(TIM4, capture + PIN_MAP[0].timer_ccr);
        if(PIN_MAP[0].user_property != -1)
        {
            if (PIN_MAP[0].user_property > 0)
            {
                PIN_MAP[0].user_property -= 1;
            }
            else
            {
                HAL_Tone_Stop(0);
            }
        }
    }

#if PLATFORM_ID == 10 // Electron
    if (TIM_GetITStatus(TIM4, TIM_IT_CC3) != RESET)
    {
        TIM_ClearITPendingBit(TIM4, TIM_IT_CC3);
        capture = TIM_GetCapture3(TIM4);
        TIM_SetCompare3(TIM4, capture + PIN_MAP[35].timer_ccr);
        if(PIN_MAP[35].user_property != -1)
        {
            if (PIN_MAP[35].user_property > 0)
            {
                PIN_MAP[35].user_property -= 1;
            }
            else
            {
                HAL_Tone_Stop(35);
            }
        }
    }

    if (TIM_GetITStatus(TIM4, TIM_IT_CC4) != RESET)
    {
        TIM_ClearITPendingBit(TIM4, TIM_IT_CC4);
        capture = TIM_GetCapture4(TIM4);
        TIM_SetCompare4(TIM4, capture + PIN_MAP[34].timer_ccr);
        if(PIN_MAP[34].user_property != -1)
        {
            if (PIN_MAP[34].user_property > 0)
            {
                PIN_MAP[34].user_property -= 1;
            }
            else
            {
                HAL_Tone_Stop(34);
            }
        }
    }
#endif
}

static void Tone_TIM5_Handler(void)
{
    uint32_t capture;

    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();

    if (TIM_GetITStatus(TIM5, TIM_IT_CC1) != RESET)
    {
        TIM_ClearITPendingBit(TIM5, TIM_IT_CC1 );
        capture = TIM_GetCapture1(TIM5);
        TIM_SetCompare1(TIM5, capture + PIN_MAP[17].timer_ccr);
        if(PIN_MAP[17].user_property != -1)
        {
            if (PIN_MAP[17].user_property > 0)
            {
                PIN_MAP[17].user_property -= 1;
            }
            else
            {
                HAL_Tone_Stop(17);
            }
        }
    }
}

#if PLATFORM_ID == 10 // Electron
static void Tone_TIM8_Handler(void)
{
    uint16_t capture;

    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();

    if (TIM_GetITStatus(TIM8, TIM_IT_CC1) != RESET)
    {
        TIM_ClearITPendingBit(TIM8, TIM_IT_CC1);
        capture = TIM_GetCapture1(TIM8);
        TIM_SetCompare1(TIM8, capture + PIN_MAP[25].timer_ccr);
        if(PIN_MAP[25].user_property != -1)
        {
            if (PIN_MAP[25].user_property > 0)
            {
                PIN_MAP[25].user_property -= 1;
            }
            else
            {
                HAL_Tone_Stop(25);
            }
        }
    }

    if (TIM_GetITStatus(TIM8, TIM_IT_CC3) != RESET)
    {
        TIM_ClearITPendingBit(TIM8, TIM_IT_CC3);
        capture = TIM_GetCapture3(TIM8);
        TIM_SetCompare3(TIM8, capture + PIN_MAP[24].timer_ccr);
        if(PIN_MAP[24].user_property != -1)
        {
            if (PIN_MAP[24].user_property > 0)
            {
                PIN_MAP[24].user_property -= 1;
            }
            else
            {
                HAL_Tone_Stop(24);
            }
        }
    }
}
#endif
