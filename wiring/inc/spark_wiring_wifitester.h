/**
 ******************************************************************************
 * @file    wifitester.h
 * @authors mat
 * @date    27 January 2015
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

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

#ifndef WIFITESTER_H
#define WIFITESTER_H

#include "system_setup.h"
#include "spark_wiring_startup.h"

#ifdef __cplusplus

class WiFiTester
{
    bool useSerial1;

    uint8_t notifiedAboutDHCP;
    int state;
    int wifi_testing;
    int dhcp_notices;
    unsigned cmd_index;
    static const unsigned cmd_length = 256;
    char command[cmd_length];
    bool power_state = false;


    void checkWifiSerial(char c);
    void wifiScan();
    void printInfo();
    void printItem(const char* name, const char* value);
    void tester_connect(char *ssid, char *pass);
    void tokenizeCommand(char *cmd, char* parts[], unsigned max_parts);
    bool isPowerOn();

public:
    WiFiTester() {
        memset(this, 0, sizeof(*this));
    }

    void setup(bool useSerial1);
    void loop(int c);

    uint8_t serialAvailable();
    int32_t serialRead();
    void serialPrintln(const char* s);
    void serialPrint(const char* s);

    static void init();
};

#endif /* __cplusplus */

#endif  /* WIFITESTER_H */
