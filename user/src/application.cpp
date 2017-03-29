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
#if PLATFORM_ID == 31
// Decrease CPU usage on Raspberry Pi
SYSTEM_LOOP_DELAY(50);
#endif

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
const pin_t D_PINS[] = {
    D0, D1, D2, D3, D4, D5, D6, D7,
#if PLATFORM_ID == 31 || UNIT_TEST // Raspberry Pi (31)
    D8, D9, D10, D11, D12, D13, D14, D15, D16
#endif
};
const pin_t A_PINS[] = {
    A0, A1, A2, A3, A4, A5, A6, A7
};
const pin_t B_PINS[] = {
#if PLATFORM_ID == 10 || UNIT_TEST // Electron (10)
    B0, B1, B2, B3, B4, B5
#endif
};
const pin_t C_PINS[] = {
#if PLATFORM_ID == 10 || UNIT_TEST // Electron (10)
    C0, C1, C2, C3, C4, C5
#endif
};
const pin_t GPIO_PINS[] = {
#if PLATFORM_ID == 31 || UNIT_TEST // Raspberry Pi (31)
    GPIO0, GPIO1, GPIO2, GPIO3, GPIO4, GPIO5, GPIO6, GPIO7, GPIO8, GPIO9,
    GPIO10, GPIO11, GPIO12, GPIO13, GPIO14, GPIO15, GPIO16, GPIO17, GPIO18, GPIO19,
    GPIO20, GPIO21, GPIO22, GPIO23, GPIO24, GPIO25, GPIO26, GPIO27
#endif
};

#if PLATFORM_ID == 31 // Raspberry Pi (31)
#define HAS_ANALOG_PINS 0
#else
#define HAS_ANALOG_PINS 1
#endif

#define countof(arr) (sizeof((arr))/sizeof((arr)[0]))

struct TinkerCommand {
    String pinType;
    unsigned pinNumber;
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
        if (c.pinNumber >= countof(D_PINS)) {
            return -1;
        }
        pinMode(D_PINS[c.pinNumber], INPUT_PULLDOWN);
        return digitalRead(D_PINS[c.pinNumber]);
    }
    else if (c.pinType.equals("A"))
    {
        if (c.pinNumber >= countof(A_PINS)) {
            return -1;
        }
        pinMode(A_PINS[c.pinNumber], INPUT_PULLDOWN);
        return digitalRead(A_PINS[c.pinNumber]);
    }
    else if(c.pinType.equals("B"))
    {
        if (c.pinNumber >= countof(B_PINS)) {
            return -1;
        }
        pinMode(B_PINS[c.pinNumber], INPUT_PULLDOWN);
        return digitalRead(B_PINS[c.pinNumber]);
    }
    else if(c.pinType.equals("C"))
    {
        if (c.pinNumber >= countof(C_PINS)) {
            return -1;
        }
        pinMode(C_PINS[c.pinNumber], INPUT_PULLDOWN);
        return digitalRead(C_PINS[c.pinNumber]);
    }
    else if(c.pinType.equals("GPIO"))
    {
        if (c.pinNumber >= countof(GPIO_PINS)) {
            return -1;
        }
        pinMode(GPIO_PINS[c.pinNumber], INPUT_PULLDOWN);
        return digitalRead(GPIO_PINS[c.pinNumber]);
    }
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
        if (c.pinNumber >= countof(D_PINS)) {
            return -1;
        }
        pinMode(D_PINS[c.pinNumber], OUTPUT);
        digitalWrite(D_PINS[c.pinNumber], c.value);
        return 1;
    }
    else if (c.pinType.equals("A"))
    {
        if (c.pinNumber >= countof(A_PINS)) {
            return -1;
        }
        pinMode(A_PINS[c.pinNumber], OUTPUT);
        digitalWrite(A_PINS[c.pinNumber], c.value);
        return 1;
    }
    else if (c.pinType.equals("B"))
    {
        if (c.pinNumber >= countof(B_PINS)) {
            return -1;
        }
        pinMode(B_PINS[c.pinNumber], OUTPUT);
        digitalWrite(B_PINS[c.pinNumber], c.value);
        return 1;
    }
    else if (c.pinType.equals("C"))
    {
        if (c.pinNumber >= countof(C_PINS)) {
            return -1;
        }
        pinMode(C_PINS[c.pinNumber], OUTPUT);
        digitalWrite(C_PINS[c.pinNumber], c.value);
        return 1;
    }
    else if(c.pinType.equals("GPIO"))
    {
        if (c.pinNumber >= countof(GPIO_PINS)) {
            return -1;
        }
        pinMode(GPIO_PINS[c.pinNumber], OUTPUT);
        digitalWrite(GPIO_PINS[c.pinNumber], c.value);
        return 1;
    }
    return -2;
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
        if (c.pinNumber >= countof(A_PINS)) {
            return -1;
        }
        return analogRead(A_PINS[c.pinNumber]);
    }
    else if (c.pinType.equals("B")) {
        if (c.pinNumber >= countof(B_PINS)) {
            return -1;
        }
        return analogRead(B_PINS[c.pinNumber]);
    }

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
        if (c.pinNumber >= countof(D_PINS)) {
            return -1;
        }
        pinMode(D_PINS[c.pinNumber], OUTPUT);
        analogWrite(D_PINS[c.pinNumber], c.value);
        return 1;
    }
    else if (c.pinType.equals("A")) {
        if (c.pinNumber >= countof(A_PINS)) {
            return -1;
        }
        pinMode(A_PINS[c.pinNumber], OUTPUT);
        analogWrite(A_PINS[c.pinNumber], c.value);
        return 1;
    }
    else if (c.pinType.equals("B"))
    {
        if (c.pinNumber >= countof(B_PINS)) {
            return -1;
        }
        pinMode(B_PINS[c.pinNumber], OUTPUT);
        analogWrite(B_PINS[c.pinNumber], c.value);
        return 1;
    }
    else if (c.pinType.equals("C"))
    {
        if (c.pinNumber >= countof(C_PINS)) {
            return -1;
        }
        pinMode(C_PINS[c.pinNumber], OUTPUT);
        analogWrite(C_PINS[c.pinNumber], c.value);
        return 1;
    }
    else if(c.pinType.equals("GPIO"))
    {
        if (c.pinNumber >= countof(GPIO_PINS)) {
            return -1;
        }
        pinMode(GPIO_PINS[c.pinNumber], OUTPUT);
        analogWrite(GPIO_PINS[c.pinNumber], c.value);
        return 1;
    }
    return -2;
}
