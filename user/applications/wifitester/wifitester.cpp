/**
 ******************************************************************************
 * @file    wifitester.cpp
 * @authors  David Middlecamp, Matthew McGowan
 * @version V1.0.0
 ******************************************************************************
  Copyright (c) 2015 Spark Labs, Inc.  All rights reserved.

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

#include "application.h"

uint8_t serialAvailable();
int32_t serialRead();
void serialPrintln(const char * str);
void serialPrint(const char * str);
void checkWifiSerial(char c);
void tokenizeCommand(char *cmd, char** parts);
void tester_connect(char *ssid, char *pass);

#define USE_SERIAL1 1

uint8_t notifiedAboutDHCP = 0;
int state = 0;
int wifi_testing = 0;
int dhcp_notices = 0;
unsigned cmd_index = 0;
const unsigned cmd_length = 256;
char command[cmd_length];
const char cmd_CONNECT[] = "CONNECT:";
const char cmd_RESET[] = "RESET:";
const char cmd_DFU[] = "DFU:";
const char cmd_LOCK[] = "LOCK:";
const char cmd_UNLOCK[] = "UNLOCK:";
const char cmd_REBOOT[] = "REBOOT:";
const char cmd_INFO[] = "INFO:";
const char cmd_CLEAR[] = "CLEAR:";

void setup()
{
    cmd_index = 0;
    RGB.control(true);
    RGB.color(64, 0, 0);

    pinMode(D2, OUTPUT);
    digitalWrite(D2, LOW);
    
    Serial.begin(9600);
#if USE_SERIAL1    
    Serial1.begin(9600);
#endif    
    for (int i=0; i<5; i++)
        serialPrintln(" READY READY READY ! READY READY READY ! READY READY READY !");
    
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

void printItem(const char* name, const char* value) {
    serialPrint(name);
    serialPrint(": ");
    serialPrintln(value);
}

// todo - this is specific to core-v2 HAL
struct varstring_t {
    uint8_t len;
    char string[32];
};

extern "C" bool fetch_or_generate_device_id(varstring_t* result);

void printInfo() {
    String deviceID = Spark.deviceID();
    
    WLanConfig ip_config;
    wlan_fetch_ipconfig(&ip_config);
    uint8_t* addr = ip_config.uaMacAddr;
    String macAddress;
    bool first = true;
    for (int i = 1; i < 6; i++)
    {
        if (!first) 
            macAddress += ":";
        first = false;
        macAddress += bytes2hex(addr++, 1);
    }
        
    varstring_t serial;    
    memset(&serial, 0, sizeof(serial));
    fetch_or_generate_device_id(&serial);
    
    String rssi(WiFi.RSSI());
    
    printItem("DeviceID", deviceID.c_str());
    printItem("Serial", serial.string);
    printItem("MAC", macAddress.c_str());    
    printItem("SSID", (const char*)ip_config.uaSSID);
    printItem("RSSI", rssi.c_str());
}

void checkWifiSerial(char c) {
    if (cmd_index==0)
        memset(command, 0, cmd_length);
    
	if (cmd_index < cmd_length) {
		command[cmd_index++] = c;
	}
	else {
		cmd_index = 0;
	}
    if (c==' ') 
        cmd_index = 0;
	if (c == ';') {
        command[cmd_index++] = 0;
        cmd_index = 0;
		serialPrintln("got semicolon.");
		serialPrint("checking command: ");
		serialPrintln(command);

		char *parts[5];
		char *start;

        if ((start = strstr(command, cmd_CONNECT))) {
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
            serialPrintln("DFU mode! DFU mode! DFU mode! DFU mode! DFU mode! DFU mode!");
            serialPrintln("DFU mode! DFU mode! DFU mode! DFU mode! DFU mode! DFU mode!");
            delay(100);
            serialPrintln("resetting into DFU mode!");

            System.bootloader();
        }
        else if ((start = strstr(command, cmd_RESET))) {
            //to trigger a factory reset:

            serialPrintln("factory reset! factory reset! factory reset! factory reset!");
            serialPrintln("factory reset! factory reset! factory reset! factory reset!");
            delay(2000);

            System.factoryReset();
        }
        else if ((start = strstr(command, cmd_UNLOCK))) {
            serialPrintln("unlocking...");

            //LOCK
            FLASH_WriteProtection_Disable(BOOTLOADER_FLASH_PAGES);

            RGB.color(0, 255, 0);            

            serialPrintln("unlocking... Done unlocking! Done unlocking! Done unlocking!");
            serialPrintln("unlocking... Done unlocking! Done unlocking! Done unlocking!");
        }
        else if ((start = strstr(command, cmd_LOCK))) {
            serialPrintln("Locking...");

            //LOCK
            FLASH_WriteProtection_Enable(BOOTLOADER_FLASH_PAGES);

            RGB.color(255, 0, 0);
            
            serialPrintln("Locking... Done locking! Done locking! Done locking!");
            serialPrintln("Locking... Done locking! Done locking! Done locking!");
        }
        else if ((start = strstr(command, cmd_REBOOT))) {
            serialPrintln("Rebooting... Rebooting... Rebooting...");
            serialPrintln("Rebooting... Rebooting... Rebooting...");

            delay(1000);
            System.reset();
        }
        else if ((start = strstr(command, cmd_INFO))) {
            printInfo();
        }        
        else if ((start = strstr(command, cmd_CLEAR))) {
            if (WiFi.hasCredentials())
                WiFi.clearCredentials();
        }        
	}
}

void serialPrintln(const char * str) {
	Serial.println(str);
	Serial.flush();
#if USE_SERIAL1
	Serial1.println(str);
	Serial1.flush();
#endif    
}
void serialPrint(const char * str) {
	Serial.print(str);
	Serial.flush();
#if USE_SERIAL1
	Serial1.print(str);
	Serial1.flush();
#endif    
}

uint8_t serialAvailable() {
	uint8_t result = Serial.available();
#if USE_SERIAL1
    result |= Serial1.available();
#endif
    return result;
}

int32_t serialRead() {
	if (Serial.available()) {
		return Serial.read();
	}	
#if USE_SERIAL1
    if (Serial1.available())
        return Serial1.read();
#endif    
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
    WiFi.clearCredentials();
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

