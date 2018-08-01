/*
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "application.h"
#include "stdarg.h"
#include "preprocessor.h"

PRODUCT_ID(PLATFORM_ID);
PRODUCT_VERSION(3);

/* Function prototypes -------------------------------------------------------*/
int tinkerDigitalRead(String pin);
int tinkerDigitalWrite(String command);
int tinkerAnalogRead(String pin);
int tinkerAnalogWrite(String command);

STARTUP(System.enable(SYSTEM_FLAG_WIFITESTER_OVER_SERIAL1));
STARTUP(System.enableFeature(FEATURE_WIFITESTER));

SYSTEM_MODE(AUTOMATIC);

#define PIN_MAPPING_ENTRY(pin) {_PP_STR(pin), pin}

struct PinMapping {
    const char* name;
    pin_t pin;
};

PinMapping pinmap[] = {
#if PLATFORM_ID == PLATFORM_XENON
    PIN_MAPPING_ENTRY(D0),
    PIN_MAPPING_ENTRY(D1),
    PIN_MAPPING_ENTRY(D2),
    PIN_MAPPING_ENTRY(D3),
    PIN_MAPPING_ENTRY(D4),
    PIN_MAPPING_ENTRY(D5),
    PIN_MAPPING_ENTRY(D6),
    PIN_MAPPING_ENTRY(D7),
    PIN_MAPPING_ENTRY(D8),
    PIN_MAPPING_ENTRY(D9),
    PIN_MAPPING_ENTRY(D10),
    PIN_MAPPING_ENTRY(D11),
    PIN_MAPPING_ENTRY(D12),
    PIN_MAPPING_ENTRY(D13),
    PIN_MAPPING_ENTRY(D14),
    PIN_MAPPING_ENTRY(D15),
    PIN_MAPPING_ENTRY(D16),
    PIN_MAPPING_ENTRY(D17),
    PIN_MAPPING_ENTRY(D18),
    PIN_MAPPING_ENTRY(D19),
    PIN_MAPPING_ENTRY(A0),
    PIN_MAPPING_ENTRY(A1),
    PIN_MAPPING_ENTRY(A2),
    PIN_MAPPING_ENTRY(A3),
    PIN_MAPPING_ENTRY(A4),
    PIN_MAPPING_ENTRY(A5),
    PIN_MAPPING_ENTRY(SCK),
    PIN_MAPPING_ENTRY(MISO),
    PIN_MAPPING_ENTRY(MOSI),
    PIN_MAPPING_ENTRY(SDA),
    PIN_MAPPING_ENTRY(SCL),
    PIN_MAPPING_ENTRY(TX),
    PIN_MAPPING_ENTRY(RX)
#endif /* PLATFORM_ID == PLATFORM_XENON */
};

pin_t lookupPinByName(const String& name) {
    for (unsigned i = 0; i < sizeof(pinmap) / sizeof(pinmap[0]); i++) {
        const auto& entry = pinmap[i];
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
    int indexOfComma = command.indexOf(',');
    if (indexOfComma <= 0) {
        return -1;
    }
    String pinStr = command.substring(0, indexOfComma);
    String pinState = command.substring(indexOfComma + 1);

    bool state;
    if (pinState == "HIGH") {
        state = true;
    } else if (pinState == "LOW") {
        state = false;
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
    if (pin != PIN_INVALID) {
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
    int indexOfComma = command.indexOf(',');
    if (indexOfComma <= 0) {
        return -1;
    }
    String pinStr = command.substring(0, indexOfComma);
    String pinValue = command.substring(indexOfComma + 1);

    int value = pinValue.toInt();
    if (value < 0 || value > 255) {
        return -2;
    }

    pin_t pin = lookupPinByName(pinStr);
    if (pin != PIN_INVALID) {
        pinMode(pin, OUTPUT);
        analogWrite(pin, value);
        return 1;
    }

    return -1;
}
