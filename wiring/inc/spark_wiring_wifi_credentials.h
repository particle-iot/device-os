/**
 ******************************************************************************
 * @file    spark_wiring_wifi_credentials.h
 * @author  Andrey Tolstoy
 * @version V1.0.0
 * @date    7-Mar-2014
 * @brief   WiFi credentials classes definitions
 ******************************************************************************
  Copyright (c) 2013-2017 Particle Industries, Inc.  All rights reserved.

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

#ifndef __SPARK_WIRING_WIFI_CREDENTIALS_H
#define __SPARK_WIRING_WIFI_CREDENTIALS_H

#include "spark_wiring_platform.h"

#if Wiring_WiFi

#include "wlan_hal.h"
#include "spark_wiring_vector.h"
#include <memory>

namespace spark {

enum SecurityType {
    UNSEC = WLAN_SEC_UNSEC,
    WEP = WLAN_SEC_WEP,
    WPA = WLAN_SEC_WPA,
    WPA2 = WLAN_SEC_WPA2,
    WPA_ENTERPRISE = WLAN_SEC_WPA_ENTERPRISE,
    WPA2_ENTERPRISE = WLAN_SEC_WPA2_ENTERPRISE
};

#define SET_STRING_WITH_LEN_FIELD(field, value, len_field, len_value) \
    field = value;                                                    \
    if (!value) {                                                     \
        len_field = 0;                                                \
    } else {                                                          \
        if (len_value < 0)                                            \
            len_field = strlen(value);                                \
        else                                                          \
            len_field = (unsigned)(len_value);                        \
    }

class WiFiCredentials {
public:
    WiFiCredentials(SecurityType security = UNSEC) {
        setSecurity((WLanSecurityType)security);
    }

    WiFiCredentials(const char* ssid, SecurityType security = UNSEC) {
        setSecurity((WLanSecurityType)security);
        setSsid(ssid);
    }

    virtual ~WiFiCredentials() {}

    virtual WiFiCredentials& setSecurity(WLanSecurityType security) {
        creds_.security = (WLanSecurityType)security;
        return *this;
    }

    virtual WiFiCredentials& setSsid(const char* ssid, int ssidLen = -1) {
        SET_STRING_WITH_LEN_FIELD(creds_.ssid, ssid, creds_.ssid_len, ssidLen);
        return *this;
    }

    virtual WiFiCredentials& setCipher(WLanSecurityCipher cipher) {
        creds_.cipher = cipher;
        return *this;
    }

    virtual WiFiCredentials& setPassword(const char* password, int passwordLen = -1) {
        SET_STRING_WITH_LEN_FIELD(creds_.password, password, creds_.password_len, passwordLen);
        return *this;
    }

    virtual WiFiCredentials& setIdentity(const char* identity, int identityLen = -1) {
        return setInnerIdentity(identity, identityLen);
    }

    virtual WiFiCredentials& setInnerIdentity(const char* identity, int identityLen = -1) {
        SET_STRING_WITH_LEN_FIELD(creds_.inner_identity, identity, creds_.inner_identity_len, identityLen);
        return *this;
    }

    virtual WiFiCredentials& setOuterIdentity(const char* identity, int identityLen = -1) {
        SET_STRING_WITH_LEN_FIELD(creds_.outer_identity, identity, creds_.outer_identity_len, identityLen);
        return *this;
    }

    virtual WiFiCredentials& setEapType(WLanEapType type) {
        creds_.eap_type = type;
        return *this;
    }

    virtual WiFiCredentials& setPrivateKey(const uint8_t* pkey, uint16_t pkeyLen) {
        creds_.private_key = pkey;
        creds_.private_key_len = pkeyLen;
        return *this;
    }

    virtual WiFiCredentials& setClientCertificate(const uint8_t* cert, uint16_t certLen) {
        creds_.client_certificate = cert;
        creds_.client_certificate_len = certLen;
        return *this;
    }

    virtual WiFiCredentials& setRootCertificate(const uint8_t* cert, uint16_t certLen) {
        creds_.ca_certificate = cert;
        creds_.ca_certificate_len = certLen;
        return *this;
    }

    virtual WiFiCredentials& setPrivateKey(const char* pkey) {
        return setPrivateKey((const uint8_t*)pkey, strlen(pkey) + 1);
    }

    virtual WiFiCredentials& setClientCertificate(const char* cert) {
        return setClientCertificate((const uint8_t*)cert, strlen(cert) + 1);
    }

    virtual WiFiCredentials& setRootCertificate(const char* cert) {
        return setRootCertificate((const uint8_t*)cert, strlen(cert) + 1);
    }

    virtual WiFiCredentials& setChannel(int ch) {
        creds_.channel = ch;
        return *this;
    }

    operator WLanCredentials() {
        return getHalCredentials();
    }

    WLanCredentials getHalCredentials() {
        creds_.size = sizeof(creds_);
        creds_.version = 3;
        return creds_;
    }

    virtual void reset() {
        memset((void*)&creds_, 0, sizeof(creds_));
    }
private:
    WLanCredentials creds_ = {0};
};

class WiFiAllocatedCredentials : public WiFiCredentials {
public:
    WiFiAllocatedCredentials(SecurityType security = UNSEC)
    : WiFiCredentials(security)
    {
    }

    WiFiAllocatedCredentials(const char* ssid, SecurityType security = UNSEC)
    : WiFiCredentials(ssid, security)
    {
    }

    virtual ~WiFiAllocatedCredentials() {}

    virtual WiFiCredentials& setSsid(const char* ssid, int ssidLen = -1) {
        if (ssid && ssidLen < 0) {
            ssidLen = strlen(ssid);
        }
        if (ssid && ssidLen) {
            ssid_.reset(new (std::nothrow) char[ssidLen]);
            if (ssid_) {
                memcpy(ssid_.get(), ssid, ssidLen);
                return WiFiCredentials::setSsid(ssid_.get(), ssidLen);
            }
        }
        return *this;
    }

    virtual WiFiCredentials& setPassword(const char* password, int passwordLen = -1) {
        if (password && passwordLen < 0) {
            passwordLen = strlen(password);
        }
        if (password && passwordLen) {
            password_.reset(new (std::nothrow) char[passwordLen]);
            if (password_) {
                memcpy(password_.get(), password, passwordLen);
                return WiFiCredentials::setPassword(password_.get(), passwordLen);
            }
        }
        return *this;
    }

    virtual WiFiCredentials& setInnerIdentity(const char* identity, int identityLen = -1) {
        if (identity && identityLen < 0) {
            identityLen = strlen(identity);
        }
        if (identity && identityLen) {
            inner_identity_.reset(new (std::nothrow) char[identityLen]);
            if (inner_identity_) {
                memcpy(inner_identity_.get(), identity, identityLen);
                return WiFiCredentials::setInnerIdentity(inner_identity_.get(), identityLen);
            }
        }
        return *this;
    }

    virtual WiFiCredentials& setOuterIdentity(const char* identity, int identityLen = -1) {
        if (identity && identityLen < 0) {
            identityLen = strlen(identity);
        }
        if (identity && identityLen) {
            outer_identity_.reset(new (std::nothrow) char[identityLen]);
            if (outer_identity_) {
                memcpy(outer_identity_.get(), identity, identityLen);
                return WiFiCredentials::setOuterIdentity(outer_identity_.get(), identityLen);
            }
        }
        return *this;
    }

    virtual WiFiCredentials& setPrivateKey(const uint8_t* pkey, uint16_t pkeyLen) {
        if (pkey && pkeyLen) {
            private_key_.reset(new (std::nothrow) uint8_t[pkeyLen]);
            if (private_key_) {
                memcpy(private_key_.get(), pkey, pkeyLen);
                return WiFiCredentials::setPrivateKey(private_key_.get(), pkeyLen);
            }
        }
        return *this;
    }

    virtual WiFiCredentials& setClientCertificate(const uint8_t* cert, uint16_t certLen) {
        if (cert && certLen) {
            client_certificate_.reset(new (std::nothrow) uint8_t[certLen]);
            if (client_certificate_) {
                memcpy(client_certificate_.get(), cert, certLen);
                return WiFiCredentials::setClientCertificate(client_certificate_.get(), certLen);
            }
        }
        return *this;
    }

    virtual WiFiCredentials& setRootCertificate(const uint8_t* cert, uint16_t certLen) {
        if (cert && certLen) {
            ca_certificate_.reset(new (std::nothrow) uint8_t[certLen]);
            if (ca_certificate_) {
                memcpy(ca_certificate_.get(), cert, certLen);
                return WiFiCredentials::setRootCertificate(ca_certificate_.get(), certLen);
            }
        }
        return *this;
    }

    virtual WiFiCredentials& setPrivateKey(const char* pkey) {
        return setPrivateKey((const uint8_t*)pkey, strlen(pkey) + 1);
    }

    virtual WiFiCredentials& setClientCertificate(const char* cert) {
        return setClientCertificate((const uint8_t*)cert, strlen(cert) + 1);
    }

    virtual WiFiCredentials& setRootCertificate(const char* cert) {
        return setRootCertificate((const uint8_t*)cert, strlen(cert) + 1);
    }

    virtual void reset() {
        ssid_.reset();
        password_.reset();
        inner_identity_.reset();
        outer_identity_.reset();
        private_key_.reset();
        client_certificate_.reset();
        ca_certificate_.reset();
        WiFiCredentials::reset();
    }

private:
    std::unique_ptr<char[]> ssid_;
    std::unique_ptr<char[]> password_;
    std::unique_ptr<char[]> inner_identity_;
    std::unique_ptr<char[]> outer_identity_;
    std::unique_ptr<uint8_t[]> private_key_;
    std::unique_ptr<uint8_t[]> client_certificate_;
    std::unique_ptr<uint8_t[]> ca_certificate_;
};

} // namespace spark

#endif // Wiring_WiFi

#endif
