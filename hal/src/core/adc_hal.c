/**
 ******************************************************************************
 * @file    adc_hal.c
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
#include "adc_hal.h"
#include "gpio_hal.h"
#include "pinmap_hal.h"
#include "pinmap_impl.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

#define ADC1_DR_ADDRESS   ((uint32_t)0x4001244C)
#define ADC_DMA_BUFFERSIZE  10
#define ADC_SAMPLING_TIME ADC_SampleTime_7Cycles5 //Allowed values: 1.5, 7.5 and 13.5 for "Dual slow interleaved mode"

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

__IO uint32_t ADC_DualConvertedValues[ADC_DMA_BUFFERSIZE];
uint8_t adcInitFirstTime = true;
uint8_t adcChannelConfigured = ADC_CHANNEL_NONE;
static uint8_t ADC_Sample_Time = ADC_SAMPLING_TIME;

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/*
 * @brief  Override the default ADC Sample time depending on requirement
 * @param  ADC_SampleTime: The sample time value to be set.
 * This parameter can be one of the following values:
 * @arg ADC_SampleTime_1Cycles5: Sample time equal to 1.5 cycles
 * @arg ADC_SampleTime_7Cycles5: Sample time equal to 7.5 cycles
 * @arg ADC_SampleTime_13Cycles5: Sample time equal to 13.5 cycles
 * @arg ADC_SampleTime_28Cycles5: Sample time equal to 28.5 cycles
 * @arg ADC_SampleTime_41Cycles5: Sample time equal to 41.5 cycles
 * @arg ADC_SampleTime_55Cycles5: Sample time equal to 55.5 cycles
 * @arg ADC_SampleTime_71Cycles5: Sample time equal to 71.5 cycles
 * @arg ADC_SampleTime_239Cycles5: Sample time equal to 239.5 cycles
 * @retval None
 */
void HAL_ADC_Set_Sample_Time(uint8_t ADC_SampleTime)
{
  if(ADC_SampleTime < ADC_SampleTime_1Cycles5 || ADC_SampleTime > ADC_SampleTime_239Cycles5)
  {
    ADC_Sample_Time = ADC_SAMPLING_TIME;
  }
  else
  {
    ADC_Sample_Time = ADC_SampleTime;
  }
}

/*
 * @brief Read the analog value of a pin.
 * Should return a 16-bit value, 0-65536 (0 = LOW, 65536 = HIGH)
 * Note: ADC is 12-bit. Currently it returns 0-4096
 */
int32_t HAL_ADC_Read(uint16_t pin)
{

  int i = 0;

  if (PIN_MAP[pin].pin_mode != AN_INPUT)
  {
    HAL_GPIO_Save_Pin_Mode(pin);
    HAL_Pin_Mode(pin, AN_INPUT);
  }

  if (adcInitFirstTime == true)
  {
    HAL_ADC_DMA_Init();
    adcInitFirstTime = false;
  }

  if (adcChannelConfigured != PIN_MAP[pin].adc_channel)
  {
    // ADC1 regular channel configuration
    ADC_RegularChannelConfig(ADC1, PIN_MAP[pin].adc_channel, 1, ADC_Sample_Time);
    // ADC2 regular channel configuration
    ADC_RegularChannelConfig(ADC2, PIN_MAP[pin].adc_channel, 1, ADC_Sample_Time);
    // Save the ADC configured channel
    adcChannelConfigured = PIN_MAP[pin].adc_channel;
  }

  for(i = 0; i < ADC_DMA_BUFFERSIZE; i++)
  {
    ADC_DualConvertedValues[i] = 0;
  }

  // Reset the number of data units in the DMA1 Channel1 transfer
  DMA_SetCurrDataCounter(DMA1_Channel1, ADC_DMA_BUFFERSIZE);

  // Enable ADC2 external trigger conversion
  ADC_ExternalTrigConvCmd(ADC2, ENABLE);

  // Enable DMA1 Channel1
  DMA_Cmd(DMA1_Channel1, ENABLE);

  // Enable ADC1 DMA
  ADC_DMACmd(ADC1, ENABLE);

  // Start ADC1 Software Conversion
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);

  // Test on Channel 1 DMA1_FLAG_TC flag
  while(!DMA_GetFlagStatus(DMA1_FLAG_TC1));

  // Clear Channel 1 DMA1_FLAG_TC flag
  DMA_ClearFlag(DMA1_FLAG_TC1);

  // Disable ADC1 DMA
  ADC_DMACmd(ADC1, DISABLE);

  // Disable DMA1 Channel1
  DMA_Cmd(DMA1_Channel1, DISABLE);

  uint16_t ADC1_ConvertedValue = 0;
  uint16_t ADC2_ConvertedValue = 0;
  uint32_t ADC_SummatedValue = 0;
  uint16_t ADC_AveragedValue = 0;

  for(i = 0; i < ADC_DMA_BUFFERSIZE; i++)
  {
    // Retrieve the ADC2 converted value and add to ADC_SummatedValue
    ADC2_ConvertedValue = ADC_DualConvertedValues[i] >> 16;
    ADC_SummatedValue += ADC2_ConvertedValue;

    // Retrieve the ADC1 converted value and add to ADC_SummatedValue
    ADC1_ConvertedValue = ADC_DualConvertedValues[i] & 0xFFFF;
    ADC_SummatedValue += ADC1_ConvertedValue;
  }

  ADC_AveragedValue = (uint16_t)(ADC_SummatedValue / (ADC_DMA_BUFFERSIZE * 2));

  // Return ADC averaged value
  return ADC_AveragedValue;
}

/*
 * @brief Initialize the ADC peripheral.
 */
void HAL_ADC_DMA_Init()
{
  //Using "Dual Slow Interleaved ADC Mode" to achieve higher input impedance

  ADC_InitTypeDef ADC_InitStructure;
  DMA_InitTypeDef DMA_InitStructure;

  // ADCCLK = PCLK2/6 = 72/6 = 12MHz
  RCC_ADCCLKConfig(RCC_PCLK2_Div6);

  // Enable DMA1 clock
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

  // Enable ADC1 and ADC2 clock
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_ADC2, ENABLE);

  // DMA1 channel1 configuration
  DMA_DeInit(DMA1_Channel1);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)ADC1_DR_ADDRESS;
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&ADC_DualConvertedValues;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = ADC_DMA_BUFFERSIZE;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel1, &DMA_InitStructure);

  // ADC1 configuration
  ADC_InitStructure.ADC_Mode = ADC_Mode_SlowInterl;
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfChannel = 1;
  ADC_Init(ADC1, &ADC_InitStructure);

  // ADC2 configuration
  ADC_InitStructure.ADC_Mode = ADC_Mode_SlowInterl;
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfChannel = 1;
  ADC_Init(ADC2, &ADC_InitStructure);

  // Enable ADC1
  ADC_Cmd(ADC1, ENABLE);

  // Enable ADC1 reset calibration register
  ADC_ResetCalibration(ADC1);

  // Check the end of ADC1 reset calibration register
  while(ADC_GetResetCalibrationStatus(ADC1));

  // Start ADC1 calibration
  ADC_StartCalibration(ADC1);

  // Check the end of ADC1 calibration
  while(ADC_GetCalibrationStatus(ADC1));

  // Enable ADC2
  ADC_Cmd(ADC2, ENABLE);

  // Enable ADC2 reset calibration register
  ADC_ResetCalibration(ADC2);

  // Check the end of ADC2 reset calibration register
  while(ADC_GetResetCalibrationStatus(ADC2));

  // Start ADC2 calibration
  ADC_StartCalibration(ADC2);

  // Check the end of ADC2 calibration
  while(ADC_GetCalibrationStatus(ADC2));
}
