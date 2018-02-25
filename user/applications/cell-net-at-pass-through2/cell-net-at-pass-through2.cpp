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

Serial1LogHandler logHandler(LOG_LEVEL_ALL); // for full debugging

SYSTEM_MODE(SEMI_AUTOMATIC);

#define RGB_RED     RGB.color(255,0,0)
#define RGB_GREEN   RGB.color(0,255,0)
#define RGB_YELLOW  RGB.color(255,255,0)
#define RGB_MAGENTA RGB.color(255,0,255)
#define RGB_CYAN    RGB.color(0,255,255)
#define RGB_BLUE    RGB.color(0,0,255)
#define RGB_IDLE    RGB.color(10,10,10)
#define RGB_OFF     RGB.color(0,0,0)

//! check for timeout
#define TIMEOUT(t, ms)  ((ms != -1) && ((millis() - t) > ms))
//! registration ok check helper
#define REG_OK(r)       ((r == REG_HOME) || (r == REG_ROAMING))

uint32_t lastUpdate = 0;
uint32_t updateRegistrationTime = 0;

NetStatus _net;

bool cellularRegisterGSM();
bool cellularRegisterGPRS();
bool cellularCheckRegisterGSM();
bool cellularCheckRegisterGPRS();
void updateRegistrationStatus();

void serial_at_response_out(void* data, const char* msg)
{
    Serial.print(msg); // AT command response sent back over USB serial

    const char* cmd = msg+3;
    int a, b, c, d, r;
    char s[32];
    // +CREG|CGREG: <n>,<stat>[,<lac>,<ci>[,AcT[,<rac>]]] // reply to AT+CREG|AT+CGREG
    // +CREG|CGREG: <stat>[,<lac>,<ci>[,AcT[,<rac>]]]     // URC
    b = (int)0xFFFF; c = (int)0xFFFFFFFF; d = -1;
    r = sscanf(cmd, "%s %*d,%d,\"%x\",\"%x\",%d",s,&a,&b,&c,&d);
    if (r <= 1)
        r = sscanf(cmd, "%s %d,\"%x\",\"%x\",%d",s,&a,&b,&c,&d);
    if (r >= 2) {
        Reg *reg = !strcmp(s, "CREG:")  ? &_net.csd :
                   !strcmp(s, "CGREG:") ? &_net.psd : NULL;
        if (reg) {
            // network status
            if      (a == 0) *reg = REG_NONE;     // 0: not registered, home network
            else if (a == 1) *reg = REG_HOME;     // 1: registered, home network
            else if (a == 2) *reg = REG_NONE;     // 2: not registered, but MT is currently searching a new operator to register to
            else if (a == 3) *reg = REG_DENIED;   // 3: registration denied
            else if (a == 4) *reg = REG_UNKNOWN;  // 4: unknown
            else if (a == 5) *reg = REG_ROAMING;  // 5: registered, roaming
            if ((r >= 3) && (b != (int)0xFFFF))      _net.lac = b; // location area code
            if ((r >= 4) && (c != (int)0xFFFFFFFF))  _net.ci  = c; // cell ID
            // access technology
            if (r >= 5) {
                if      (d == 0) _net.act = ACT_GSM;      // 0: GSM
                else if (d == 1) _net.act = ACT_GSM;      // 1: GSM COMPACT
                else if (d == 2) _net.act = ACT_UTRAN;    // 2: UTRAN
                else if (d == 3) _net.act = ACT_EDGE;     // 3: GSM with EDGE availability
                else if (d == 4) _net.act = ACT_UTRAN;    // 4: UTRAN with HSDPA availability
                else if (d == 5) _net.act = ACT_UTRAN;    // 5: UTRAN with HSUPA availability
                else if (d == 6) _net.act = ACT_UTRAN;    // 6: UTRAN with HSDPA and HSUPA availability
            }
            // DEBUG_D("%d,%d\r\n",a,d);
        }
    }
}

STARTUP(cellular_at_response_handler_set(serial_at_response_out, NULL, NULL));

bool connectGSMandGPRS() {
    memset(&_net, 0, sizeof(_net));
    cellular_result_t result = -1;
    Serial.print("Connecting to the cellular network: ");
    result = cellular_init(NULL);
    if (result) {
        Serial.println("ERROR!\r\nFailed modem initialization! Did you turn the modem on? SIM installed?");
    }
    else {
        cellularRegisterGSM();
        cellularRegisterGPRS();
        updateRegistrationTime = millis();
        return true;
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

        // START THE GSM and GPRS CONNECTION PROCESS
        if (!connectGSMandGPRS()) {
            RGB_RED;
        }
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
            else if(cmd == ":update1;" || cmd == ":UPDATE1;") {
                updateRegistrationTime = millis();
            }
            else if(cmd == ":update0;" || cmd == ":UPDATE0;") {
                updateRegistrationTime = millis() - 2*60*1000UL;
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
    } // END if (Serial.available() > 0)

    // Update GSM and GPRS registration status every 5 seconds for 2 minutes
    if (millis() - updateRegistrationTime < 2*60*1000UL) {
        if (millis() - lastUpdate > 5000UL) {
            lastUpdate = millis();
            updateRegistrationStatus();
        }
    }

    // Receive all URCs
    cellular_urcs_get(NULL);
}

void updateRegistrationStatus() {
    if (cellularCheckRegisterGSM() && cellularCheckRegisterGPRS()) {
        RGB_MAGENTA;
        // DEBUG("GSM & GPRS - CSD: %d PSD: %d", REG_OK(_net.csd), REG_OK(_net.psd));
    }
    else if (cellularCheckRegisterGSM()) {
        RGB_GREEN;
        // DEBUG("GSM ONLY - CSD: %d & PSD: %d", REG_OK(_net.csd), REG_OK(_net.psd));
    }
    else if (cellularCheckRegisterGPRS()) {
        RGB_YELLOW;
        // DEBUG("GPRS ONLY - CSD: %d & PSD: %d", REG_OK(_net.csd), REG_OK(_net.psd));
    }
}

bool cellularRegisterGSM()
{
    // system_tick_t start = millis();
    DEBUG_D("\r\n[ cellularRegisterGSM ] = = = = = = = = = = = = = =\r\n");
    if (!cellularCheckRegisterGSM()) Cellular.command("AT+CREG=2\r\n");
    // while (!cellularCheckRegisterGSM() && !TIMEOUT(start, (5*60*1000) )) {
    //     system_tick_t start = millis();
    //     while (millis() - start < 15000UL); // just wait
    // }
    // if (_net.csd == REG_DENIED) DEBUG_D("CSD Registration Denied\r\n");
    // DEBUG("%d", REG_OK(_net.csd));
    return REG_OK(_net.csd);
}

bool cellularRegisterGPRS()
{
    // system_tick_t start = millis();
    DEBUG_D("\r\n[ cellularRegisterGPRS ] = = = = = = = = = = = = = =\r\n");
    if (!cellularCheckRegisterGPRS()) Cellular.command("AT+CGREG=2\r\n");
    // while (!cellularCheckRegisterGPRS() && !TIMEOUT(start, (5*60*1000) )) {
    //     system_tick_t start = millis();
    //     while (millis() - start < 15000UL); // just wait
    // }
    // if (_net.psd == REG_DENIED) DEBUG_D("PSD Registration Denied\r\n");
    // DEBUG("%d", REG_OK(_net.psd));
    return REG_OK(_net.psd);
}

bool cellularCheckRegisterGSM()
{
    Cellular.command("AT+CREG?\r\n");
    return REG_OK(_net.csd);
}

bool cellularCheckRegisterGPRS()
{
    Cellular.command("AT+CGREG?\r\n");
    return REG_OK(_net.psd);
}