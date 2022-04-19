/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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

/* Includes ------------------------------------------------------------------*/
#include "application.h"
#include <cctype>

struct PinMapping {
    const char* name;
    pin_t pin;
};

#define PIN(p) {#p, p}

const PinMapping g_pinmap[] = {
    // Gen 3
#if HAL_PLATFORM_NRF52840
# if PLATFORM_ID == PLATFORM_TRACKER
    PIN(D0), PIN(D1), PIN(D2), PIN(D3), PIN(D4), PIN(D5), PIN(D6), PIN(D7), PIN(D8), PIN(D9)
# else // PLATFORM_ID == PLATFORM_TRACKER
    PIN(D0), PIN(D1), PIN(D2), PIN(D3), PIN(D4), PIN(D5), PIN(D6), PIN(D7),
    PIN(D8), PIN(D9), PIN(D10), PIN(D11), PIN(D12), PIN(D13), PIN(D14), PIN(D15),
    PIN(D16), PIN(D17), PIN(D18), PIN(D19), PIN(A0), PIN(A1), PIN(A2), PIN(A3),
    PIN(A4), PIN(A5), PIN(SCK), PIN(MISO), PIN(MOSI), PIN(SDA), PIN(SCL), PIN(TX),
    PIN(RX)
#  if PLATFORM_ID == PLATFORM_BSOM || PLATFORM_ID == PLATFORM_B5SOM || PLATFORM_ID == PLATFORM_ASOM
    ,
    PIN(D20), PIN(D21), PIN(D22), PIN(D23),
    PIN(A6), PIN(A7)
#  if PLATFORM_ID == PLATFORM_ASOM
    ,
    PIN(D24)
#  endif // PLATFORM_ID == PLATFORM_ASOM
#  endif // PLATFORM_ID == PLATFORM_BSOM || PLATFORM_ID == PLATFORM_B5SOM || PLATFORM_ID == PLATFORM_ASOM
# endif // PLATFORM_ID == PLATFORM_TRACKER

#else // HAL_PLATFORM_NRF52840
# error Unsupported platform
#endif
};

const size_t g_pin_count = sizeof(g_pinmap) / sizeof(*g_pinmap);

PRODUCT_ID(PLATFORM_ID);
PRODUCT_VERSION(3);

/* Function prototypes -------------------------------------------------------*/
int tinkerDigitalRead(String pin);
int tinkerDigitalWrite(String command);
int tinkerAnalogRead(String pin);
int tinkerAnalogWrite(String command);

#ifdef LOG_SERIAL1
Serial1LogHandler g_logSerial1(115200, LOG_LEVEL_ALL);
#endif // LOG_SERIAL1

#ifdef LOG_SERIAL
SerialLogHandler g_logSerial(LOG_LEVEL_ALL);
#endif // LOG_SERIAL

#if defined(HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL) && HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL
// Enable runtime PMIC / FuelGauge detection
STARTUP(System.enable(SYSTEM_FLAG_PM_DETECTION));
#endif // defined(HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL) && HAL_PLATFORM_POWER_MANAGEMENT_OPTIONAL

#ifndef PIN_INVALID
#define PIN_INVALID (0xff)
#endif // PIN_INVALID

SYSTEM_MODE(AUTOMATIC);

pin_t lookupPinByName(const String& name) {
    for (unsigned i = 0; i < g_pin_count; i++) {
        const auto& entry = g_pinmap[i];
        if (!strcmp(entry.name, name.c_str())) {
            return entry.pin;
        }
    }

    return PIN_INVALID;
}

/* This function is called once at start up ----------------------------------*/
void setup()
{
    //Setup the Tinker application here

    //Register all the Tinker functions
    Particle.function("digitalread", tinkerDigitalRead);
    Particle.function("digitalwrite", tinkerDigitalWrite);

    Particle.function("analogread", tinkerAnalogRead);
    Particle.function("analogwrite", tinkerAnalogWrite);
}

/* This function loops forever --------------------------------------------*/
void loop()
{
    //This will run in a loop
}

/*******************************************************************************
 * Function Name  : tinkerDigitalRead
 * Description    : Reads the digital value of a given pin
 * Input          : Pin
 * Output         : None.
 * Return         : Value of the pin (0 or 1) in INT type
                    Returns a negative number on failure
 *******************************************************************************/
int tinkerDigitalRead(String pinStr)
{
    pin_t pin = lookupPinByName(pinStr);
    if (pin != PIN_INVALID) {
        pinMode(pin, INPUT_PULLDOWN);
        return digitalRead(pin);
    }
    return -1;
}

/*******************************************************************************
 * Function Name  : tinkerDigitalWrite
 * Description    : Sets the specified pin HIGH or LOW
 * Input          : Pin and value
 * Output         : None.
 * Return         : 1 on success and a negative number on failure
 *******************************************************************************/
int tinkerDigitalWrite(String command)
{
    int separatorIndex = -1;
    for (unsigned i = 0; i < command.length(); ++i) {
        const char c = command.charAt(i);
        if (!std::isalnum((unsigned char)c)) {
            separatorIndex = i;
            break;
        }
    }
    if (separatorIndex <= 0) {
        return -1;
    }
    String pinStr = command.substring(0, separatorIndex);
    String pinState = command.substring(separatorIndex + 1);

    uint8_t state;
    if (pinState == "HIGH") {
        state = HIGH;
    } else if (pinState == "LOW") {
        state = LOW;
    } else {
        return -2;
    }

    pin_t pin = lookupPinByName(pinStr);

    if (pin != PIN_INVALID) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, state);
        return 1;
    }

    return -1;
}

/*******************************************************************************
 * Function Name  : tinkerAnalogRead
 * Description    : Reads the analog value of a pin
 * Input          : Pin
 * Output         : None.
 * Return         : Returns the analog value in INT type (0 to 4095)
                    Returns a negative number on failure
 *******************************************************************************/
int tinkerAnalogRead(String pinStr)
{
    pin_t pin = lookupPinByName(pinStr);
    if (pin != PIN_INVALID && HAL_Validate_Pin_Function(pin, PF_ADC) == PF_ADC) {
        return analogRead(pin);
    }
    return -1;
}

/*******************************************************************************
 * Function Name  : tinkerAnalogWrite
 * Description    : Writes an analog value (PWM) to the specified pin
 * Input          : Pin and Value (0 to 255)
 * Output         : None.
 * Return         : 1 on success and a negative number on failure
 *******************************************************************************/
int tinkerAnalogWrite(String command)
{
    int separatorIndex = -1;
    for (unsigned i = 0; i < command.length(); ++i) {
        const char c = command.charAt(i);
        if (!std::isalnum((unsigned char)c)) {
            separatorIndex = i;
            break;
        }
    }
    if (separatorIndex <= 0) {
        return -1;
    }
    String pinStr = command.substring(0, separatorIndex);
    String pinValue = command.substring(separatorIndex + 1);

    int value = pinValue.toInt();
    if (value < 0 || value > 255) {
        return -2;
    }

    pin_t pin = lookupPinByName(pinStr);
    if (pin != PIN_INVALID && (HAL_Validate_Pin_Function(pin, PF_DAC) == PF_DAC ||
        HAL_Validate_Pin_Function(pin, PF_TIMER) == PF_TIMER)) {
        pinMode(pin, OUTPUT);
        analogWrite(pin, value);
        return 1;
    }

    return -1;
}
