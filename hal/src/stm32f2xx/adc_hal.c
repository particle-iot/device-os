/******************************************************************************
 * @file    adc_hal.c
 * @authors Satish Nair
 * @version V1.0.0
 * @date    22-Dec-2014
 * @brief
 ******************************************************************************/
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

/* Includes ------------------------------------------------------------------*/
#include "adc_hal.h"
#include "gpio_hal.h"
#include "pinmap_hal.h"
#include "pinmap_impl.h"
#include "system_error.h"
#include <stddef.h>

/* Private define ------------------------------------------------------------*/
#define ADC_CDR_ADDRESS     ((uint32_t)0x40012308)
#define ADC_DMA_BUFFERSIZE  10
#define ADC_SAMPLING_TIME   ADC_SampleTime_480Cycles

__IO uint16_t convertedValues[ADC_DMA_BUFFERSIZE];
static hal_adc_state_t adcState = HAL_ADC_STATE_DISABLED;
static uint8_t channelConfigured = ADC_CHANNEL_NONE;
static uint8_t sampleTime = ADC_SAMPLING_TIME;

/*
 * @brief  Override the default ADC Sample time depending on requirement
 * @param  ADC_SampleTime: The sample time value to be set.
 * This parameter can be one of the following values:
 * @arg ADC_SampleTime_3Cycles: Sample time equal to 3 cycles
 * @arg ADC_SampleTime_15Cycles: Sample time equal to 15 cycles
 * @arg ADC_SampleTime_28Cycles: Sample time equal to 28 cycles
 * @arg ADC_SampleTime_56Cycles: Sample time equal to 56 cycles
 * @arg ADC_SampleTime_84Cycles: Sample time equal to 84 cycles
 * @arg ADC_SampleTime_112Cycles: Sample time equal to 112 cycles
 * @arg ADC_SampleTime_144Cycles: Sample time equal to 144 cycles
 * @arg ADC_SampleTime_480Cycles: Sample time equal to 480 cycles
 * @retval None
 */
void hal_adc_set_sample_time(uint8_t sample_time) {
    if (sample_time < ADC_SampleTime_3Cycles || sample_time > ADC_SampleTime_480Cycles) {
        sampleTime = ADC_SAMPLING_TIME;
    } else {
        sampleTime = sample_time;
    }
}

/*
 * @brief Read the analog value of a pin.
 * Should return a 16-bit value, 0-65536 (0 = LOW, 65536 = HIGH)
 * Note: ADC is 12-bit. Currently it returns 0-4096
 */
int32_t hal_adc_read(uint16_t pin) {
    int i = 0;
    Hal_Pin_Info* PIN_MAP = HAL_Pin_Map();

    if (PIN_MAP[pin].pin_mode != AN_INPUT) {
        HAL_GPIO_Save_Pin_Mode(pin);
        HAL_Pin_Mode(pin, AN_INPUT);
    }

    if (adcState != HAL_ADC_STATE_ENABLED) {
        hal_adc_dma_init();
    }

    if (channelConfigured != PIN_MAP[pin].adc_channel) {
        // ADC1 regular channel configuration
        ADC_RegularChannelConfig(ADC1, PIN_MAP[pin].adc_channel, 1, sampleTime);
        // ADC2 regular channel configuration
        ADC_RegularChannelConfig(ADC2, PIN_MAP[pin].adc_channel, 1, sampleTime);
        // Save the ADC configured channel
        channelConfigured = PIN_MAP[pin].adc_channel;
    }

    for (i = 0; i < ADC_DMA_BUFFERSIZE; i++) {
        convertedValues[i] = 0;
    }

    // Enable DMA2_Stream0
    DMA_Cmd(DMA2_Stream0, ENABLE);

    // Enable DMA request after last transfer (Multi-ADC mode)
    ADC_MultiModeDMARequestAfterLastTransferCmd(ENABLE);

    // Enable ADC1
    ADC_Cmd(ADC1, ENABLE);

    // Enable ADC2
    ADC_Cmd(ADC2, ENABLE);

    // Start ADC1 Software Conversion
    ADC_SoftwareStartConv(ADC1);

    // Test on DMA2 Stream0 DMA_FLAG_TCIF0 flag
    while (!DMA_GetFlagStatus(DMA2_Stream0, DMA_FLAG_TCIF0));

    // Clear DMA2 Stream0 DMA_FLAG_TCIF0 flag
    DMA_ClearFlag(DMA2_Stream0, DMA_FLAG_TCIF0);

    // Disable DMA2_Stream0
    DMA_Cmd(DMA2_Stream0, DISABLE);

    // Disable DMA request after last transfer (Multi-ADC mode)
    ADC_MultiModeDMARequestAfterLastTransferCmd(DISABLE);

    // Disable ADC1
    ADC_Cmd(ADC1, DISABLE);

    // Disable ADC2
    ADC_Cmd(ADC2, DISABLE);

    uint32_t summatedValue = 0;
    uint16_t averagedValue = 0;

    for (i = 0; i < ADC_DMA_BUFFERSIZE; i++) {
        summatedValue += convertedValues[i];
    }

    averagedValue = (uint16_t)(summatedValue / ADC_DMA_BUFFERSIZE);

    // Return ADC averaged value
    return averagedValue;
}

/*
 * @brief Initialize the ADC peripheral.
 */
void hal_adc_dma_init() {
    //Using Dual ADC Regular Simultaneous DMA Mode 1
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;

    // Enable DMA2 clock
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

    // Enable ADC1 and ADC2 clock
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_ADC2, ENABLE);

    // DMA2 Stream0 channel0 configuration
    DMA_InitStructure.DMA_Channel = DMA_Channel_0;
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&convertedValues;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)ADC_CDR_ADDRESS;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize = ADC_DMA_BUFFERSIZE;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init(DMA2_Stream0, &DMA_InitStructure);

    /* ADC Common Init */
    ADC_CommonInitStructure.ADC_Mode = ADC_DualMode_RegSimult;
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_1;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit(&ADC_CommonInitStructure);

    // ADC1 configuration
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    // ADC2 configuration
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion = 1;
    ADC_Init(ADC2, &ADC_InitStructure);

    adcState = HAL_ADC_STATE_ENABLED;
}

/*
 * @brief Uninitialize the ADC peripheral.
 */
int hal_adc_dma_uninit(void* reserved) {
    // Disable DMA2 clock
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, DISABLE);

    // Disable ADC1 and ADC2 clock
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_ADC2, DISABLE);

    adcState = HAL_ADC_STATE_DISABLED;

    return SYSTEM_ERROR_NONE;
}

/*
 * @brief ADC peripheral enters sleep mode
 */
int hal_adc_sleep(bool sleep, void* reserved) {
    if (sleep) {
        // Suspend ADC
        if (adcState != HAL_ADC_STATE_ENABLED) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        hal_adc_dma_uninit(NULL);
        adcState = HAL_ADC_STATE_SUSPENDED;
    } else {
        // Restore ADC
        if (adcState != HAL_ADC_STATE_SUSPENDED) {
            return SYSTEM_ERROR_INVALID_STATE;
        }
        hal_adc_dma_init();
    }
    return SYSTEM_ERROR_NONE;
}