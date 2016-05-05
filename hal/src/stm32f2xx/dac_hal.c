/**
 ******************************************************************************
 * @file    dac_hal.c
 * @authors Satish Nair
 * @version V1.0.0
 * @date    7-Jan-2015
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
#include "dac_hal.h"
#include "gpio_hal.h"
#include "pinmap_hal.h"
#include "pinmap_impl.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

uint8_t dacInitFirstTime = true;

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/*
 * @brief Initialize the DAC peripheral.
 */
static void HAL_DAC_Init()
{
    DAC_InitTypeDef DAC_InitStructure;

    /* DAC Periph clock enable */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

    DAC_DeInit();

    /* DAC channel1 & channel2 Configuration */
    memset(&DAC_InitStructure, 0, sizeof DAC_InitStructure);
    DAC_InitStructure.DAC_Trigger = DAC_Trigger_None;
    DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
    DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;

    DAC_Init(DAC_Channel_1, &DAC_InitStructure);
    DAC_Init(DAC_Channel_2, &DAC_InitStructure);
}

/*
 * @brief Write the analog value to the pin.
 * DAC is 12-bit. Range of values: 0-4096
 */
void HAL_DAC_Write(pin_t pin, uint16_t value)
{
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();

    if (dacInitFirstTime == true)
    {
        HAL_DAC_Init();
        dacInitFirstTime = false;
    }

    if (HAL_Get_Pin_Mode(pin) != AN_OUTPUT)
    {
        HAL_Pin_Mode(pin, AN_OUTPUT);
        HAL_DAC_Enable(pin, 1);
    }

    if (PIN_MAP[pin].dac_channel == DAC_Channel_1)
    {
        /* Set the DAC Channel1 data */
        DAC_SetChannel1Data(DAC_Align_12b_R, value);

    }
    else if (PIN_MAP[pin].dac_channel == DAC_Channel_2)
    {
        /* Set the DAC Channel2 data */
        DAC_SetChannel2Data(DAC_Align_12b_R, value);
    }
}

uint8_t HAL_DAC_Is_Enabled(pin_t pin)
{
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    if (!dacInitFirstTime && HAL_Get_Pin_Mode(pin) == AN_OUTPUT)
    {
        if (DAC->CR & (DAC_CR_EN1 << PIN_MAP[pin].dac_channel))
            return 1;
    }

    return 0;
}

uint8_t HAL_DAC_Enable(pin_t pin, uint8_t state)
{
    STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();
    if (!dacInitFirstTime && HAL_Get_Pin_Mode(pin) == AN_OUTPUT)
    {
        DAC_Cmd(PIN_MAP[pin].dac_channel, state ? ENABLE : DISABLE);
        return 0;
    }

    return 1;
}
