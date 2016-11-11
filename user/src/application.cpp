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

#ifndef UNIT_TEST

/* Includes ------------------------------------------------------------------*/
#include "application.h"
#include "stdarg.h"

PRODUCT_ID(PLATFORM_ID);
PRODUCT_VERSION(3);

/* Function prototypes -------------------------------------------------------*/
int tinkerDigitalRead(String pin);
int tinkerDigitalWrite(String command);
int tinkerAnalogRead(String pin);
int tinkerAnalogWrite(String command);

#if Wiring_WiFi
STARTUP(System.enable(SYSTEM_FLAG_WIFITESTER_OVER_SERIAL1));
#endif

SYSTEM_MODE(AUTOMATIC);

#endif // UNIT_TEST

// Pin config for various platforms
#if PLATFORM_ID == 31 || PLATFORM_ID == 3
#define D_PIN_COUNT 17
#define A_PIN_COUNT 8
#define GPIO_PIN_COUNT 28
#else
#define D_PIN_COUNT 8
#define A_PIN_COUNT 8
#define GPIO_PIN_COUNT 0
#endif

#if PLATFORM_ID == 31
#define HAS_ANALOG_PINS 0
#else
#define HAS_ANALOG_PINS 1
#endif

#if PLATFORM_ID == 10 || PLATFORM_ID == 3
#define B_PIN_COUNT 6
#define C_PIN_COUNT 6
#define B_ANALOG_PIN_COUNT 3
#else
#define B_PIN_COUNT 0
#define C_PIN_COUNT 0
#define B_ANALOG_PIN_COUNT 0
#endif

struct TinkerCommand {
    String pinType;
    int pinNumber;
    int value;
    bool hasNumber;
    bool hasValue;
    TinkerCommand() {
        pinType = "";
        pinNumber = 0;
        value = 0;
        hasNumber = false;
        hasValue = false;
    }
};

// Convert text like D7=HIGH into a structured command
TinkerCommand parseCommand(String command) {
    TinkerCommand result;

    // find the type of pin (D7 --> D, RX --> RX)
    unsigned pos = 0;
    for (; pos < command.length(); pos++) {
        char c = command.charAt(pos);
        if (c >= 'A' && c <= 'Z') {
        	result.pinType += c;
        } else {
        	break;
        }
    }

    // find the pin number (D7 --> 7)
    String num = "";
    for (; pos < command.length(); pos++) {
        char c = command.charAt(pos);
        if (c >= '0' && c <= '9') {
        	num += c;
        } else {
        	break;
        }
    }
    result.hasNumber = num.length() > 0;
    result.pinNumber = num.toInt();

    // Convert the value to integer (A0=50 --> 50, D7=HIGH --> 1)

    String value = command.substring(pos + 1); // + 1 to skip the equal sign
    result.hasValue = value.length() > 0;
    if (value.equals("HIGH")) {
        result.value = 1;
    } else if (value.equals("LOW")) {
        result.value = 0;
    } else {
        result.value = value.toInt();
    }

    return result;
}

#ifndef UNIT_TEST

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

#endif // UNIT_TEST

/*******************************************************************************
 * Function Name  : tinkerDigitalRead
 * Description    : Reads the digital value of a given pin
 * Input          : Pin
 * Output         : None.
 * Return         : Value of the pin (0 or 1) in INT type
                    Returns a negative number on failure
 *******************************************************************************/
int tinkerDigitalRead(String command)
{
    TinkerCommand c = parseCommand(command);

    if (!c.hasNumber) {
        return -2;
    }

    if (c.pinType.equals("D"))
    {
        if (c.pinNumber >= D_PIN_COUNT) {
            return -1;
        }
        pinMode(D0 + c.pinNumber, INPUT_PULLDOWN);
        return digitalRead(D0 + c.pinNumber);
    }
    else if (c.pinType.equals("A"))
    {
        if (c.pinNumber >= A_PIN_COUNT) {
            return -1;
        }
        pinMode(A0 + c.pinNumber, INPUT_PULLDOWN);
        return digitalRead(A0 + c.pinNumber);
    }
#if B_PIN_COUNT > 0
    else if(c.pinType.equals("B"))
    {
        if (c.pinNumber >= B_PIN_COUNT) {
            return -3;
        }
        pinMode(B0 + c.pinNumber, INPUT_PULLDOWN);
        return digitalRead(B0 + c.pinNumber);
    }
#endif
#if C_PIN_COUNT > 0
    else if(c.pinType.equals("C"))
    {
        if (c.pinNumber >= C_PIN_COUNT) {
            return -4;
        }
        pinMode(C0 + c.pinNumber, INPUT_PULLDOWN);
        return digitalRead(C0 + c.pinNumber);
    }
#endif
#if GPIO_PIN_COUNT > 0
    else if(c.pinType.equals("GPIO"))
    {
        if (c.pinNumber >= GPIO_PIN_COUNT) {
            return -5;
        }
        pinMode(GPIO0 + c.pinNumber, INPUT_PULLDOWN);
        return digitalRead(GPIO0 + c.pinNumber);
    }
#endif
    return -2;
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
    TinkerCommand c = parseCommand(command);

    if (!c.hasNumber || !c.hasValue) {
        return -2;
    }

    if (c.value != 0 && c.value != 1) {
        return -2;
    }

    if (c.pinType.equals("D"))
    {
        if (c.pinNumber >= D_PIN_COUNT) {
            return -1;
        }
        pinMode(D0 + c.pinNumber, OUTPUT);
        digitalWrite(D0 + c.pinNumber, c.value);
        return 1;
    }
    else if (c.pinType.equals("A"))
    {
        if (c.pinNumber >= A_PIN_COUNT) {
            return -1;
        }
        pinMode(A0 + c.pinNumber, OUTPUT);
        digitalWrite(A0 + c.pinNumber, c.value);
        return 1;
    }
#if B_PIN_COUNT > 0
    else if (c.pinType.equals("B"))
    {
        if (c.pinNumber >= B_PIN_COUNT) {
            return -4;
        }
        pinMode(B0 + c.pinNumber, OUTPUT);
        digitalWrite(B0 + c.pinNumber, c.value);
        return 1;
    }
#endif
#if C_PIN_COUNT > 0
    else if (c.pinType.equals("C"))
    {
        if (c.pinNumber >= C_PIN_COUNT) {
            return -5;
        }
        pinMode(C0 + c.pinNumber, OUTPUT);
        digitalWrite(C0 + c.pinNumber, c.value);
        return 1;
    }
#endif
#if GPIO_PIN_COUNT > 0
    else if(c.pinType.equals("GPIO"))
    {
        if (c.pinNumber >= GPIO_PIN_COUNT) {
            return -6;
        }
        pinMode(GPIO0 + c.pinNumber, OUTPUT);
        digitalWrite(GPIO0 + c.pinNumber, c.value);
        return 1;
    }
#endif
    return -3;
}

/*******************************************************************************
 * Function Name  : tinkerAnalogRead
 * Description    : Reads the analog value of a pin
 * Input          : Pin
 * Output         : None.
 * Return         : Returns the analog value in INT type (0 to 4095)
                    Returns a negative number on failure
 *******************************************************************************/
int tinkerAnalogRead(String command)
{
#if HAS_ANALOG_PINS == 1
    TinkerCommand c = parseCommand(command);

    if (!c.hasNumber) {
        return -2;
    }

    if (c.pinType.equals("D")) {
        return -3;
    }
    else if (c.pinType.equals("A")) {
        if (c.pinNumber >= A_PIN_COUNT) {
            return -1;
        }
        return analogRead(A0 + c.pinNumber);
    }
#if B_PIN_COUNT > 0
    else if (c.pinType.equals("B")) {
        if (c.pinNumber >= B_PIN_COUNT) {
            return -3;
        }
        return analogRead(B0 + c.pinNumber);
    }
#endif

#endif
    return -2;
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
    TinkerCommand c = parseCommand(command);

    if (!c.hasValue) {
        return -2;
    }

    if (c.pinType.equals("TX")) {
        pinMode(TX, OUTPUT);
        analogWrite(TX, c.value);
        return 1;
    }
    else if (c.pinType.equals("RX")) {
        pinMode(RX, OUTPUT);
        analogWrite(RX, c.value);
        return 1;
    }

    if (!c.hasNumber) {
        return -2;
    }

    if (c.pinType.equals("D")) {
        if (c.pinNumber >= D_PIN_COUNT) {
            return -1;
        }
        pinMode(D0 + c.pinNumber, OUTPUT);
        analogWrite(D0 + c.pinNumber, c.value);
        return 1;
    }
    else if (c.pinType.equals("A")) {
        if (c.pinNumber >= A_PIN_COUNT) {
            return -1;
        }
        pinMode(A0 + c.pinNumber, OUTPUT);
        analogWrite(A0 + c.pinNumber, c.value);
        return 1;
    }
#if B_PIN_COUNT > 0
    else if (c.pinType.equals("B"))
    {
        if (c.pinNumber >= B_PIN_COUNT) {
            return -3;
        }
        pinMode(B0 + c.pinNumber, OUTPUT);
        analogWrite(B0 + c.pinNumber, c.value);
        return 1;
    }
#endif
#if C_PIN_COUNT > 0
    else if (c.pinType.equals("C"))
    {
        if (c.pinNumber >= C_PIN_COUNT) {
            return -4;
        }
        pinMode(C0 + c.pinNumber, OUTPUT);
        analogWrite(C0 + c.pinNumber, c.value);
        return 1;
    }
#endif
#if GPIO_PIN_COUNT > 0
    else if(c.pinType.equals("GPIO"))
    {
        if (c.pinNumber >= GPIO_PIN_COUNT) {
            return -5;
        }
        pinMode(GPIO0 + c.pinNumber, OUTPUT);
        analogWrite(GPIO0 + c.pinNumber, c.value);
        return 1;
    }
#endif
    return -2;
}
