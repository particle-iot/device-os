/**
 ******************************************************************************
 * @file    wifitester.cpp
 * @authors  David Middlecamp, Matthew McGowan
 * @version V1.0.0
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

#include "spark_wiring.h"
#include "spark_wiring_wifi.h"
#include "spark_wiring_usbserial.h"
#include "spark_wiring_usartserial.h"
#include "spark_wiring_wifitester.h"
#include "core_hal.h"
#include "spark_wiring_version.h"
#include "string_convert.h"
#include "bytes2hexbuf.h"
#include "deviceid_hal.h"

#if 0 //MODULAR_FIRMWARE
// The usual wiring implementations are dependent upon I2C, SPI and other global instances. Rewriting the GPIO functions to talk directly to the HAL

void pinMode(pin_t pin, PinMode setMode) {
    HAL_Pin_Mode(pin, setMode);
}

void digitalWrite(pin_t pin, uint8_t value) {
    HAL_GPIO_Write(pin, value);
}

int32_t digitalRead(pin_t pin) {
    return HAL_GPIO_Read(pin);
}
#endif

#if PLATFORM_ID==4 || PLATFORM_ID==5 || PLATFORM_ID==6 || PLATFORM_ID==7 || PLATFORM_ID==8
#define WIFI_SCAN 1
#else
#define WIFI_SCAN 0
#endif

#if WIFI_SCAN
#include "wlan_hal.h"
#endif


#if Wiring_WiFi
#define NETWORK WiFi
#elif Wiring_Cellular
#define NETWORK Cellular
#elif Wiring_Mesh
#define NETWORK Mesh
#endif

#ifdef NETWORK

using namespace spark;

uint8_t serialAvailable();
int32_t serialRead();
void serialPrintln(const char * str);
void serialPrint(const char * str);
void checkWifiSerial(char c);
void tokenizeCommand(char *cmd, char** parts, unsigned max_parts);
void tester_connect(char *ssid, char *pass);

#define USE_SERIAL1 1

const char cmd_CONNECT[] = "CONNECT:";
const char cmd_RESET[] = "RESET:";
const char cmd_DFU[] = "DFU:";
const char cmd_LOCK[] = "LOCK:";
const char cmd_UNLOCK[] = "UNLOCK:";
const char cmd_REBOOT[] = "REBOOT:";
const char cmd_INFO[] = "INFO:";
const char cmd_CLEAR[] = "CLEAR:";
#if WIFI_SCAN
const char cmd_WIFI_SCAN[] = "WIFI_SCAN:";
const char cmd_ANT[] = "ANT";
#endif
const char cmd_SET_PIN[] = "SET_PIN:";
const char cmd_SET_PRODUCT[] = "SET_PRODUCT:";
const char cmd_POWER_ON[] = "POWER_ON:";

// This class should be factored out into a network neutral base, and subclasses for WiFi/Cellular

void WiFiTester::setup(bool useSerial1) {
    this->useSerial1 = useSerial1;

#if Wiring_WiFi
    // Only turn on Wi-Fi module by default.  Turning on Cellular without a modem present will create long delays.
    NETWORK.on();
    power_state = true;
#endif
    cmd_index = 0;
    RGB.control(true);
    RGB.color(64, 0, 0);

    HAL_Pin_Mode(D2, OUTPUT);
    HAL_GPIO_Write(D2, LOW);

    serialPrintln("GOOD DAY, WIFI TESTER AT YOUR SERVICE!!!");
    //DONE: startup without wifi, via SEMI_AUTOMATIC mode
    //DONE: run setup/loop without wifi (ditto))
    //TODO: try to connect to manual wifi asap
    //TODO: set the pin / report via serial
}

void WiFiTester::loop(int c) {

    if (NETWORK.ready() && (dhcp_notices < 5)) {
        serialPrintln(" DHCP DHCP DHCP ! DHCP DHCP DHCP ! DHCP DHCP DHCP !");
        RGB.color(255, 0, 255);
        HAL_GPIO_Write(D2, HIGH);
        dhcp_notices++;
    } else if (wifi_testing) {
        state = !state;
        RGB.color(64, (state) ? 255 : 0, 64);
    }
    if (c != -1) {
        state = !state;
        RGB.color(0, 64, (state) ? 255 : 0);
        checkWifiSerial((char) c);
    }
}

void WiFiTester::printItem(const char* name, const char* value) {
    serialPrint(name);
    serialPrint(": ");
    serialPrintln(value);
}

// todo - this is specific to photon HAL

struct varstring_t {
    uint8_t len;
    char string[33];
};

#if PLATFORM_ID>3
extern "C" bool fetch_or_generate_setup_ssid(varstring_t* result);
#else // PLATFORM <= 3
extern "C" bool fetch_or_generate_setup_ssid(varstring_t* result) {
    result->len = 4;
    strcpy(result->string, "CORE-1234");
    return true;
}
#endif

bool WiFiTester::isPowerOn() {
    return power_state;
}

void WiFiTester::printInfo() {
    String deviceID = Particle.deviceID();

#if Wiring_WiFi
    WLanConfig ip_config;
    ip_config.size = sizeof(ip_config);
    wlan_fetch_ipconfig(&ip_config);

    uint8_t* addr = ip_config.nw.uaMacAddr;
    String macAddress;
    bool first = true;
    char tmp[3] = {0};
    for (int m = 0; m < 6; m++) {
        if (!first)
            macAddress += ":";
        first = false;
        bytes2hexbuf(addr, 1, tmp);
        macAddress += tmp;
    }
#endif

    varstring_t serial;
    memset(&serial, 0, sizeof (serial));
    fetch_or_generate_setup_ssid(&serial);
    serial.string[serial.len] = 0;

    printItem("DeviceID", deviceID.c_str());
    printItem("Serial", strstr((const char*) serial.string, "-") + 1);
#if Wiring_WiFi
    String rssi(WiFi.RSSI());
    printItem("MAC", macAddress.c_str());
#endif


#if Wiring_Cellular
    printItem("MAC", "00:00:00:00:00:00");   // needed by the sticker rig

    // GET ICCID and IMEI (Modem with power on required)
    if (isPowerOn()) {
        CellularDevice dev;
        cellular_device_info(&dev, NULL);
        printItem("ICCID", dev.iccid);
        printItem("IMEI", dev.imei);
    }
#endif

    printItem("SSID", serial.string);

#if Wiring_WiFi
    printItem("RSSI", rssi.c_str());
#else
    // the sticker rig waits for the RSSI value
    printItem("RSSI", "-50");
#endif

}

#if WIFI_SCAN
void wlan_scan_callback(WiFiAccessPoint* ap, void* data)
{
    WiFiTester& tester = *(WiFiTester*)data;
    char str_ssid[33];
    memcpy(str_ssid, ap->ssid, ap->ssidLength);
    str_ssid[ap->ssidLength] = 0;
    tester.serialPrint(str_ssid);
    tester.serialPrint(",");
    itoa(ap->rssi, str_ssid, 10);
    tester.serialPrintln(str_ssid);
}

void WiFiTester::wifiScan() {
    WiFi.on();
    serialPrintln("SCAN_START");
    wlan_scan(wlan_scan_callback, this);
    serialPrintln("SCAN_STOP");
}
#endif

void setPinOutput(uint16_t pin, uint16_t value)
{
    HAL_Pin_Mode(pin, OUTPUT);
    HAL_GPIO_Write(pin, value);
}

void setPinOutputRange(uint16_t start, uint16_t end, uint16_t value)
{
    while (start<=end) {
        setPinOutput(start++, value);
    }
}

bool is_digit(char c) {
    return c>='0' && c<='9';
}

const char* parseAntennaSelect(const char* c, WLanSelectAntenna_TypeDef& select) {
    static const char* names[] = {
        "INTERNAL", "EXTERNAL", "", "AUTO"
    };
    for (unsigned i=0; i<arraySize(names); i++) {
        if (!strncmp(names[i], c, strlen(c))) {
            select = WLanSelectAntenna_TypeDef(i);
            return names[i];
        }
    }
    return NULL;
}

void WiFiTester::checkWifiSerial(char c) {
    if (cmd_index == 0)
        memset(command, 0, cmd_length);

    if (cmd_index < cmd_length) {
        command[cmd_index++] = c;
    } else {
        cmd_index = 0;
    }

    if (c == ';') {
        command[cmd_index++] = 0;
        cmd_index = 0;
        serialPrint("checking command: ");
        serialPrintln(command);

        char *parts[5];
        char *start;

        if ((start = strstr(command, cmd_CONNECT))) {
            serialPrintln("tokenizing...");

            //expecting CONNECT:SSID:PASS;
            tokenizeCommand(start, parts, 5);
            serialPrintln("parts...");
            serialPrintln(parts[0]);
            serialPrintln(parts[1]);
            if (parts[2]) {
                serialPrintln(parts[2]);
            }

            serialPrintln("connecting...");
            wifi_testing = 1;
            tester_connect(parts[1], parts[2]);
        } else if ((start = strstr(command, cmd_DFU))) {
            serialPrintln("DFU mode! DFU mode! DFU mode! DFU mode! DFU mode! DFU mode!");
            serialPrintln("DFU mode! DFU mode! DFU mode! DFU mode! DFU mode! DFU mode!");
            HAL_Delay_Milliseconds(200);

            System.dfu();
        } else if ((start = strstr(command, cmd_RESET))) {
            //to trigger a factory reset:
            serialPrintln("factory reset! factory reset! factory reset! factory reset!");
            serialPrintln("factory reset! factory reset! factory reset! factory reset!");
            HAL_Delay_Milliseconds(200);

            System.factoryReset();
        } else if ((start = strstr(command, cmd_UNLOCK))) {
            serialPrintln("unlocking...");

            //UNLOCK
            HAL_Bootloader_Lock(false);

            RGB.color(0, 255, 0);

            serialPrintln("unlocking... Done unlocking! Done unlocking! Done unlocking!");
            serialPrintln("unlocking... Done unlocking! Done unlocking! Done unlocking!");
        } else if ((start = strstr(command, cmd_LOCK))) {
            serialPrintln("Locking...");

            //LOCK
            HAL_Bootloader_Lock(true);

            RGB.color(255, 0, 0);

            serialPrintln("Locking... Done locking! Done locking! Done locking!");
            serialPrintln("Locking... Done locking! Done locking! Done locking!");
        } else if ((start = strstr(command, cmd_REBOOT))) {
            serialPrintln("Rebooting... Rebooting... Rebooting...");
            serialPrintln("Rebooting... Rebooting... Rebooting...");

            HAL_Delay_Milliseconds(200);
            System.reset();
        } else if ((start = strstr(command, cmd_POWER_ON))) {
            serialPrintln("Power on... Power on... Power on...");
            serialPrintln("Power on... Power on... Power on...");

#if Wiring_Cellular
            HAL_Delay_Milliseconds(200);
            cellular_result_t result = cellular_on(NULL);
            if (result == 0) {
                power_state = true;
            } else {
                power_state = false;
            }
#endif
            // Respond for all platforms
            if (isPowerOn()) {
                serialPrintln("POWER IS ON");
            } else {
                serialPrintln("POWER FAILED TO TURN ON!!!");
            }
            HAL_Delay_Milliseconds(200);
        } else if ((start = strstr(command, cmd_INFO))) {
            printInfo();
        }
        else if ((start = strstr(command, cmd_CLEAR))) {
#if Wiring_WiFi
                if (WiFi.hasCredentials())
                WiFi.clearCredentials();
#endif
        }
#if WIFI_SCAN
        else if ((start = strstr(command, cmd_WIFI_SCAN))) {
            wifiScan();
        }
#endif
        else if ((start = strstr(command, cmd_SET_PIN))) {
            tokenizeCommand(start, parts, 5);
            bool ok = parts[1] && parts[2];
            uint16_t pinValue;
            if (ok) {
                if (!strcmp("HIGH", parts[2])) {
                    pinValue = HIGH;
                }
                else if (!strcmp("LOW", parts[2])) {
                    pinValue = LOW;
                }
                else {
                    ok = false;
                }
            }
            if (ok) {
                if (!strcmp("ALL", parts[1])) {
#if HAL_PLATFORM_NRF52840
                    setPinOutputRange(A0, A5, pinValue);
                    setPinOutputRange(D0, D8, pinValue);
#else
                    setPinOutputRange(A0, A7, pinValue);
                    setPinOutputRange(D0, D7, pinValue);
#endif
                }
                else if (parts[1][0]=='D' && is_digit(parts[1][1]) && !parts[1][2]) {
                    setPinOutput(D0 + (parts[1][1]-'0'), pinValue);
                }
                else if (parts[1][0]=='A' && is_digit(parts[1][1]) && !parts[1][2]) {
                    setPinOutput(A0 + (parts[1][1]-'0'), pinValue);
                }
                else {
                    ok = false;
                }
            }
            if (ok) {
                serialPrint("PIN:");
                serialPrint(parts[1]);
                serialPrint(": IS ");
                serialPrintln(parts[2]);
            }
        }
        else if ((start = strstr(command, cmd_SET_PRODUCT))) {
            tokenizeCommand(start, parts, 5);
            long productID = strtoul(parts[1], NULL, 10);
            if (productID) {
                spark_protocol_set_product_id(spark_protocol_instance(), productID);
                product_details_t details;
                details.size = sizeof(details);
                spark_protocol_get_product_details(spark_protocol_instance(), &details);
                serialPrint("PRODUCT_ID IS NOW ");
                String id(details.product_id);
                serialPrintln(id.c_str());
            }

        }
#if WIFI_SCAN
        else if ((start=strstr(command, cmd_ANT))) {
            tokenizeCommand(start, parts, 5);
            if (!parts[1]) {
                WiFi.on();
                serialPrintln("ANTENNA TEST");
                serialPrintln("");
                serialPrintln("Internal");
                serialPrintln("--------");
                WiFi.selectAntenna(ANT_INTERNAL);
                wifiScan();
                serialPrintln("");
                serialPrintln("External");
                serialPrintln("--------");
                WiFi.selectAntenna(ANT_EXTERNAL);
                wifiScan();
                serialPrintln("ANTENNA TEST COMPLETE");
            }
            else {
                WLanSelectAntenna_TypeDef selectAntenna;
                const char* name;
                if ((name=parseAntennaSelect(parts[1], selectAntenna))!=NULL) {
                    serialPrint("Selected antenna ");
                    serialPrintln(name);
                    WiFi.selectAntenna(selectAntenna);
                }
            }
        }
#endif
    }
}

void WiFiTester::serialPrintln(const char * str) {
    Serial.println(str);
    Serial.flush();
#if USE_SERIAL1
    if (useSerial1) {
        Serial1.println(str);
        Serial1.flush();
    }
#endif
}

void WiFiTester::serialPrint(const char * str) {
    Serial.print(str);
    Serial.flush();
#if USE_SERIAL1
    if (useSerial1) {
        Serial1.print(str);
        Serial1.flush();
    }
#endif
}

uint8_t WiFiTester::serialAvailable() {
    uint8_t result = Serial.available();
#if USE_SERIAL1
    result |= (useSerial1 && Serial1.available());
#endif
    return result;
}

int32_t WiFiTester::serialRead() {
    if (Serial.available()) {
        return Serial.read();
    }
#if USE_SERIAL1
    if (useSerial1 && Serial1.available())
        return Serial1.read();
#endif
    return 0;
}

void WiFiTester::tokenizeCommand(char *cmd, char* parts[], unsigned max_parts) {
    char * pch;
    unsigned idx = 0;

    for (unsigned i = 0; i < max_parts; i++) {
        parts[i] = NULL;
    }

    pch = strtok(cmd, ":;");
    while (pch != NULL) {
        if (idx < max_parts) {
            parts[idx++] = pch;
        }
        pch = strtok(NULL, ":;");
    }
}

void WiFiTester::tester_connect(char *ssid, char *pass) {

#if 0
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
    //  wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);
    //  wlan_connect(WLAN_SEC_WPA2, ssid, strlen(ssid), NULL, pass, strlen(pass));
    //  WLAN_MANUAL_CONNECT = 0;
    //
    //  RGBColor = 0xFF00FF;        //purple
    //USERLED_SetRGBColor(0xFF00FF);        //purple
    //USERLED_On(LED_RGB);
    serialPrintln("  WIFI Connected?    ");
#endif
}

void WiFiTester::init() {
#if defined(SETUP_OVER_SERIAL1) && SETUP_OVER_SERIAL1 == 1
    system_tester_handlers_t handlers = {0};
    handlers.version = 1;
    handlers.size = sizeof(handlers);
    handlers.create = [](void* reserved) -> void* {
        return new WiFiTester();
    };
    handlers.destroy = [](void* tester, void* reserved) -> int {
        WiFiTester* t = reinterpret_cast<WiFiTester*>(tester);
        if (t != nullptr) {
            delete t;
            return 0;
        }
        return 1;
    };
    handlers.setup = [](void* tester, bool useSerial1, void* reserved) -> int {
        WiFiTester* t = reinterpret_cast<WiFiTester*>(tester);
        if (t != nullptr) {
            t->setup(useSerial1);
            return 0;
        }
        return 1;
    };
    handlers.loop = [](void* tester, int c, void* reserved) -> int {
        WiFiTester* t = reinterpret_cast<WiFiTester*>(tester);
        if (t != nullptr) {
            t->loop(c);
            return 0;
        }
        return 1;
    };
    system_set_tester_handlers(&handlers, nullptr);
#endif // defined(SETUP_OVER_SERIAL1) && SETUP_OVER_SERIAL1 == 1
}

#endif
