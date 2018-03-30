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

// USBSerial1LogHandler logHandler(LOG_LEVEL_ALL); // for full debugging
Serial1LogHandler logHandler(LOG_LEVEL_ALL); // for full debugging

SYSTEM_MODE(SEMI_AUTOMATIC);

#define RGB_RED     RGB.color(255,0,0)
#define RGB_GREEN   RGB.color(0,255,0)
#define RGB_YELLOW  RGB.color(255,255,0)
#define RGB_WHITE   RGB.color(255,255,255)
#define RGB_MAGENTA RGB.color(255,0,255)
#define RGB_CYAN    RGB.color(0,255,255)
#define RGB_BLUE    RGB.color(0,0,255)
#define RGB_IDLE    RGB.color(10,10,10)
#define RGB_OFF     RGB.color(0,0,0)

//! check for timeout
#define TIMEOUT(t, ms)  ((ms != -1) && ((millis() - t) > ms))
//! registration ok check helper
#define REG_OK(r)       ((r == REG_HOME) || (r == REG_ROAMING))
#define COPS_TIMEOUT (3 * 60 * 1000)
// ID of the PDP context used to configure the default EPS bearer when registering in an LTE network
// Note: There are no PDP contexts in LTE, SARA-R4 uses this naming for the sake of simplicity
#define PDP_CONTEXT 1

uint32_t lastUpdate = 0;
uint32_t updateRegistrationTime = 0;

NetStatus _net;

struct CGDCONTparam { char type[8]; char apn[32]; };

bool cellularRegisterEPS();
bool cellularRegisterGPRS();
bool cellularCheckRegisterEPS();
bool cellularCheckRegisterGPRS();
void updateRegistrationStatus();
bool cellularDisableEPSandGPRSreg();
void cellularStopEPSandGPRSstatusUpdates();
bool cellularSetAPN(const char* apn);
void showHelp();

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
        Reg* reg = nullptr;
            if (strcmp(s, "CREG:") == 0) {
                reg = &_net.csd;
            } else if (strcmp(s, "CGREG:") == 0) {
                reg = &_net.psd;
            } else if (strcmp(s, "CEREG:") == 0) {
                reg = &_net.eps;
            }
        if (reg) {
            // network status
            if      (a == 0) *reg = REG_NONE;     // 0: not registered, home network
            else if (a == 1) *reg = REG_HOME;     // 1: registered, home network
            else if (a == 2) *reg = REG_NONE;     // 2: not registered, but MT is currently searching a new operator to register to
            else if (a == 3) *reg = REG_DENIED;   // 3: registration denied
            else if (a == 4) *reg = REG_UNKNOWN;  // 4: unknown
            else if (a == 5) *reg = REG_ROAMING;  // 5: registered, roaming
            else if (a == 6) *reg = REG_HOME;     // 6: registered, sms only, home
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
                else if (d == 7) _net.act = ACT_LTE;         // 7: LTE
                else if (d == 8) _net.act = ACT_LTE_CAT_M1;  // 8: LTE CAT-M1
                else if (d == 9) _net.act = ACT_LTE_CAT_NB1; // 9: LTE CAT-NB1
            }
            // DEBUG_D("%d,%d\r\n",a,d);
        }
    }
}

int _cbCGDCONT(int type, const char* buf, int len, CGDCONTparam* param) {
    if (type == TYPE_PLUS || type == TYPE_UNKNOWN) {
        buf = (const char*)memchr(buf, '+', len); // Skip leading new line characters
        if (buf) {
            int id;
            CGDCONTparam p = {};
            static_assert(sizeof(p.type) == 8 && sizeof(p.apn) == 32, "The format string below needs to be updated accordingly");
            if (sscanf(buf, "+CGDCONT: %d,\"%7[^,],\"%31[^,],", &id, p.type, p.apn) == 3 && id == PDP_CONTEXT) {
                p.type[strlen(p.type) - 1] = '\0'; // Trim trailing quote character
                p.apn[strlen(p.apn) - 1] = '\0';
                *param = p;
            }
        }
    }
    return WAIT;
}

STARTUP(cellular_at_response_handler_set(serial_at_response_out, NULL, NULL));

bool connectEPSandGPRS() {
    memset(&_net, 0, sizeof(_net));
    cellular_result_t result = -1;
    Serial.print("Connecting to the cellular network: ");
    result = cellular_init(NULL);
    if (result) {
        Serial.println("ERROR!\r\nFailed modem initialization! Did you turn the modem on? SIM installed?");
    }
    else {
        cellularSetAPN("");
        cellularRegisterEPS();
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

        // Ensure we are disconnected from the network
        if (!cellularDisableEPSandGPRSreg()) {
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
            if(cmd == ":on;" || cmd == ":ON;") {
                if (0 == cellular_on(NULL)) {
                    RGB_BLUE;
                } else {
                    RGB_RED;
                }
            }
            else if(cmd == ":off;" || cmd == ":OFF;") {
                if (0 == cellular_off(NULL)) {
                    RGB_WHITE;
                } else {
                    RGB_RED;
                }
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
            else if(cmd == ":con;" || cmd == ":CON;") {
                // START THE EPS and GPRS CONNECTION PROCESS
                if (!connectEPSandGPRS()) {
                    RGB_RED;
                }
            }
            else if(cmd == ":dis;" || cmd == ":DIS;") {
                // STOP polling the EPS and GPRS status
                if (!cellularDisableEPSandGPRSreg()) {
                    RGB_RED;
                } else {
                    RGB_BLUE;
                }
            }
            else if(cmd == ":help;" || cmd == ":HELP;") {
                showHelp();
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

    // Update EPS and GPRS registration status every 5 seconds for 2 minutes
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
    if (cellularCheckRegisterEPS() && cellularCheckRegisterGPRS()) {
        RGB_GREEN;
        cellularStopEPSandGPRSstatusUpdates();
        // DEBUG("EPS & GPRS - EPS: %d PSD: %d", REG_OK(_net.eps), REG_OK(_net.psd));
    }
    else if (cellularCheckRegisterEPS()) {
        RGB_MAGENTA;
        // DEBUG("EPS ONLY - EPS: %d & PSD: %d", REG_OK(_net.eps), REG_OK(_net.psd));
    }
    else if (cellularCheckRegisterGPRS()) {
        RGB_YELLOW;
        // DEBUG("GPRS ONLY - EPS: %d & PSD: %d", REG_OK(_net.eps), REG_OK(_net.psd));
    }
}

bool cellularRegisterEPS()
{
    // system_tick_t start = millis();
    // DEBUG_D("\r\n[ cellularRegisterEPS ] = = = = = = = = = = = = = =\r\n");
    if (!cellularCheckRegisterEPS()) Cellular.command("AT+CEREG=2\r\n");
    // while (!cellularCheckRegisterGSM() && !TIMEOUT(start, (5*60*1000) )) {
    //     system_tick_t start = millis();
    //     while (millis() - start < 15000UL); // just wait
    // }
    // if (_net.csd == REG_DENIED) DEBUG_D("CSD Registration Denied\r\n");
    // DEBUG("%d", REG_OK(_net.csd));
    return REG_OK(_net.eps);
}

bool cellularRegisterGPRS()
{
    // system_tick_t start = millis();
    // DEBUG_D("\r\n[ cellularRegisterGPRS ] = = = = = = = = = = = = = =\r\n");
    if (!cellularCheckRegisterGPRS()) Cellular.command("AT+CGREG=2\r\n");
    // while (!cellularCheckRegisterGPRS() && !TIMEOUT(start, (5*60*1000) )) {
    //     system_tick_t start = millis();
    //     while (millis() - start < 15000UL); // just wait
    // }
    // if (_net.psd == REG_DENIED) DEBUG_D("PSD Registration Denied\r\n");
    // DEBUG("%d", REG_OK(_net.psd));
    return REG_OK(_net.psd);
}

bool cellularCheckRegisterEPS()
{
    Cellular.command("AT+CEREG?\r\n");
    return REG_OK(_net.eps);
}

bool cellularCheckRegisterGPRS()
{
    Cellular.command("AT+CGREG?\r\n");
    return REG_OK(_net.psd);
}

void cellularStopEPSandGPRSstatusUpdates()
{
    // Update the timer such that it won't attempt to poll for EPS and GPRS status
    updateRegistrationTime = millis() - 2*60*1000UL;
}

bool cellularDisableEPSandGPRSreg()
{
    cellularStopEPSandGPRSstatusUpdates();
    return (RESP_OK == Cellular.command(COPS_TIMEOUT, "AT+COPS=2\r\n"));
}

bool cellularSetAPN(const char* apn)
{
    // Get default context settings
    CGDCONTparam ctx = {};
    if (RESP_OK != Cellular.command(_cbCGDCONT, &ctx, 10000, "AT+CGDCONT?\r\n")) {
        return false;
    }
    // TODO: SARA-R410-01B modules come preconfigured with AT&T's APN ("broadband"), which may
    // cause network registration issues with MVNO providers and third party SIM cards. As a
    // workaround the code below sets a blank APN if it detects that the current context is
    // configured to use the dual stack IPv4/IPv6 capability ("IPV4V6"), which is the case for
    // the factory default settings. Ideally, setting of a default APN should be based on IMSI
    if (strcmp(ctx.type, "IP") != 0 || strcmp(ctx.apn, apn ? apn : "") != 0) {
        // Stop the network registration and update the context settings
        if (RESP_OK != Cellular.command(COPS_TIMEOUT, "AT+COPS=2\r\n")) {
            return false;
        }
        if (RESP_OK != Cellular.command("AT+CGDCONT=%d,\"IP\",\"%s\"\r\n", PDP_CONTEXT, apn ? apn : "")) {
            return false;
        }
    }
    // Make sure automatic network registration is enabled
    if (RESP_OK != Cellular.command(COPS_TIMEOUT, "AT+COPS=0\r\n")) {
        return false;
    }

    return true;
}

void showHelp() {
    Serial.println("\r\nCommands supported:"
                   "\r\n[:on;     ] turn the cellular modem ON (default), LED=BLUE"
                   "\r\n[:off;    ] turn the cellular modem OFF, LED=WHITE"
                   "\r\n[:echo1;  ] AT command echo ON (default)"
                   "\r\n[:echo0;  ] AT command echo OFF"
                   "\r\n[:assist1;] turns on ESCAPE and BACKSPACE keyboard assistance (default)"
                   "\r\n[:assist0;] turns off ESCAPE and BACKSPACE keyboard assistance"
                   "\r\n[:con;    ] Start the EPS and GPRS connection process, LED=GREEN (GPRS & EPS), MAGENTA (EPS), YELLOW (GPRS)"
                   "\r\n[:dis;    ] Network disconnect and stop polling the EPS and GPRS status (default), LED=BLUE"
                   "\r\n[:help;   ] show this help menu\r\n");
}
