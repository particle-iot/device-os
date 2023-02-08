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
#include "enumclass.h"

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

  hal_gpio_mode(pin, setMode);
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
  return hal_gpio_get_mode(pin);
}


#if HAL_PLATFORM_GEN == 3
/*
 * @brief Set the drive strength of the pin for OUTPUT modes
 */
int pinSetDriveStrength(hal_pin_t pin, DriveStrength drive)
{
  if (pin >= TOTAL_PINS) {
    return SYSTEM_ERROR_INVALID_ARGUMENT;
  }

  // Safety check
  if (!pinAvailable(pin)) {
    return SYSTEM_ERROR_INVALID_STATE;
  }

  auto mode = getPinMode(pin);

  if (mode != OUTPUT && mode != OUTPUT_OPEN_DRAIN) {
    return SYSTEM_ERROR_INVALID_STATE;
  }

  const hal_gpio_config_t conf = {
    .size = sizeof(hal_gpio_config_t),
    .version = HAL_GPIO_VERSION,
    .mode = mode,
    .set_value = 0,
    .value = 0,
    .drive_strength = particle::to_underlying(drive)
  };
  return hal_gpio_configure(pin, &conf, nullptr);
}
#endif // HAL_PLATFORM_GEN == 3

/*
 * @brief Perform safety check on desired pin to see if it's already
 * being used.  Return 0 if used, otherwise return 1 if available.
 */
bool pinAvailable(uint16_t pin) {
  if (pin >= TOTAL_PINS) {
    return false;
  }

  // SPI safety check
#ifndef SPARK_WIRING_NO_SPI
  if((pin == SCK || pin == MOSI || pin == MISO) && hal_spi_is_enabled(SPI.interface()) == true)
  {
    return false; // 'pin' is used
  }
#endif
  // I2C safety check
#ifndef SPARK_WIRING_NO_I2C
  if((pin == SCL || pin == SDA) && hal_i2c_is_enabled(Wire.interface(), nullptr) == true)
  {
    return false; // 'pin' is used
  }
#endif
#ifndef SPARK_WIRING_NO_USART_SERIAL
  // Serial1 safety check
  if((pin == RX || pin == TX) && hal_usart_is_enabled(Serial1.interface()) == true)
  {
    return false; // 'pin' is used
  }
#endif

  return true; // 'pin' is available
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
void digitalWrite(hal_pin_t pin, uint8_t value)
{
    PinMode mode = hal_gpio_get_mode(pin);
    if (mode==PIN_MODE_NONE || is_input_mode(mode))
        return;
  // Safety check
  if( !pinAvailable(pin) ) {
    return;
  }

  hal_gpio_write(pin, value);
}

inline bool is_af_output_mode(PinMode mode) {
    return mode == AF_OUTPUT_PUSHPULL ||
           mode == AF_OUTPUT_DRAIN;
}

/*
 * @brief Reads the value of a GPIO pin. Should return either 1 (HIGH) or 0 (LOW).
 */
int32_t digitalRead(hal_pin_t pin)
{
    PinMode mode = hal_gpio_get_mode(pin);
    if (is_af_output_mode(mode))
        return LOW;

    // Safety check
    if( !pinAvailable(pin) ) {
      return LOW;
    }

    return hal_gpio_read(pin);
}

/*
 * @brief Read the analog value of a pin.
 * Should return a 16-bit value, 0-65536 (0 = LOW, 65536 = HIGH)
 * Note: ADC is 12-bit. Currently it returns 0-4095
 */
int32_t analogRead(hal_pin_t pin)
{
#if !HAL_PLATFORM_RTL872X
  // Allow people to use 0-7 to define analog pins by checking to see if the values are too low.
#if defined(FIRST_ANALOG_PIN) && FIRST_ANALOG_PIN > 0
  if(pin < FIRST_ANALOG_PIN)
  {
    pin = pin + FIRST_ANALOG_PIN;
  }
#endif // defined(FIRST_ANALOG_PIN) && FIRST_ANALOG_PIN > 0
#endif

  // Safety check
  if( !pinAvailable(pin) ) {
    return LOW;
  }

  if(hal_pin_validate_function(pin, PF_ADC)!=PF_ADC)
  {
    return LOW;
  }

  return hal_adc_read(pin);
}

int analogCalibrate() {
  return hal_adc_calibrate(0, nullptr);
}

/*
 * @brief Should take an integer 0-255 and create a 500Hz PWM signal with a duty cycle from 0-100%.
 */
void analogWrite(hal_pin_t pin, uint32_t value)
{
    // Safety check
    if (!pinAvailable(pin))
    {
        return;
    }

    if (hal_pin_validate_function(pin, PF_DAC) == PF_DAC)
    {
        HAL_DAC_Write(pin, value);
    }
    else if (hal_pin_validate_function(pin, PF_TIMER) == PF_TIMER)
    {
        PinMode mode = hal_gpio_get_mode(pin);

        if (mode != OUTPUT && mode != AF_OUTPUT_PUSHPULL)
        {
            return;
        }

        hal_pwm_write_ext(pin, value);
    }
}


/*
 * @brief Should take an integer 0-255 and create a PWM signal with a duty cycle from 0-100%
 * and frequency from 1 to 65535 Hz.
 */
void analogWrite(hal_pin_t pin, uint32_t value, uint32_t pwm_frequency)
{
    // Safety check
    if (!pinAvailable(pin))
    {
        return;
    }

    if (hal_pin_validate_function(pin, PF_TIMER) == PF_TIMER)
    {
        PinMode mode = hal_gpio_get_mode(pin);

        if (mode != OUTPUT && mode != AF_OUTPUT_PUSHPULL)
        {
            return;
        }

        hal_pwm_write_with_frequency_ext(pin, value, pwm_frequency);
    }
}

uint8_t analogWriteResolution(hal_pin_t pin, uint8_t value)
{
  // Safety check
  if (!pinAvailable(pin))
  {
      return 0;
  }

  if (hal_pin_validate_function(pin, PF_DAC) == PF_DAC)
  {
    HAL_DAC_Set_Resolution(pin, value);
    return HAL_DAC_Get_Resolution(pin);
  }
  else if (hal_pin_validate_function(pin, PF_TIMER) == PF_TIMER)
  {
    hal_pwm_set_resolution(pin, value);
    return hal_pwm_get_resolution(pin);
  }
  

  return 0;
}

uint8_t analogWriteResolution(hal_pin_t pin)
{
  // Safety check
  if (!pinAvailable(pin))
  {
      return 0;
  }

  if (hal_pin_validate_function(pin, PF_DAC) == PF_DAC)
  {
    return HAL_DAC_Get_Resolution(pin);
  }
  else if (hal_pin_validate_function(pin, PF_TIMER) == PF_TIMER)
  {
    return hal_pwm_get_resolution(pin);
  }

  return 0;
}

uint32_t analogWriteMaxFrequency(hal_pin_t pin)
{
  // Safety check
  if (!pinAvailable(pin))
  {
      return 0;
  }

  return hal_pwm_get_max_frequency(pin);
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
uint32_t pulseIn(hal_pin_t pin, uint16_t value) {

    // NO SAFETY CHECKS!!! WILD WILD WEST!!!

    return hal_gpio_pulse_in(pin, value);
}

void setDACBufferred(hal_pin_t pin, uint8_t state) {
  HAL_DAC_Enable_Buffer(pin, state);
}

int analogSetReference(AdcReference reference) {
  return hal_adc_set_reference(particle::to_underlying(reference), nullptr);
}

AdcReference analogGetReference(void) {
  int result = hal_adc_get_reference(nullptr);
  if (result >= SYSTEM_ERROR_NONE) {
    return static_cast<AdcReference>(result);
  } 
  return AdcReference::DEFAULT;
}
