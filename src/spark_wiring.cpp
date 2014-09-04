/**
 ******************************************************************************
 * @file    spark_wiring.cpp
 * @author  Satish Nair, Zachary Crockett, Zach Supalla and Mohit Bhoite
 * @version V1.0.0
 * @date    13-March-2013
 * @brief   
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

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

#include "spark_wiring.h"
#include "spark_wiring_interrupts.h"
#include "spark_wiring_usartserial.h"
#include "spark_wiring_spi.h"
#include "spark_wiring_i2c.h"

/*
 * Globals
 */
__IO uint32_t ADC_DualConvertedValues[ADC_DMA_BUFFERSIZE];
uint8_t adcInitFirstTime = true;
uint8_t adcChannelConfigured = NONE;
static uint8_t ADC_Sample_Time = ADC_SAMPLING_TIME;

PinMode digitalPinModeSaved = (PinMode)NONE;

/*
 * Pin mapping
 */

STM32_Pin_Info PIN_MAP[TOTAL_PINS] =
{
/*
 * gpio_peripheral (GPIOA or GPIOB; not using GPIOC)
 * gpio_pin (0-15)
 * adc_channel (0-9 or NONE. Note we don't define the peripheral because our chip only has one)
 * timer_peripheral (TIM2 - TIM4, or NONE)
 * timer_ch (1-4, or NONE)
 * pin_mode (NONE by default, can be set to OUTPUT, INPUT, or other types)
 * timer_ccr (0 by default, store the CCR value for TIM interrupt use)
 * user_property (0 by default, user variable storage)
 */
  { GPIOB, GPIO_Pin_7, NONE, TIM4, TIM_Channel_2, (PinMode)NONE, 0, 0 },
  { GPIOB, GPIO_Pin_6, NONE, TIM4, TIM_Channel_1, (PinMode)NONE, 0, 0 },
  { GPIOB, GPIO_Pin_5, NONE, NULL, NONE, (PinMode)NONE, 0, 0 },
  { GPIOB, GPIO_Pin_4, NONE, NULL, NONE, (PinMode)NONE, 0, 0 },
  { GPIOB, GPIO_Pin_3, NONE, NULL, NONE, (PinMode)NONE, 0, 0 },
  { GPIOA, GPIO_Pin_15, NONE, NULL, NONE, (PinMode)NONE, 0, 0 },
  { GPIOA, GPIO_Pin_14, NONE, NULL, NONE, (PinMode)NONE, 0, 0 },
  { GPIOA, GPIO_Pin_13, NONE, NULL, NONE, (PinMode)NONE, 0, 0 },
  { GPIOA, GPIO_Pin_8, NONE, NULL, NONE, (PinMode)NONE, 0, 0 },
  { GPIOA, GPIO_Pin_9, NONE, NULL, NONE, (PinMode)NONE, 0, 0 },
  { GPIOA, GPIO_Pin_0, ADC_Channel_0, TIM2, TIM_Channel_1, (PinMode)NONE, 0, 0 },
  { GPIOA, GPIO_Pin_1, ADC_Channel_1, TIM2, TIM_Channel_2, (PinMode)NONE, 0, 0 },
  { GPIOA, GPIO_Pin_4, ADC_Channel_4, NULL, NONE, (PinMode)NONE, 0, 0 },
  { GPIOA, GPIO_Pin_5, ADC_Channel_5, NULL, NONE, (PinMode)NONE, 0, 0 },
  { GPIOA, GPIO_Pin_6, ADC_Channel_6, TIM3, TIM_Channel_1, (PinMode)NONE, 0, 0 },
  { GPIOA, GPIO_Pin_7, ADC_Channel_7, TIM3, TIM_Channel_2, (PinMode)NONE, 0, 0 },
  { GPIOB, GPIO_Pin_0, ADC_Channel_8, TIM3, TIM_Channel_3, (PinMode)NONE, 0, 0 },
  { GPIOB, GPIO_Pin_1, ADC_Channel_9, TIM3, TIM_Channel_4, (PinMode)NONE, 0, 0 },
  { GPIOA, GPIO_Pin_3, ADC_Channel_3, TIM2, TIM_Channel_4, (PinMode)NONE, 0, 0 },
  { GPIOA, GPIO_Pin_2, ADC_Channel_2, TIM2, TIM_Channel_3, (PinMode)NONE, 0, 0 },
  { GPIOA, GPIO_Pin_10, NONE, NULL, NONE, (PinMode)NONE, 0, 0 }
};

/*
 * @brief Set the mode of the pin to OUTPUT, INPUT, INPUT_PULLUP, or INPUT_PULLDOWN
 */
void pinMode(uint16_t pin, PinMode setMode)
{

	if (pin >= TOTAL_PINS || setMode == NONE )
	{
		return;
	}

	// SPI safety check
	if (SPI.isEnabled() == true && (pin == SCK || pin == MOSI || pin == MISO))
	{
		return;
	}

	// I2C safety check
	if (Wire.isEnabled() == true && (pin == SCL || pin == SDA))
	{
		return;
	}

	// Serial1 safety check
	if (Serial1.isEnabled() == true && (pin == RX || pin == TX))
	{
		return;
	}

	GPIO_TypeDef *gpio_port = PIN_MAP[pin].gpio_peripheral;
	uint16_t gpio_pin = PIN_MAP[pin].gpio_pin;

	GPIO_InitTypeDef GPIO_InitStructure;

	if (gpio_port == GPIOA )
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	}
	else if (gpio_port == GPIOB )
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	}

	GPIO_InitStructure.GPIO_Pin = gpio_pin;

	switch (setMode)
	{

	case OUTPUT:
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		PIN_MAP[pin].pin_mode = OUTPUT;
		break;

	case INPUT:
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		PIN_MAP[pin].pin_mode = INPUT;
		break;

	case INPUT_PULLUP:
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
		PIN_MAP[pin].pin_mode = INPUT_PULLUP;
		break;

	case INPUT_PULLDOWN:
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
		PIN_MAP[pin].pin_mode = INPUT_PULLDOWN;
		break;

	case AF_OUTPUT_PUSHPULL:	//Used internally for Alternate Function Output PushPull(TIM, UART, SPI etc)
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		PIN_MAP[pin].pin_mode = AF_OUTPUT_PUSHPULL;
		break;

	case AF_OUTPUT_DRAIN:		//Used internally for Alternate Function Output Drain(I2C etc)
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		PIN_MAP[pin].pin_mode = AF_OUTPUT_DRAIN;
		break;

	case AN_INPUT:				//Used internally for ADC Input
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
		PIN_MAP[pin].pin_mode = AN_INPUT;
		break;

	default:
		break;
	}

	GPIO_Init(gpio_port, &GPIO_InitStructure);
}

/*
 * @brief Sets a GPIO pin to HIGH or LOW.
 */
void digitalWrite(uint16_t pin, uint8_t value)
{
	if (pin >= TOTAL_PINS || PIN_MAP[pin].pin_mode == INPUT
	|| PIN_MAP[pin].pin_mode == INPUT_PULLUP || PIN_MAP[pin].pin_mode == INPUT_PULLDOWN
	|| PIN_MAP[pin].pin_mode == AN_INPUT || PIN_MAP[pin].pin_mode == NONE)
	{
		return;
	}

	// SPI safety check
	if (SPI.isEnabled() == true && (pin == SCK || pin == MOSI || pin == MISO))
	{
		return;
	}

	// I2C safety check
	if (Wire.isEnabled() == true && (pin == SCL || pin == SDA))
	{
		return;
	}

	// Serial1 safety check
	if (Serial1.isEnabled() == true && (pin == RX || pin == TX))
	{
		return;
	}

	//If the pin is used by analogWrite, we need to change the mode
	if(PIN_MAP[pin].pin_mode == AF_OUTPUT_PUSHPULL)
	{
		pinMode(pin, OUTPUT);
	}

	if (value == LOW)
	{
		PIN_MAP[pin].gpio_peripheral->BRR = PIN_MAP[pin].gpio_pin;
	} else
	{
		PIN_MAP[pin].gpio_peripheral->BSRR = PIN_MAP[pin].gpio_pin;
	}
}

/*
 * @brief Reads the value of a GPIO pin. Should return either 1 (HIGH) or 0 (LOW).
 */
int32_t digitalRead(uint16_t pin)
{
	if (pin >= TOTAL_PINS || PIN_MAP[pin].pin_mode == NONE
	|| PIN_MAP[pin].pin_mode == AF_OUTPUT_PUSHPULL || PIN_MAP[pin].pin_mode == AF_OUTPUT_DRAIN)
	{
		return LOW;
	}

	// SPI safety check
	if (SPI.isEnabled() == true && (pin == SCK || pin == MOSI || pin == MISO))
	{
		return LOW;
	}

	// I2C safety check
	if (Wire.isEnabled() == true && (pin == SCL || pin == SDA))
	{
		return LOW;
	}

	// Serial1 safety check
	if (Serial1.isEnabled() == true && (pin == RX || pin == TX))
	{
		return LOW;
	}

	if(PIN_MAP[pin].pin_mode == AN_INPUT)
	{
		if(digitalPinModeSaved == NONE)
		{
			return LOW;
		}
		else
		{
			//Restore the PinMode after calling analogRead on same pin earlier
			pinMode(pin, digitalPinModeSaved);
		}
	}

	if(PIN_MAP[pin].pin_mode == OUTPUT)
	{
		return GPIO_ReadOutputDataBit(PIN_MAP[pin].gpio_peripheral, PIN_MAP[pin].gpio_pin);
	}

	return GPIO_ReadInputDataBit(PIN_MAP[pin].gpio_peripheral, PIN_MAP[pin].gpio_pin);
}

/*
 * @brief Initialize the ADC peripheral.
 */
void ADC_DMA_Init()
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
void setADCSampleTime(uint8_t ADC_SampleTime)
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
int32_t analogRead(uint16_t pin)
{
	// Allow people to use 0-7 to define analog pins by checking to see if the values are too low.
	if (pin < FIRST_ANALOG_PIN)
	{
		pin = pin + FIRST_ANALOG_PIN;
	}

	// SPI safety check
	if (SPI.isEnabled() == true && (pin == SCK || pin == MOSI || pin == MISO))
	{
		return LOW;
	}

	// I2C safety check
	if (Wire.isEnabled() == true && (pin == SCL || pin == SDA))
	{
		return LOW;
	}

	// Serial1 safety check
	if (Serial1.isEnabled() == true && (pin == RX || pin == TX))
	{
		return LOW;
	}

	if (pin >= TOTAL_PINS || PIN_MAP[pin].adc_channel == NONE )
	{
		return LOW;
	}

	int i = 0;

	if (adcChannelConfigured != PIN_MAP[pin].adc_channel)
	{
		digitalPinModeSaved = PIN_MAP[pin].pin_mode;
		pinMode(pin, AN_INPUT);
	}

	if (adcInitFirstTime == true)
	{
		ADC_DMA_Init();
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

	for(i = 0 ; i < ADC_DMA_BUFFERSIZE ; i++)
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

	for(int i = 0 ; i < ADC_DMA_BUFFERSIZE ; i++)
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
 * @brief Should take an integer 0-255 and create a PWM signal with a duty cycle from 0-100%.
 * TIM_PWM_FREQ is set at 500 Hz
 */
void analogWrite(uint16_t pin, uint8_t value)
{

	if (pin >= TOTAL_PINS || PIN_MAP[pin].timer_peripheral == NULL)
	{
		return;
	}

	// SPI safety check
	if (SPI.isEnabled() == true && (pin == SCK || pin == MOSI || pin == MISO))
	{
		return;
	}

	// I2C safety check
	if (Wire.isEnabled() == true && (pin == SCL || pin == SDA))
	{
		return;
	}

	// Serial1 safety check
	if (Serial1.isEnabled() == true && (pin == RX || pin == TX))
	{
		return;
	}

	if(PIN_MAP[pin].pin_mode != OUTPUT && PIN_MAP[pin].pin_mode != AF_OUTPUT_PUSHPULL)
	{
		return;
	}

	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;

	//PWM Frequency : 500 Hz
	uint16_t TIM_Prescaler = (uint16_t)(SystemCoreClock / 24000000) - 1;//TIM Counter clock = 24MHz
	uint16_t TIM_ARR = (uint16_t)(24000000 / TIM_PWM_FREQ) - 1;

	// TIM Channel Duty Cycle(%) = (TIM_CCR / TIM_ARR + 1) * 100
	uint16_t TIM_CCR = (uint16_t)(value * (TIM_ARR + 1) / 255);

	// AFIO clock enable
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	pinMode(pin, AF_OUTPUT_PUSHPULL);

	// TIM clock enable
	if(PIN_MAP[pin].timer_peripheral == TIM2)
	{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	}
	else if(PIN_MAP[pin].timer_peripheral == TIM3)
	{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	}
	else if(PIN_MAP[pin].timer_peripheral == TIM4)
	{
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	}

	// Time base configuration
	TIM_TimeBaseStructure.TIM_Period = TIM_ARR;
	TIM_TimeBaseStructure.TIM_Prescaler = TIM_Prescaler;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(PIN_MAP[pin].timer_peripheral, &TIM_TimeBaseStructure);

	// PWM1 Mode configuration
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_Pulse = TIM_CCR;

	if(PIN_MAP[pin].timer_ch == TIM_Channel_1)
	{
		// PWM1 Mode configuration: Channel1
		TIM_OC1Init(PIN_MAP[pin].timer_peripheral, &TIM_OCInitStructure);
		TIM_OC1PreloadConfig(PIN_MAP[pin].timer_peripheral, TIM_OCPreload_Enable);
	}
	else if(PIN_MAP[pin].timer_ch == TIM_Channel_2)
	{
		// PWM1 Mode configuration: Channel2
		TIM_OC2Init(PIN_MAP[pin].timer_peripheral, &TIM_OCInitStructure);
		TIM_OC2PreloadConfig(PIN_MAP[pin].timer_peripheral, TIM_OCPreload_Enable);
	}
	else if(PIN_MAP[pin].timer_ch == TIM_Channel_3)
	{
		// PWM1 Mode configuration: Channel3
		TIM_OC3Init(PIN_MAP[pin].timer_peripheral, &TIM_OCInitStructure);
		TIM_OC3PreloadConfig(PIN_MAP[pin].timer_peripheral, TIM_OCPreload_Enable);
	}
	else if(PIN_MAP[pin].timer_ch == TIM_Channel_4)
	{
		// PWM1 Mode configuration: Channel4
		TIM_OC4Init(PIN_MAP[pin].timer_peripheral, &TIM_OCInitStructure);
		TIM_OC4PreloadConfig(PIN_MAP[pin].timer_peripheral, TIM_OCPreload_Enable);
	}

	TIM_ARRPreloadConfig(PIN_MAP[pin].timer_peripheral, ENABLE);

	// TIM enable counter
	TIM_Cmd(PIN_MAP[pin].timer_peripheral, ENABLE);
}

/*
 * TIMING
 */

/*
 * @brief Should return the number of milliseconds since the processor started up.
 * 		  This is useful for measuring the passage of time.
 * 		  For now, let's not worry about what happens when this overflows (which should happen after 49 days).
 * 		  At some point we'll have to figure that out, though.
 */
system_tick_t millis(void)
{
    return GetSystem1MsTick();
}

/*
 * @brief Should return the number of microseconds since the processor started up.
 */
unsigned long micros(void)
{
	return (DWT->CYCCNT / SYSTEM_US_TICKS);
}

/*
 * @brief This should block for a certain number of milliseconds and also execute spark_wlan_loop
 */
void delay(unsigned long ms)
{
#ifdef SPARK_WLAN_ENABLE
	volatile system_tick_t spark_loop_elapsed_millis = SPARK_LOOP_DELAY_MILLIS;
	spark_loop_total_millis += ms;
#endif

	volatile system_tick_t last_millis = GetSystem1MsTick();

	while (1)
	{
	        KICK_WDT();

		volatile system_tick_t current_millis = GetSystem1MsTick();
		volatile long elapsed_millis = current_millis - last_millis;

		//Check for wrapping
		if (elapsed_millis < 0)
		{
			elapsed_millis = last_millis + current_millis;
		}

		if (elapsed_millis >= ms)
		{
			break;
		}

#ifdef SPARK_WLAN_ENABLE
		if (!SPARK_WLAN_SETUP || SPARK_WLAN_SLEEP)
		{
			//Do not yield for SPARK_WLAN_Loop()
		}
		else if ((elapsed_millis >= spark_loop_elapsed_millis) || (spark_loop_total_millis >= SPARK_LOOP_DELAY_MILLIS))
		{
			spark_loop_elapsed_millis = elapsed_millis + SPARK_LOOP_DELAY_MILLIS;
			//spark_loop_total_millis is reset to 0 in SPARK_WLAN_Loop()
			do
			{
				//Run once if the above condition passes
				SPARK_WLAN_Loop();
			}
			while (SPARK_FLASH_UPDATE);//loop during OTA update
		}
#endif
	}
}

/*
 * @brief This should block for a certain number of microseconds.
 */
void delayMicroseconds(unsigned int us)
{
	Delay_Microsecond(us);
}

long map(long value, long fromStart, long fromEnd, long toStart, long toEnd)
{
    return (value - fromStart) * (toEnd - toStart) / (fromEnd - fromStart) + toStart;
}

uint8_t shiftIn(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder) {
	uint8_t value = 0;
	uint8_t i;

	for (i = 0; i < 8; ++i) {
		digitalWrite(clockPin, HIGH);
		if (bitOrder == LSBFIRST)
			value |= digitalRead(dataPin) << i;
		else
			value |= digitalRead(dataPin) << (7 - i);
		digitalWrite(clockPin, LOW);
	}
	return value;
}

void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val)
{
	uint8_t i;

	for (i = 0; i < 8; i++)  {
		if (bitOrder == LSBFIRST)
			digitalWrite(dataPin, !!(val & (1 << i)));
		else
			digitalWrite(dataPin, !!(val & (1 << (7 - i))));

		digitalWrite(clockPin, HIGH);
		digitalWrite(clockPin, LOW);
	}
}

