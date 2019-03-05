/**
  ******************************************************************************
  * @file    wifi_credentials_reader.h
  * @author  Zachary Crockett and Satish Nair
  * @version V1.0.0
  * @date    24-April-2013
  * @brief   header for wifi_credentials_reader.cpp
  ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

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
#pragma once

#include <string.h>
#include "spark_wiring_usbserial.h"
#include "spark_wiring_platform.h"
#include "wlan_hal.h"
#include <memory>

#if PLATFORM_ID>2
#define SETUP_OVER_SERIAL1 1
#endif

#ifndef SETUP_OVER_SERIAL1
#define SETUP_OVER_SERIAL1 0
#endif

typedef int (*ConnectCallback)( void* data,
                                const char *ssid,
                                const char *password,
                                unsigned long security_type,
                                unsigned long cipher,
                                bool dry_run);

typedef int (*ConnectCallback2)(void* data,
                                WLanCredentials* creds,
                                bool dry_run);


struct SystemSetupConsoleConfig
{

};


#if Wiring_WiFi
struct WiFiSetupConsoleConfig : SystemSetupConsoleConfig
{
    ConnectCallback connect_callback;
    ConnectCallback2 connect_callback2;
    void* connect_callback_data;
};
#endif

class SystemSetupConsoleBase {
public:
    SystemSetupConsoleBase() = default;
    virtual ~SystemSetupConsoleBase() = default;
    virtual void loop(void) = 0;
};

template<typename Config> class SystemSetupConsole : public SystemSetupConsoleBase
{
public:
    SystemSetupConsole(const Config& config);
    virtual ~SystemSetupConsole();
    virtual void loop(void);
protected:
    virtual void exit();
    /**
     * Handle a character received over serial.
     */
    virtual void handle(char c);

    /**
     * Handle a character peeked over serial.
     * Returns true if the character was pulled from the stream and handled.
     * Returns false if the character isn't recognized.
     */
    virtual bool handle_peek(char c);

    Config config;
    void print(const char *s);
    void read_line(char *dst, int max_len);
    void read_multiline(char *dst, int max_len);

    virtual void cleanup();

private:
    USBSerial serial;
#if SETUP_OVER_SERIAL1
    bool serial1Enabled;
    uint8_t magicPos;                   // how far long the magic key we are
    // Opaque pointer
    void* tester;
#endif
};

template class SystemSetupConsole<SystemSetupConsoleConfig>;

#if Wiring_WiFi
class WiFiSetupConsole : public SystemSetupConsole<WiFiSetupConsoleConfig>
{
    using super = SystemSetupConsole<WiFiSetupConsoleConfig>;

public:
    WiFiSetupConsole(const WiFiSetupConsoleConfig& config);
    virtual ~WiFiSetupConsole();

protected:
    virtual void handle(char c) override;
    virtual void exit() override;
#if Wiring_WpaEnterprise == 1
    virtual void cleanup() override;
#endif
private:
    char ssid[33];
    char password[65];
    char security_type_string[2];
    WLanSecurityType security_ = WLAN_SEC_NOT_SET;
    WLanSecurityCipher cipher_ = WLAN_CIPHER_NOT_SET;
#if Wiring_WpaEnterprise == 1
    char eap_type_string[2];
    std::unique_ptr<char[]> tmp_;
#endif
};
#endif

#if Wiring_Cellular
class CellularSetupConsoleConfig : SystemSetupConsoleConfig
{

};

class CellularSetupConsole : public SystemSetupConsole<CellularSetupConsoleConfig>
{
    using super = SystemSetupConsole<CellularSetupConsoleConfig>;

public:
    CellularSetupConsole(const CellularSetupConsoleConfig& config);
    virtual ~CellularSetupConsole();

    virtual void exit() override;
    virtual void handle(char c) override;
};

#endif /* Wiring_Cellular */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    uint16_t version;
    uint16_t size;
    void* (*create)(void* reserved);
    int   (*destroy)(void* tester, void* reserved);
    int   (*setup)(void* tester, bool useSerial1, void* reserved);
    int   (*loop)(void* tester, int c, void* reserved);
} system_tester_handlers_t;

int system_set_tester_handlers(system_tester_handlers_t* handlers, void* reserved);

#ifdef __cplusplus
}
#endif /* __cplusplus */
