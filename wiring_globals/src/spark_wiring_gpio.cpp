/**
 ******************************************************************************
 * @file    spark_wiring.cpp
 * @authors Satish Nair, Zachary Crockett, Zach Supalla, Mohit Bhoite and
 *          Brett Walach
 * @version V1.0.0
 * @date    13-March-2013
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
#include "spark_wiring.h"
#include "spark_wiring_interrupts.h"
#include "spark_wiring_usartserial.h"
#include "spark_wiring_spi.h"
#include "spark_wiring_i2c.h"
#include "watchdog_hal.h"
#include "delay_hal.h"
#include "pinmap_hal.h"
#include "system_task.h"

/*
 * @brief Set the mode of the pin to OUTPUT, INPUT, INPUT_PULLUP,
 * or INPUT_PULLDOWN
 */
void pinMode(uint16_t pin, PinMode setMode)
{

  if(pin >= TOTAL_PINS || setMode == PIN_MODE_NONE )
  {
    return;
  }

  // Safety check
  if( !pinAvailable(pin) ) {
    return;
  }

  HAL_Pin_Mode(pin, setMode);
}

/*
 * @brief Returns the mode of the selected pin as type PinMode
 *
 * OUTPUT = 0
 * INPUT = 1
 * INPUT_PULLUP = 2
 * INPUT_PULLDOWN = 3
 * AF_OUTPUT_PUSHPULL = 4
 * AF_OUTPUT_DRAIN = 5
 * AN_INPUT = 6
 * AN_OUTPUT = 7
 * PIN_MODE_NONE = 255
 */
PinMode getPinMode(uint16_t pin)
{
  return HAL_Get_Pin_Mode(pin);
}

/*
 * @brief Perform safety check on desired pin to see if it's already
 * being used.  Return 0 if used, otherwise return 1 if available.
 */
bool pinAvailable(uint16_t pin) {

  // SPI safety check
#ifndef SPARK_WIRING_NO_SPI
  if(SPI.isEnabled() == true && (pin == SCK || pin == MOSI || pin == MISO))
  {
    return 0; // 'pin' is used
  }
#endif
  // I2C safety check
#ifndef SPARK_WIRING_NO_I2C
  if(Wire.isEnabled() == true && (pin == SCL || pin == SDA))
  {
    return 0; // 'pin' is used
  }
#endif
#ifndef SPARK_WIRING_NO_USART_SERIAL
  // Serial1 safety check
  if(Serial1.isEnabled() == true && (pin == RX || pin == TX))
  {
    return 0; // 'pin' is used
  }
#endif

  if (pin >= TOTAL_PINS)
    return 0;
  else
    return 1; // 'pin' is available
}

inline bool is_input_mode(PinMode mode) {
    return  mode == INPUT ||
            mode == INPUT_PULLUP ||
            mode == INPUT_PULLDOWN ||
            mode == AN_INPUT;
}

/*
 * @brief Sets a GPIO pin to HIGH or LOW.
 */
void digitalWrite(pin_t pin, uint8_t value)
{
    PinMode mode = HAL_Get_Pin_Mode(pin);
    if (mode==PIN_MODE_NONE || is_input_mode(mode))
        return;
  // Safety check
  if( !pinAvailable(pin) ) {
    return;
  }

  HAL_GPIO_Write(pin, value);
}

inline bool is_af_output_mode(PinMode mode) {
    return mode == AF_OUTPUT_PUSHPULL ||
           mode == AF_OUTPUT_DRAIN;
}

/*
 * @brief Reads the value of a GPIO pin. Should return either 1 (HIGH) or 0 (LOW).
 */
int32_t digitalRead(pin_t pin)
{
    PinMode mode = HAL_Get_Pin_Mode(pin);
    if (is_af_output_mode(mode))
        return LOW;

    // Safety check
    if( !pinAvailable(pin) ) {
      return LOW;
    }

    return HAL_GPIO_Read(pin);
}

/*
 * @brief Set the ADC reference to either VDD / 4 (VDD4) or the internal 0.6v (INTERNAL)
 */

void analogReference(vref_e v){
    HAL_ADC_Set_VREF(v);
}

/*
 * @brief Read the analog value of a pin.
 * Should return a 16-bit value, 0-65536 (0 = LOW, 65536 = HIGH)
 * Note: ADC is 12-bit. Currently it returns 0-4095
 */
int32_t analogRead(pin_t pin)
{
  // Allow people to use 0-7 to define analog pins by checking to see if the values are too low.
  if(pin < FIRST_ANALOG_PIN)
  {
    pin = pin + FIRST_ANALOG_PIN;
  }

  // Safety check
  if( !pinAvailable(pin) ) {
    return LOW;
  }

  if(HAL_Validate_Pin_Function(pin, PF_ADC)!=PF_ADC)
  {
    return LOW;
  }

  return HAL_ADC_Read(pin);
}


/*
 * @brief Should take an integer 0-255 and create a 500Hz PWM signal with a duty cycle from 0-100%.
 * On Photon, DAC1 and DAC2 act as true analog outputs(values: 0 to 4095) using onchip DAC peripheral
 */
void analogWrite(pin_t pin, uint32_t value)
{
    // Safety check
    if (!pinAvailable(pin))
    {
        return;
    }

    if (HAL_Validate_Pin_Function(pin, PF_DAC) == PF_DAC)
    {
        HAL_DAC_Write(pin, value);
    }
    else if (HAL_Validate_Pin_Function(pin, PF_TIMER) == PF_TIMER)
    {
        PinMode mode = HAL_Get_Pin_Mode(pin);

        if (mode != OUTPUT && mode != AF_OUTPUT_PUSHPULL)
        {
            return;
        }

        HAL_PWM_Write_Ext(pin, value);
    }
}


/*
 * @brief Should take an integer 0-255 and create a PWM signal with a duty cycle from 0-100%
 * and frequency from 1 to 65535 Hz.
 */
void analogWrite(pin_t pin, uint32_t value, uint32_t pwm_frequency)
{
    // Safety check
    if (!pinAvailable(pin))
    {
        return;
    }

    if (HAL_Validate_Pin_Function(pin, PF_TIMER) == PF_TIMER)
    {
        PinMode mode = HAL_Get_Pin_Mode(pin);

        if (mode != OUTPUT && mode != AF_OUTPUT_PUSHPULL)
        {
            return;
        }

        HAL_PWM_Write_With_Frequency_Ext(pin, value, pwm_frequency);
    }
}

uint8_t analogWriteResolution(pin_t pin, uint8_t value)
{
  // Safety check
  if (!pinAvailable(pin))
  {
      return 0;
  }

  if (HAL_Validate_Pin_Function(pin, PF_DAC) == PF_DAC)
  {
    HAL_DAC_Set_Resolution(pin, value);
    return HAL_DAC_Get_Resolution(pin);
  }
  else if (HAL_Validate_Pin_Function(pin, PF_TIMER) == PF_TIMER)
  {
    HAL_PWM_Set_Resolution(pin, value);
    return HAL_PWM_Get_Resolution(pin);
  }


  return 0;
}

uint8_t analogWriteResolution(pin_t pin)
{
  // Safety check
  if (!pinAvailable(pin))
  {
      return 0;
  }

  if (HAL_Validate_Pin_Function(pin, PF_DAC) == PF_DAC)
  {
    return HAL_DAC_Get_Resolution(pin);
  }
  else if (HAL_Validate_Pin_Function(pin, PF_TIMER) == PF_TIMER)
  {
    return HAL_PWM_Get_Resolution(pin);
  }

  return 0;
}

uint32_t analogWriteMaxFrequency(pin_t pin)
{
  // Safety check
  if (!pinAvailable(pin))
  {
      return 0;
  }

  return HAL_PWM_Get_Max_Frequency(pin);
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

/*
 * @brief   blocking call to measure a high or low pulse
 * @returns uint32_t pulse width in microseconds up to 3 seconds,
 *          returns 0 on 3 second timeout error, or invalid pin.
 */
uint32_t pulseIn(pin_t pin, uint16_t value) {

    // NO SAFETY CHECKS!!! WILD WILD WEST!!!

    return HAL_Pulse_In(pin, value);
}

void setDACBufferred(pin_t pin, uint8_t state) {
  HAL_DAC_Enable_Buffer(pin, state);
}
