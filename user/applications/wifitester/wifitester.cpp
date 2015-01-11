/**
 ******************************************************************************
 * @file    wifitester.cpp
 * @authors  David Middlecamp, Matthew McGowan
 * @version V1.0.0
 ******************************************************************************
  Copyright (c) 2013-2014 Spark Labs, Inc.  All rights reserved.

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
#include "hw_config.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

uint8_t serialAvailable();
int32_t serialRead();
void serialPrintln(const char * str);
void serialPrint(const char * str);
void checkWifiSerial(char c);
void tokenizeCommand(char *cmd, char** parts);
void tester_connect(char *ssid, char *pass);


uint8_t notifiedAboutDHCP = 0;
int state = 0;
int wifi_testing = 0;
int dhcp_notices = 0;
int cmd_index = 0, cmd_length = 256;
char command[256];
const char cmd_CONNECT[] = "CONNECT:";
const char cmd_RESET[] = "RESET:";
const char cmd_DFU[] = "DFU:";
const char cmd_LOCK[] = "LOCK:";
const char cmd_UNLOCK[] = "UNLOCK:";
const char cmd_REBOOT[] = "REBOOT:";


void setup()
{
    // mdma - this isn't implemented for the broadcom chip
    //WLAN_MANUAL_CONNECT = 1;            

    RGB.control(true);
    RGB.color(64, 0, 0);

    pinMode(D2, OUTPUT);
    digitalWrite(D2, LOW);

    Serial.begin(9600);
    Serial1.begin(9600);
    
    //DONE: startup without wifi, via SEMI_AUTOMATIC mode
    //DONE: run setup/loop without wifi (ditto))
    //TODO: try to connect to manual wifi asap
    //TODO: set the pin / report via serial
}


void loop()
{
    if (WiFi.ready() && (dhcp_notices < 5)) {
        serialPrintln(" DHCP DHCP DHCP ! DHCP DHCP DHCP ! DHCP DHCP DHCP !");
		RGB.color(255, 0, 255);
		digitalWrite(D2, HIGH);
		dhcp_notices++;
	}
	else if (wifi_testing) {
	    state = !state;
	    RGB.color(64, (state) ? 255 : 0, 64);
	}

	if (serialAvailable()) {
	    state = !state;
	    RGB.color(0, 64, (state) ? 255 : 0);

		int c = serialRead();
		checkWifiSerial((char)c);
    }
}


void checkWifiSerial(char c) {
	if (cmd_index < cmd_length) {
		command[cmd_index] = c;
		cmd_index++;
	}
	else {
		cmd_index = 0;
	}

	if (c == ' ') {
		//reset the command index.
		cmd_index = 0;
	}
	else if (c == ';') {
		serialPrintln("got semicolon.");

		serialPrint("checking command: ");
		serialPrintln(command);

		char *parts[5];
		char *start;

        if ((start = strstr(command, cmd_CONNECT))) {
            cmd_index = 0;

            serialPrintln("tokenizing...");

            //expecting CONNECT:SSID:PASS;
            tokenizeCommand(start, parts);
            serialPrintln("parts...");
            serialPrintln(parts[0]);
            serialPrintln(parts[1]);
            if (parts[2]) {
                serialPrintln(parts[2]);
            }

            serialPrintln("connecting...");
            wifi_testing = 1;
            tester_connect(parts[1], parts[2]);
        }
        else if ((start = strstr(command, cmd_DFU))) {
            cmd_index = 0;

            serialPrintln("DFU mode! DFU mode! DFU mode! DFU mode! DFU mode! DFU mode!");
            serialPrintln("DFU mode! DFU mode! DFU mode! DFU mode! DFU mode! DFU mode!");
            delay(100);
            serialPrintln("resetting into DFU mode!");

            System.bootloader();
        }
        else if ((start = strstr(command, cmd_RESET))) {
            cmd_index = 0;

            //to trigger a factory reset:

            serialPrintln("factory reset! factory reset! factory reset! factory reset!");
            serialPrintln("factory reset! factory reset! factory reset! factory reset!");
            delay(2000);

            System.factoryReset();
        }
        else if ((start = strstr(command, cmd_UNLOCK))) {
            cmd_index = 0;

            serialPrintln("unlocking...");

            //LOCK
            FLASH_WriteProtection_Disable(BOOTLOADER_FLASH_PAGES);

            RGB.color(0, 255, 0);            

            serialPrintln("unlocking... Done unlocking! Done unlocking! Done unlocking!");
            serialPrintln("unlocking... Done unlocking! Done unlocking! Done unlocking!");
        }
        else if ((start = strstr(command, cmd_LOCK))) {
            cmd_index = 0;

            serialPrintln("Locking...");

            //LOCK
            FLASH_WriteProtection_Enable(BOOTLOADER_FLASH_PAGES);

            RGB.color(255, 0, 0);
            
            serialPrintln("Locking... Done locking! Done locking! Done locking!");
            serialPrintln("Locking... Done locking! Done locking! Done locking!");
        }
        else if ((start = strstr(command, cmd_REBOOT))) {
            cmd_index = 0;

            serialPrintln("Rebooting... Rebooting... Rebooting...");
            serialPrintln("Rebooting... Rebooting... Rebooting...");

            delay(1000);
            System.reset();
        }
        
        // if we get here, clear the buffer for the next command read
        cmd_index = 0;
        memset(command, 0, sizeof(command));

	}
}


void serialPrintln(const char * str) {
	Serial.println(str);
	Serial1.println(str);

	Serial.flush();
	Serial1.flush();
}
void serialPrint(const char * str) {
	Serial.print(str);
	Serial1.print(str);

    Serial.flush();
	Serial1.flush();
}

uint8_t serialAvailable() {
	return Serial.available() | Serial1.available();
}

int32_t serialRead() {
	if (Serial.available()) {
		return Serial.read();
	}
	else if (Serial1.available()) {
		return Serial1.read();
	}
	return 0;
}

void tokenizeCommand(char *cmd, char* parts[]) {
	char * pch;
	int idx = 0;

	for(int i=0;i<5;i++) {
	    parts[i] = NULL;
	}

	//printf ("Splitting string \"%s\" into tokens:\n", cmd);
	pch = strtok (cmd,":;");
	while (pch != NULL)
	{
		if (idx < 5) {
			parts[idx++] = pch;
		}
		pch = strtok (NULL, ":;");
	}
}

void tester_connect(char *ssid, char *pass) {

    RGB.color(64, 64, 0);

    int auth = WPA2;

    if (!pass || (strlen(pass) == 0)) {
        auth = UNSEC;
    }


    //SPARK_MANUAL_CREDS(ssid, pass, auth);

	RGB.color(0, 0, 64);
    
    WiFi.on();
    WiFi.setCredentials(ssid, pass, auth);    
    WiFi.connect();
    
//
//	wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);
//	wlan_connect(WLAN_SEC_WPA2, ssid, strlen(ssid), NULL, pass, strlen(pass));
//	WLAN_MANUAL_CONNECT = 0;
//
//	RGBColor = 0xFF00FF;		//purple
	//USERLED_SetRGBColor(0xFF00FF);		//purple
	//USERLED_On(LED_RGB);
	serialPrintln("  WIFI Connected?    ");
}
