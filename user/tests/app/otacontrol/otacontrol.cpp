/**
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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

/**
 * Modified version of tinker that includes a "cmd" function to tweak the OTA behavior:
 * - enableUpdates   - allow OTA updates
 * - disableUpdates  - disallow OTA updates
 * - updatesEnabled (returns 0/1) - determine if updates are enabled
 * - enableReset     - allow system reset
 * - disableReset    - disallow system reset
 * - resetEnabled    - determine if reset is enabled
 * - updateTimeout=X  - set update timeout (how many seconds the application blocks when notified of an available update)
 */

/* Function prototypes -------------------------------------------------------*/
int tinkerDigitalRead(String pin);
int tinkerDigitalWrite(String command);
int tinkerAnalogRead(String pin);
int tinkerAnalogWrite(String command);

SYSTEM_MODE(AUTOMATIC);

int updateTimeout = 0;

asdf

int handleCmd(String cmd)
{
    if (cmd==String("enableUpdates")) {
        System.enableUpdates();
        Serial.println("enabled updates");
    }
    if (cmd==String("disableUpdates")) {
        System.disableUpdates();
        Serial.println("disabled updates");
    }

    if (cmd==String("updatesEnabled")) {
        Serial.printlnf("updates are %s " System.updatesEnabled() ? "enabled" : "disabled");
        return System.updatesEnabled();
    }

    if (cmd==String("enableReset"))
        System.enableReset();
    if (cmd==String("disableReset"))
        System.disableReset();
    if (cmd==String("resetEnabled")) {
        Serial.printlnf("reset is %s " System.resetEnabled() ? "enabled" : "disabled");
        return System.resetEnabled();
    }

    const char* str = cmd;
    const char* ut = "updateTimeout=";
    if (memcmp(ut, str, strlen(ut))==0)
    {
        updateTimeout = atoi(str+strlen(ut));
    }
    return 0;
}


void eventHandler(system_event_t event, int param, void*)
{
    switch (event)
    {
    case reset:
        Serial.println("going down for reboot...NOW");
        break;
    case reset_pending:
        Serial.println("reset pending");
        break;

    case firmware_update_pending:
        Serial.println("firmware update pending.");
        if (updateTimeout) {
            delay(updateTimeout*1000);
        }
        break;
    case firmware_update:
        if (param==firmware_update_begin)
        {
            Serial.println("firmware update begin");
        }
        if (param==firmware_update_complete)
        {
            Serial.println("firmware update complete");
        }
        if (param==firmware_update_failed)
        {
            Serial.println("firmware update failed");
        }
    }
}

/* This function is called once at start up ----------------------------------*/
void setup()
{
    Serial.begin(9600);

    //Setup the Tinker application here
    //Register all the Tinker functions
    Particle.function("digitalread", tinkerDigitalRead);
    Particle.function("digitalwrite", tinkerDigitalWrite);

    Particle.function("analogread", tinkerAnalogRead);
    Particle.function("analogwrite", tinkerAnalogWrite);

    System.on(all_events, eventHandler);

    Particle.function("cmd", handleCmd);
}

/* This function loops forever --------------------------------------------*/
void loop()
{
}

/*******************************************************************************
 * Function Name  : tinkerDigitalRead
 * Description    : Reads the digital value of a given pin
 * Input          : Pin
 * Output         : None.
 * Return         : Value of the pin (0 or 1) in INT type
                    Returns a negative number on failure
 *******************************************************************************/
int tinkerDigitalRead(String pin)
{
    //convert ascii to integer
    int pinNumber = pin.charAt(1) - '0';
    //Sanity check to see if the pin numbers are within limits
    if (pinNumber< 0 || pinNumber >7) return -1;

    if(pin.startsWith("D"))
    {
        pinMode(pinNumber, INPUT_PULLDOWN);
        return digitalRead(pinNumber);
    }
    else if (pin.startsWith("A"))
    {
        pinMode(pinNumber+10, INPUT_PULLDOWN);
        return digitalRead(pinNumber+10);
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
    bool value = 0;
    //convert ascii to integer
    int pinNumber = command.charAt(1) - '0';
    //Sanity check to see if the pin numbers are within limits
    if (pinNumber< 0 || pinNumber >7) return -1;

    if(command.substring(3,7) == "HIGH") value = 1;
    else if(command.substring(3,6) == "LOW") value = 0;
    else return -2;

    if(command.startsWith("D"))
    {
        pinMode(pinNumber, OUTPUT);
        digitalWrite(pinNumber, value);
        return 1;
    }
    else if(command.startsWith("A"))
    {
        pinMode(pinNumber+10, OUTPUT);
        digitalWrite(pinNumber+10, value);
        return 1;
    }
    else return -3;
}

/*******************************************************************************
 * Function Name  : tinkerAnalogRead
 * Description    : Reads the analog value of a pin
 * Input          : Pin
 * Output         : None.
 * Return         : Returns the analog value in INT type (0 to 4095)
                    Returns a negative number on failure
 *******************************************************************************/
int tinkerAnalogRead(String pin)
{
    //convert ascii to integer
    int pinNumber = pin.charAt(1) - '0';
    //Sanity check to see if the pin numbers are within limits
    if (pinNumber< 0 || pinNumber >7) return -1;

    if(pin.startsWith("D"))
    {
        return -3;
    }
    else if (pin.startsWith("A"))
    {
        return analogRead(pinNumber+10);
    }
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
    //convert ascii to integer
    int pinNumber = command.charAt(1) - '0';
    //Sanity check to see if the pin numbers are within limits
    if (pinNumber< 0 || pinNumber >7) return -1;

    String value = command.substring(3);

    if(command.startsWith("D"))
    {
        pinMode(pinNumber, OUTPUT);
        analogWrite(pinNumber, value.toInt());
        return 1;
    }
    else if(command.startsWith("A"))
    {
        pinMode(pinNumber+10, OUTPUT);
        analogWrite(pinNumber+10, value.toInt());
        return 1;
    }
    else return -2;
}