/**
 ******************************************************************************
 * @file    wifi_credentials_reader.cpp
 * @author  Zachary Crockett and Satish Nair
 * @version V1.0.0
 * @date    24-April-2013
 * @brief
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

#include "system_setup.h"
#include "delay_hal.h"
#include "wlan_hal.h"
#include "cellular_hal.h"
#include "system_cloud_internal.h"
#include "system_update.h"
#include "spark_wiring.h"   // for serialReadLine
#include "system_network_internal.h"
#include "system_network.h"
#include "system_task.h"
#include "spark_wiring_thread.h"
#include "system_ymodem.h"

#if SETUP_OVER_SERIAL1
#define SETUP_LISTEN_MAGIC 1
void loop_wifitester(int c);
#include "spark_wiring_usartserial.h"
#include "wifitester.h"
#endif

#ifndef SETUP_LISTEN_MAGIC
#define SETUP_LISTEN_MAGIC 0
#endif

#define SETUP_SERIAL Serial1

class StreamAppender : public Appender
{
    Stream& stream_;

public:
    StreamAppender(Stream& stream) : stream_(stream) {}

    bool append(const uint8_t* data, size_t length) {
        return stream_.write(data, length)==length;
    }
};

template <typename Config> SystemSetupConsole<Config>::SystemSetupConsole(Config& config_)
    : config(config_)
{
    WITH_LOCK(serial);
    if (serial.baud() == 0)
    {
        serial.begin(9600);
    }
}

template<typename Config> void SystemSetupConsole<Config>::loop(void)
{
    TRY_LOCK(serial)
    {
        if (serial.available())
        {
            int c = serial.peek();
            if (c >= 0)
            {
                if (!handle_peek((char)c))
                {
                    if (serial.available())
                    {
                        c = serial.read();
                        handle((char)c);
                    }
                }
            }
        }
    }
}

template <typename Config>
bool SystemSetupConsole<Config>::handle_peek(char c)
{
    if (YModem::SOH == c || YModem::STX == c)
    {
        system_firmwareUpdate(&serial);
        return true;
    }
    return false;
}

template<typename Config> void SystemSetupConsole<Config>::handle(char c)
{
    if ('i' == c)
    {
#if PLATFORM_ID<3
        print("Your core id is ");
#else
        print("Your device id is ");
#endif
        String id = spark_deviceID();
        print(id.c_str());
        print("\r\n");
    }
    else if ('m' == c)
    {
        print("Your device MAC address is\r\n");
        IPConfig config;
        config.size = sizeof(config);
        network.get_ipconfig(&config);
        const uint8_t* addr = config.nw.uaMacAddr;
        print(bytes2hex(addr++, 1).c_str());
        for (int i = 1; i < 6; i++)
        {
            print(":");
            print(bytes2hex(addr++, 1).c_str());
        }
        print("\r\n");
    }
    else if ('f' == c)
    {
        serial.println("Waiting for the binary file to be sent ... (press 'a' to abort)");
        system_firmwareUpdate(&serial);
    }
    else if ('x' == c)
    {
        exit();
    }
    else if ('s' == c)
    {
        StreamAppender appender(serial);
        print("{");
        system_module_info(append_instance, &appender);
        print("}\r\n");
    }
    else if ('v' == c)
    {
        StreamAppender appender(serial);
        append_system_version_info(&appender);
        print("\r\n");
    }
    else if ('L' == c)
    {
        system_set_flag(SYSTEM_FLAG_STARTUP_SAFE_LISTEN_MODE, 1, nullptr);
        System.enterSafeMode();
    }
}

/* private methods */

template<typename Config> void SystemSetupConsole<Config>::print(const char *s)
{
    for (size_t i = 0; i < strlen(s); ++i)
    {
        serial.write(s[i]);
        HAL_Delay_Milliseconds(1); // ridonkulous, but required
    }
}

template<typename Config> void SystemSetupConsole<Config>::read_line(char *dst, int max_len)
{
    serialReadLine(&serial, dst, max_len, 0); //no timeout
    print("\r\n");
    while (0 < serial.available())
        serial.read();
}

#if Wiring_WiFi

inline bool setup_serial1() {
	uint8_t value = 0;
	system_get_flag(SYSTEM_FLAG_WIFITESTER_OVER_SERIAL1, &value, nullptr);
	return value;
}

WiFiSetupConsole::WiFiSetupConsole(WiFiSetupConsoleConfig& config)
 : SystemSetupConsole(config)
{
#if SETUP_OVER_SERIAL1
    serial1Enabled = false;
    magicPos = 0;
    if (setup_serial1()) {
    		SETUP_SERIAL.begin(9600);
    }
    this->tester = NULL;
#endif
}

WiFiSetupConsole::~WiFiSetupConsole()
{
#if SETUP_OVER_SERIAL1
    delete this->tester;
#endif
}

void WiFiSetupConsole::loop()
{
#if SETUP_OVER_SERIAL1
	if (setup_serial1()) {
		int c = -1;
		if (SETUP_SERIAL.available()) {
			c = SETUP_SERIAL.read();
		}
		if (SETUP_LISTEN_MAGIC) {
			static uint8_t magic_code[] = { 0xe1, 0x63, 0x57, 0x3f, 0xe7, 0x87, 0xc2, 0xa6, 0x85, 0x20, 0xa5, 0x6c, 0xe3, 0x04, 0x9e, 0xa0 };
			if (!serial1Enabled) {
				if (c>=0) {
					if (c==magic_code[magicPos++]) {
						serial1Enabled = magicPos==sizeof(magic_code);
						if (serial1Enabled) {
							if (tester==NULL)
								tester = new WiFiTester();
							tester->setup(SETUP_OVER_SERIAL1);
						}
					}
					else {
						magicPos = 0;
					}
					c = -1;
				}
			}
			else {
				if (tester)
					tester->loop(c);
			}
		}
	}
#endif
    super::loop();
}

void WiFiSetupConsole::handle(char c)
{
    if ('w' == c)
    {
        memset(ssid, 0, 33);
        memset(password, 0, 65);
        memset(security_type_string, 0, 2);

        print("SSID: ");
        read_line(ssid, 32);

        // TODO: would be great if the network auto-detected the Security type
        // The photon is scanning the network so could determine this.
        do
        {
            print("Security 0=unsecured, 1=WEP, 2=WPA, 3=WPA2: ");
            read_line(security_type_string, 1);
        }
        while ('0' > security_type_string[0] || '3' < security_type_string[0]);

#if PLATFORM_ID<3
        if ('1' == security_type_string[0])
        {
            print("\r\n ** Even though the CC3000 supposedly supports WEP,");
            print("\r\n ** we at Spark have never seen it work.");
            print("\r\n ** If you control the network, we recommend changing it to WPA2.\r\n");
        }
#endif

        unsigned long security_type = security_type_string[0] - '0';

        WLanSecurityCipher cipher = WLAN_CIPHER_NOT_SET;

        if (security_type)
            password[0] = '1'; // non-empty password so security isn't set to None

        // dry run
        if (this->config.connect_callback(this->config.connect_callback_data, ssid, password, security_type, cipher, true)==WLAN_SET_CREDENTIALS_CIPHER_REQUIRED)
        {
            do
            {
                print("Security Cipher 1=AES, 2=TKIP, 3=AES+TKIP: ");
                read_line(security_type_string, 1);
            }
            while ('1' > security_type_string[0] || '3' < security_type_string[0]);
            switch (security_type_string[0]-'0') {
                case 1: cipher = WLAN_CIPHER_AES; break;
                case 2: cipher = WLAN_CIPHER_TKIP; break;
                case 3: cipher = WLAN_CIPHER_AES_TKIP; break;
            }
        }

        if (0 < security_type)
        {
            print("Password: ");
            read_line(password, 64);
        }

        print("Thanks! Wait "
#if PLATFORM_ID<3
    "about 7 seconds "
#endif
            "while I save those credentials...\r\n\r\n");

        if (this->config.connect_callback(this->config.connect_callback_data, ssid, password, security_type, cipher, false)==0)
        {
            print("Awesome. Now we'll connect!\r\n\r\n");
            print("If you see a pulsing cyan light, your "
    #if PLATFORM_ID==0
                "Spark Core"
    #else
                "device"
    #endif
                "\r\n");
            print("has connected to the Cloud and is ready to go!\r\n\r\n");
            print("If your LED flashes red or you encounter any other problems,\r\n");
            print("visit https://www.particle.io/support to debug.\r\n\r\n");
            print("    Particle <3 you!\r\n\r\n");
        }
        else
        {
            print("Derp. Sorry, we couldn't save the credentials.\r\n\r\n");
        }
    }
    else {
        super::handle(c);
    }
}


void WiFiSetupConsole::exit()
{
    network.listen(true);
}

#endif


#if Wiring_Cellular

CellularSetupConsole::CellularSetupConsole(CellularSetupConsoleConfig& config)
 : SystemSetupConsole(config)
{
}

void CellularSetupConsole::exit()
{
    network.listen(true);
}

void CellularSetupConsole::handle(char c)
{
    if (c=='i')
    {
        CellularDevice dev;
        cellular_device_info(&dev, NULL);
        String id = spark_deviceID();
        print("Device ID: ");
        print(id.c_str());
        print("\r\n");
        print("IMEI: ");
        print(dev.imei);
        print("\r\n");
        print("ICCID: ");
        print(dev.iccid);
        print("\r\n");
    }
    else
        super::handle(c);
}


#endif
