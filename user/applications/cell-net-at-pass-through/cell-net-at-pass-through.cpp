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
#include "cellular_hal.h"

// ALL_LEVEL, TRACE_LEVEL, DEBUG_LEVEL, INFO_LEVEL, WARN_LEVEL, ERROR_LEVEL, PANIC_LEVEL, NO_LOG_LEVEL
// All system debugging info output on TX pin at 115200 baud
Serial1DebugOutput debugOutput(115200, ALL_LEVEL);

SYSTEM_MODE(SEMI_AUTOMATIC);

#define RGB_RED   RGB.color(255,0,0)
#define RGB_GREEN RGB.color(0,255,0)
#define RGB_BLUE  RGB.color(0,0,255)
#define RGB_IDLE  RGB.color(10,10,10)
#define RGB_OFF   RGB.color(0,0,0)

void serial_at_response_out(void* data, const char* msg)
{
   Serial.print(msg); // AT command response sent back over USB serial
}

STARTUP(cellular_at_response_handler_set(serial_at_response_out, NULL, NULL));

bool connect() {
    cellular_result_t result = -1;
    Serial.print("Connecting to the cellular network: ");
    result = cellular_init(NULL);
    if (result) {
        Serial.println("ERROR!\r\nFailed modem initialization! Did you turn the modem on? SIM installed?");
    }
    else {
        result = cellular_register(NULL);
        if (result) {
            Serial.println("ERROR!\r\nFailed to register to cellular network! Do you have the SIM installed?"
                                 "\r\nTry removing power completely, and re-applying.");
        }
        else {
            // CellularCredentials* savedCreds;
            // savedCreds = cellular_credentials_get(NULL);
            // result = cellular_gprs_attach(savedCreds, NULL);
            // if (result) {
            //     Serial.println("ERROR!\r\nFailed to GPRS attach! Did you activate your SIM?");
            // }
            // else {
                Serial.println("OK! CSD and PSD registered.");
                return true;
            // }
        }
    }
    return false;
}

void setup()
{
    Serial.begin(9600);
    RGB.control(true);
    RGB_RED; delay(250);
    RGB_GREEN; delay(250);
    RGB_BLUE; delay(250);
    RGB_IDLE;

    // POWER ON THE MODEM
    Serial.print("Turning on the modem: ");
    cellular_result_t result = -1;
    result = cellular_on(NULL);
    if (result) {
        Serial.println("ERROR!\r\nFailure powering on the modem!");
        RGB_RED;
    }
    else {
        Serial.println("OK! Power on.");
        RGB_BLUE;

        // CONNECT TO THE CELLULAR NETWORK
        if (connect()) RGB_GREEN;
        else RGB_RED;
    }
}

void loop()
{
    // AT commands received over USB serial
    static String cmd = "";
    static bool echo_commands = true;
    static bool assist = true;
    if (Serial.available() > 0) {
        char c = Serial.read();
        if (c == '\r') {
            Serial.println();
            if(cmd == ":power1;" || cmd == ":POWER1;") {
                cellular_on(NULL);
            }
            else if(cmd == ":power0;" || cmd == ":POWER0;") {
                cellular_off(NULL);
            }
            else if(cmd == ":echo1;" || cmd == ":ECHO1;") {
                echo_commands = true;
            }
            else if(cmd == ":echo0;" || cmd == ":ECHO0;") {
                echo_commands = false;
            }
            else if(cmd == ":assist1;" || cmd == ":ASSIST1;") {
                assist = true;
            }
            else if(cmd == ":assist0;" || cmd == ":ASSIST0;") {
                assist = false;
            }
            else if(cmd != "") {
                Cellular.command("%s\r\n", cmd.c_str());
            }
            cmd = "";
        }
        else if (assist && c == 27) { // ESC
            if (cmd.length() > 0 && echo_commands) {
                Serial.print('\r');
                for (uint32_t x=0; x<cmd.length(); x++) {
                    Serial.print(' ');
                }
                Serial.print('\r');
            }
            cmd = "";
        }
        else if (assist && (c == 8 || c == 127)) { // BACKSPACE
            if (cmd.length() > 0) {
                cmd.remove(cmd.length()-1, 1);
                if (echo_commands) {
                    Serial.print("\b \b");
                }
            }
        }
        else {
            cmd += c;
            if (echo_commands) Serial.print(c);
        }
    }
}

