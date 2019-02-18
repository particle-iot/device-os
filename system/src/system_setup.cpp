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
#include "ota_flash_hal.h"
#include "wlan_hal.h"
#include "cellular_hal.h"
#include "system_cloud_internal.h"
#include "system_update.h"
#include "spark_wiring.h"   // for serialReadLine
#include "system_network_internal.h"
#include "system_network.h"
#include "system_task.h"
#include "spark_wiring_thread.h"
#include "spark_wiring_wifi_credentials.h"
#include "system_ymodem.h"
#include "mbedtls_util.h"
#include "ota_flash_hal.h"

#if SETUP_OVER_SERIAL1
#define SETUP_LISTEN_MAGIC 1
void loop_wifitester(int c);
#include "spark_wiring_usartserial.h"

static system_tester_handlers_t s_tester_handlers = {0};

int system_set_tester_handlers(system_tester_handlers_t* handlers, void* reserved) {
    memset(&s_tester_handlers, 0, sizeof(s_tester_handlers));
    if (handlers != nullptr) {
        memcpy(&s_tester_handlers, handlers, std::min(static_cast<uint16_t>(sizeof(s_tester_handlers)), handlers->size));
    }
    return 0;
}

#else /* SETUP_OVER_SERIAL1 */

int system_set_tester_handlers(system_tester_handlers_t* handlers, void* reserved) {
    return 1;
}

#endif /* SETUP_OVER_SERIAL1 */

#ifndef SETUP_LISTEN_MAGIC
#define SETUP_LISTEN_MAGIC 0
#endif

#ifndef PRIVATE_KEY_SIZE
#define PRIVATE_KEY_SIZE        (2*1024)
#endif

#ifndef CERTIFICATE_SIZE
#define CERTIFICATE_SIZE        (4*1024)
#endif

// This can be changed to Serial for testing purposes
#define SETUP_SERIAL Serial1

int is_empty(const char *s) {
  while (*s != '\0') {
    if (!isspace(*s))
      return 0;
    s++;
  }
  return 1;
}

class StreamAppender : public Appender
{
    Stream& stream_;

public:
    StreamAppender(Stream& stream) : stream_(stream) {}

    virtual bool append(const uint8_t* data, size_t length) override {
        return append(&stream_, data, length);
    }

    static bool append(void* appender, const uint8_t* data, size_t length) { // appender_fn
        const auto stream = static_cast<Stream*>(appender);
        return (stream->write(data, length) == length);
    }
};

class WrappedStreamAppender : public StreamAppender
{
  bool wrotePrefix_;
  const uint8_t* prefix_;
  size_t prefixLength_;
  const uint8_t* suffix_;
  size_t suffixLenght_;
public:
  WrappedStreamAppender(
      Stream& stream,
      const uint8_t* prefix,
      size_t prefixLength,
      const uint8_t* suffix,
      size_t suffixLenght) :
    StreamAppender(stream),
    wrotePrefix_(false),
    prefix_(prefix),
    prefixLength_(prefixLength),
    suffix_(suffix),
    suffixLenght_(suffixLenght)
  {}

  ~WrappedStreamAppender() {
    append(suffix_, suffixLenght_);
  }

  virtual bool append(const uint8_t* data, size_t length) override {
    if (!wrotePrefix_) {
      StreamAppender::append(prefix_, prefixLength_);
      wrotePrefix_ = true;
    }
    return StreamAppender::append(data, length);
  }
};


#if SETUP_OVER_SERIAL1
inline bool setup_serial1() {
    uint8_t value = 0;
    system_get_flag(SYSTEM_FLAG_WIFITESTER_OVER_SERIAL1, &value, nullptr);
    return value;
}
#endif

template <typename Config> SystemSetupConsole<Config>::SystemSetupConsole(const Config& config_)
    : config(config_)
{
    WITH_LOCK(serial);
    if (serial.baud() == 0)
    {
        serial.begin(9600);
    }
#if SETUP_OVER_SERIAL1
    serial1Enabled = false;
    magicPos = 0;
    if (setup_serial1()) {
            //WITH_LOCK(SETUP_SERIAL);
            SETUP_SERIAL.begin(9600);
    }
    this->tester = nullptr;
#endif
}

template <typename Config> SystemSetupConsole<Config>::~SystemSetupConsole()
{
#if SETUP_OVER_SERIAL1
    if (tester != nullptr && s_tester_handlers.destroy) {
        s_tester_handlers.destroy(this->tester, nullptr);
    }
#endif
}

template<typename Config> void SystemSetupConsole<Config>::loop(void)
{
#if SETUP_OVER_SERIAL1
    //TRY_LOCK(SETUP_SERIAL)
    {
        if (setup_serial1() && s_tester_handlers.size != 0) {
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
                                if (tester == nullptr && s_tester_handlers.create != nullptr) {
                                    tester = s_tester_handlers.create(nullptr);
                                }
                                if (tester != nullptr && s_tester_handlers.setup) {
                                    s_tester_handlers.setup(tester, SETUP_OVER_SERIAL1, nullptr);
                                }
                            }
                        }
                        else {
                            magicPos = 0;
                        }
                        c = -1;
                    }
                }
                else {
                    if (tester != nullptr && s_tester_handlers.loop) {
                        s_tester_handlers.loop(tester, c, nullptr);
                    }
                }
            }
        }
    }
#endif

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
void SystemSetupConsole<Config>::cleanup()
{
}

template <typename Config>
void SystemSetupConsole<Config>::exit()
{
    network_listen(0, NETWORK_LISTEN_EXIT, nullptr);
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

bool filter_key(const char* src, char* dest, size_t size) {
	memcpy(dest, src, size);
	if (!strcmp(src, "imei")) {
		strcpy(dest, "IMEI");
	}
	else if (!strcmp(src, "iccid")) {
		strcpy(dest, "ICCID");
	}
	else if (!strcmp(src, "sn")) {
		strcpy(dest, "Serial Number");
	}
	else if (!strcmp(src, "ms")) {
		strcpy(dest, "Device Secret");
	}
	return false;
}

template<typename Config> void SystemSetupConsole<Config>::handle(char c)
{
    if ('i' == c)
    {
    	// see if we have any additional properties. This is true
    	// for Cellular and Mesh devices.
    	hal_system_info_t info = {};
    	info.size = sizeof(info);
    	HAL_OTA_Add_System_Info(&info, true, nullptr);
    	LOG(TRACE, "device info key/value count: %d", info.key_value_count);
    	if (info.key_value_count) {
    		print("Device ID: ");
    		String id = spark_deviceID();
			print(id.c_str());
			print("\r\n");

			for (int i=0; i<info.key_value_count; i++) {
				char key[20];
				if (!filter_key(info.key_values[i].key, key, sizeof(key))) {
					print(key);
					print(": ");
					print(info.key_values[i].value);
					print("\r\n");
				}
			}
		}
    	else {
	#if PLATFORM_ID<3
			print("Your core id is ");
	#else
			print("Your device id is ");
	#endif
			String id = spark_deviceID();
			print(id.c_str());
			print("\r\n");
    	}
    	HAL_OTA_Add_System_Info(&info, false, nullptr);
    }
    else if ('m' == c)
    {
        print("Your device MAC address is\r\n");
        IPConfig config = {};
    #if !HAL_PLATFORM_WIFI    
        auto conf = static_cast<const IPConfig*>(network_config(0, 0, 0));
    #else
        auto conf = static_cast<const IPConfig*>(network_config(NETWORK_INTERFACE_WIFI_STA, 0, 0));
    #endif
        if (conf && conf->size) {
            memcpy(&config, conf, std::min(sizeof(config), (size_t)conf->size));
        }
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
        auto prefix = "{";
        auto suffix = "}\r\n";
        WrappedStreamAppender appender(serial, (const uint8_t*)prefix, strlen(prefix), (const uint8_t*)suffix, strlen(suffix));
        system_module_info(append_instance, &appender);
    }
    else if ('v' == c)
    {
        StreamAppender appender(serial);
        append_system_version_info(&appender);
        print("\r\n");
    }
    else if ('L' == c)
    {
        system_set_flag(SYSTEM_FLAG_STARTUP_LISTEN_MODE, 1, nullptr);
        System.enterSafeMode();
    }
    else if ('c' == c)
    {
            bool claimed = HAL_IsDeviceClaimed(nullptr);
            print("Device claimed: ");
            print(claimed ? "yes" : "no");
            print("\r\n");
    }
    else if ('C' == c)
    {
            char code[64];
            print("Enter 63-digit claim code: ");
            read_line(code, 63);
            if (strlen(code)==63) {
                HAL_Set_Claim_Code(code);
                print("Claim code set to: ");
                print(code);
            }
            else {
                print("Sorry, claim code is not 63 characters long. Claim code unchanged.");
}
        print("\r\n");
    }
    else if ('d' == c)
    {
        system_format_diag_data(nullptr, 0, 0, StreamAppender::append, &serial, nullptr);
        print("\r\n");
    }
}

/* private methods */

template<typename Config> void SystemSetupConsole<Config>::print(const char *s)
{
    for (size_t i = 0; i < strlen(s); ++i)
    {
        serial.write(s[i]);
    }
}

template<typename Config> void SystemSetupConsole<Config>::read_line(char *dst, int max_len)
{
    serialReadLine(&serial, dst, max_len, 0); //no timeout
    print("\r\n");
    while (0 < serial.available())
        serial.read();
}

template<typename Config> void SystemSetupConsole<Config>::read_multiline(char *dst, int max_len)
{
    char *ptr = dst;
    int len = max_len;
    while(len > 3) {
        serialReadLine(&serial, ptr, len, 0); //no timeout
        print("\r\n");
        int l = strlen(ptr);
        len -= l;
        ptr += l;
        if (len > 3) {
            if (l != 0) {
                *ptr++ = '\r';
                *ptr++ = '\n';
            }
            *ptr = '\0';
        }
        if (l == 0)
            return;
    }
    while (0 < serial.available())
        serial.read();
}

#if Wiring_WiFi

WiFiSetupConsole::WiFiSetupConsole(const WiFiSetupConsoleConfig& config)
 : SystemSetupConsole(config)
{
}

WiFiSetupConsole::~WiFiSetupConsole()
{
}

void WiFiSetupConsole::handle(char c)
{
    if ('w' == c)
    {
        memset(ssid, 0, sizeof(ssid));
        memset(password, 0, sizeof(password));
        memset(security_type_string, 0, sizeof(security_type_string));
        security_ = WLAN_SEC_NOT_SET;
        cipher_ = WLAN_CIPHER_NOT_SET;

#if Wiring_WpaEnterprise == 1
        spark::WiFiAllocatedCredentials credentials;
        memset(eap_type_string, 0, sizeof(eap_type_string));
#else
        spark::WiFiCredentials credentials;
#endif
        WLanCredentials creds;

        do {
        print("SSID: ");
        read_line(ssid, 32);
        } while (strlen(ssid) == 0);

        wlan_scan([](WiFiAccessPoint* ap, void* ptr) {
            if (ptr) {
                WiFiSetupConsole* self = reinterpret_cast<WiFiSetupConsole*>(ptr);
                if (ap) {
                    if (ap->ssidLength && !strncmp(self->ssid, ap->ssid, std::max((size_t)ap->ssidLength, (size_t)strlen(self->ssid)))) {
                        self->security_ = ap->security;
                        self->cipher_ = ap->cipher;
                    }
                }
            }
        }, this);

        if (security_ == WLAN_SEC_NOT_SET)
        {
        do
        {
#if Wiring_WpaEnterprise == 0
            print("Security 0=unsecured, 1=WEP, 2=WPA, 3=WPA2: ");
            read_line(security_type_string, 1);
        }
        while ('0' > security_type_string[0] || '3' < security_type_string[0]);
#else
                print("Security 0=unsecured, 1=WEP, 2=WPA, 3=WPA2, 4=WPA Enterprise, 5=WPA2 Enterprise: ");
                read_line(security_type_string, 1);
            }
            while ('0' > security_type_string[0] || '5' < security_type_string[0]);
#endif
            security_ = (WLanSecurityType)(security_type_string[0] - '0');
        }

#if PLATFORM_ID<3
        if (security_ == WLAN_SEC_WEP)
        {
            print("\r\n ** Even though the CC3000 supposedly supports WEP,");
            print("\r\n ** we at Spark have never seen it work.");
            print("\r\n ** If you control the network, we recommend changing it to WPA2.\r\n");
        }
#endif

        if (security_ != WLAN_SEC_UNSEC)
            password[0] = '1'; // non-empty password so security isn't set to None

        credentials.setSsid(ssid);
        credentials.setPassword(password);
        credentials.setSecurity(security_);
        credentials.setCipher(cipher_);

        // dry run
        creds = credentials.getHalCredentials();
        if (this->config.connect_callback2(this->config.connect_callback_data, &creds, true)==WLAN_SET_CREDENTIALS_CIPHER_REQUIRED ||
            (cipher_ == WLAN_CIPHER_NOT_SET))
        {
            do
            {
                print("Security Cipher 1=AES, 2=TKIP, 3=AES+TKIP: ");
                read_line(security_type_string, 1);
            }
            while ('1' > security_type_string[0] || '3' < security_type_string[0]);
            switch (security_type_string[0]-'0') {
                case 1: cipher_ = WLAN_CIPHER_AES; break;
                case 2: cipher_ = WLAN_CIPHER_TKIP; break;
                case 3: cipher_ = WLAN_CIPHER_AES_TKIP; break;
            }
            credentials.setCipher(cipher_);
        }

        if (0 < security_ && security_ <= 3)
        {
            print("Password: ");
            read_line(password, sizeof(password) - 1);
            credentials.setPassword(password);
        }
#if Wiring_WpaEnterprise == 1
        else if (security_ >= 4)
        {
            do
            {
                print("EAP Type 0=PEAP/MSCHAPv2, 1=EAP-TLS: ");
                read_line(eap_type_string, 1);
            }
            while ('0' > eap_type_string[0] || '1' < eap_type_string[0]);
            int eap_type = eap_type_string[0] - '0';

            if (!tmp_) {
                tmp_.reset(new (std::nothrow) char[CERTIFICATE_SIZE]);
            }
            if (!tmp_) {
                print("Error while preparing to store enterprise credentials.\r\n\r\n");
                return;
            }

            if (eap_type == 1) {
                // EAP-TLS
                // Required:
                // - client certificate
                // - private key
                // Optional:
                // - root CA
                // - outer identity
                credentials.setEapType(WLAN_EAP_TYPE_TLS);

                memset(tmp_.get(), 0, CERTIFICATE_SIZE);
                print("Client certificate in PEM format:\r\n");
                read_multiline((char*)tmp_.get(), CERTIFICATE_SIZE - 1);
                {
                    uint8_t* der = NULL;
                    size_t der_len = 0;
                    if (!mbedtls_x509_crt_pem_to_der((const char*)tmp_.get(), strnlen(tmp_.get(), CERTIFICATE_SIZE - 1) + 1, &der, &der_len)) {
                        credentials.setClientCertificate(der, der_len);
                        free(der);
                    }
                }

                memset(tmp_.get(), 0, CERTIFICATE_SIZE);
                print("Private key in PEM format:\r\n");
                read_multiline((char*)tmp_.get(), PRIVATE_KEY_SIZE - 1);
                {
                    uint8_t* der = NULL;
                    size_t der_len = 0;
                    if (!mbedtls_pk_pem_to_der((const char*)tmp_.get(), strnlen(tmp_.get(), PRIVATE_KEY_SIZE - 1) + 1, &der, &der_len)) {
                        credentials.setPrivateKey(der, der_len);
                        free(der);
                    }
                }
            } else {
                // PEAP/MSCHAPv2
                // Required:
                // - inner identity
                // - password
                // Optional:
                // - root CA
                // - outer identity
                credentials.setEapType(WLAN_EAP_TYPE_PEAP);

                memset(tmp_.get(), 0, CERTIFICATE_SIZE);
                print("Username: ");
                read_line((char*)tmp_.get(), 64);
                credentials.setIdentity((const char*)tmp_.get());

                print("Password: ");
                read_line(password, sizeof(password) - 1);
                credentials.setPassword(password);
        }

            memset(tmp_.get(), 0, CERTIFICATE_SIZE);
            print("Outer identity (optional): ");
            read_line((char*)tmp_.get(), 64);
            if (strlen(tmp_.get())) {
                credentials.setOuterIdentity((const char*)tmp_.get());
            }

            memset(tmp_.get(), 0, CERTIFICATE_SIZE);
            print("Root CA in PEM format (optional):\r\n");
            read_multiline((char*)tmp_.get(), CERTIFICATE_SIZE - 1);
            if (strlen(tmp_.get()) && !is_empty(tmp_.get())) {
                uint8_t* der = NULL;
                size_t der_len = 0;
                if (!mbedtls_x509_crt_pem_to_der((const char*)tmp_.get(), strnlen(tmp_.get(), CERTIFICATE_SIZE - 1) + 1, &der, &der_len)) {
                    credentials.setRootCertificate(der, der_len);
                    free(der);
                }
            }
            tmp_.reset();
        }
#endif

        print("Thanks! Wait "
#if PLATFORM_ID<3
    "about 7 seconds "
#endif
            "while I save those credentials...\r\n\r\n");
        creds = credentials.getHalCredentials();
        if (this->config.connect_callback2(this->config.connect_callback_data, &creds, false)==0)
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
        cleanup();
    }
    else {
        super::handle(c);
    }
}

#if Wiring_WpaEnterprise == 1
void WiFiSetupConsole::cleanup()
{
    tmp_.reset();
}
#endif

void WiFiSetupConsole::exit()
{
    network_listen(0, NETWORK_LISTEN_EXIT, 0);
}

#endif


#if Wiring_Cellular

CellularSetupConsole::CellularSetupConsole(const CellularSetupConsoleConfig& config)
 : SystemSetupConsole(config)
{
}

CellularSetupConsole::~CellularSetupConsole()
{
}

void CellularSetupConsole::exit()
{
    network_listen(0, NETWORK_LISTEN_EXIT, 0);
}

void CellularSetupConsole::handle(char c)
{
	super::handle(c);
}

#endif
