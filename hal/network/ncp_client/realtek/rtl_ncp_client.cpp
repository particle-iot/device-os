/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#define NO_STATIC_ASSERT
#include "logging.h"
LOG_SOURCE_CATEGORY("ncp.rltk.client");

#include "rtl_ncp_client.h"

#include "gpio_hal.h"
#include "timer_hal.h"
#include "delay_hal.h"

#include "check.h"

#include <cstdlib>

#include "wifi_conf.h"
#include "wifi_constants.h"
#include "lwip_rltk.h"

#undef ON
#undef OFF
extern "C" {
#include "hal_platform_rtl.h"
#include "rtl8721dlp_sysreg.h"
#include "rtl8721dhp_sysreg.h"
#include "rtl8721dhp_rcc.h"
#include "rtl8721d.h"
} // extern "C"

extern "C" void rtw_efuse_boot_write(void);

namespace particle {

namespace {

} // unnamed

RealtekNcpClient::RealtekNcpClient() :
        ncpState_(NcpState::OFF),
        prevNcpState_(NcpState::OFF),
        connState_(NcpConnectionState::DISCONNECTED) {
}

RealtekNcpClient::~RealtekNcpClient() {
    destroy();
}

int RealtekNcpClient::init(const NcpClientConfig& conf) {
    // rltkOff();

    conf_ = conf;
    ncpState_ = NcpState::OFF;
    prevNcpState_ = NcpState::OFF;
    connState_ = NcpConnectionState::DISCONNECTED;
    pwrState_ = NcpPowerState::OFF;
    // We know for a fact that ESP32 is off on boot because we've initialized ESPEN pin to output 0
    ncpPowerState(NcpPowerState::OFF);
    return 0;
}

void RealtekNcpClient::destroy() {
    if (ncpState_ != NcpState::OFF) {
        ncpState_ = NcpState::OFF;
        rltkOff();
    }
}

int RealtekNcpClient::on() {
    const NcpClientLock lock(this);
    if (ncpState_ == NcpState::DISABLED) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (ncpState_ == NcpState::ON) {
        return 0;
    }
    ncpPowerState(NcpPowerState::TRANSIENT_ON);
    CHECK(rltkOn());
    ncpState(NcpState::ON);
    return 0;
}

int RealtekNcpClient::off() {
    const NcpClientLock lock(this);
    if (ncpState_ == NcpState::DISABLED) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    ncpPowerState(NcpPowerState::TRANSIENT_OFF);
    rltkOff();
    ncpState(NcpState::OFF);
    ncpPowerState(NcpPowerState::OFF);
    return 0;
}

int RealtekNcpClient::enable() {
    const NcpClientLock lock(this);
    if (ncpState_ != NcpState::DISABLED) {
        return 0;
    }
    ncpState_ = prevNcpState_;
    off();
    return 0;
}

void RealtekNcpClient::disable() {
    // This method is used to unblock the network interface thread, so we're not trying to acquire
    // the client lock here
    const NcpState state = ncpState_;
    if (state == NcpState::DISABLED) {
        return;
    }
    prevNcpState_ = state;
    ncpState_ = NcpState::DISABLED;
}

NcpState RealtekNcpClient::ncpState() {
    return ncpState_;
}

NcpPowerState RealtekNcpClient::ncpPowerState() {
    return pwrState_;
}

int RealtekNcpClient::disconnect() {
    const NcpClientLock lock(this);
    if (ncpState_ == NcpState::DISABLED) {
        return SYSTEM_ERROR_INVALID_STATE;
    }
    if (connState_ == NcpConnectionState::DISCONNECTED) {
        return 0;
    }
    wifi_disconnect();
    connectionState(NcpConnectionState::DISCONNECTED);
    return 0;
}

NcpConnectionState RealtekNcpClient::connectionState() {
    return connState_;
}

int RealtekNcpClient::connect(const char* ssid, const MacAddress& bssid, WifiSecurity sec, const WifiCredentials& cred) {
    const NcpClientLock lock(this);
    scan(nullptr, nullptr);

    CHECK_TRUE(connState_ == NcpConnectionState::DISCONNECTED, SYSTEM_ERROR_INVALID_STATE);
    LOG(INFO, "connecting");
    volatile uint32_t rtlContinue = 1;
    while (!rtlContinue) {
        asm volatile ("nop");
    }

    char ssid_name[] = "PCN";
    char passwd[] = "makeitparticle!";
    int security = RTW_SECURITY_WPA_WPA2_MIXED_PSK;

    char mac[32] = {};
    wifi_get_mac_address(mac);
    int r = -1;
    for (int i = 0; i < 3; i++) {
        LOG(INFO, "AAA: try to connect to ssid: %s, passwd: %s, mac: %s", ssid_name, passwd, mac);
        r = wifi_connect(ssid_name, security, passwd, strlen(ssid_name), strlen(passwd), -1, nullptr);
        LOG(INFO, "BBB: connect result %d", r);
        if (r == 0) {
            break;
        }
        HAL_Delay_Milliseconds(1000);
    }

    connectionState(NcpConnectionState::CONNECTED);
    return r;
}

int RealtekNcpClient::getNetworkInfo(WifiNetworkInfo* info) {
    return SYSTEM_ERROR_NOT_SUPPORTED;
}

int RealtekNcpClient::scan(WifiScanCallback callback, void* data) {
    volatile uint32_t rtlContinue = 1;
    while (!rtlContinue) {
        asm volatile ("nop");
    }
    volatile uint32_t done = 0;
    LOG(INFO, "primask = %d", __get_PRIMASK());
    auto r = wifi_scan_networks([](rtw_scan_handler_result_t* malloced_scan_result) -> rtw_result_t {
        LOG(INFO, "scan callback");
        volatile uint32_t* done = (volatile uint32_t*)malloced_scan_result->user_data;
        if (malloced_scan_result->scan_complete != RTW_TRUE) {
            rtw_scan_result_t* record = &malloced_scan_result->ap_details;
            record->SSID.val[record->SSID.len] = 0; /* Ensure the SSID is null terminated */
            LOG(INFO, "AP");
            LOG(INFO, "%s\t ", ( record->bss_type == RTW_BSS_TYPE_ADHOC ) ? "Adhoc" : "Infra" );
            LOG(INFO, MAC_FMT, MAC_ARG(record->BSSID.octet) );
            LOG(INFO, " %d\t ", record->signal_strength );
            LOG(INFO, " %d\t  ", record->channel );
            LOG(INFO, " %d\t  ", record->wps_type );
            LOG(INFO, "%s\t\t ", ( record->security == RTW_SECURITY_OPEN ) ? "Open" :
                                            ( record->security == RTW_SECURITY_WEP_PSK ) ? "WEP" :
                                            ( record->security == RTW_SECURITY_WPA_TKIP_PSK ) ? "WPA TKIP" :
                                            ( record->security == RTW_SECURITY_WPA_AES_PSK ) ? "WPA AES" :
                                            ( record->security == RTW_SECURITY_WPA_MIXED_PSK ) ? "WPA Mixed" :
                                            ( record->security == RTW_SECURITY_WPA2_AES_PSK ) ? "WPA2 AES" :
                                            ( record->security == RTW_SECURITY_WPA2_TKIP_PSK ) ? "WPA2 TKIP" :
                                            ( record->security == RTW_SECURITY_WPA2_MIXED_PSK ) ? "WPA2 Mixed" :
                                            ( record->security == RTW_SECURITY_WPA_WPA2_TKIP_PSK) ? "WPA/WPA2 TKIP" :
                                            ( record->security == RTW_SECURITY_WPA_WPA2_AES_PSK) ? "WPA/WPA2 AES" :
                                            ( record->security == RTW_SECURITY_WPA_WPA2_MIXED_PSK) ? "WPA/WPA2 Mixed" :
            #ifdef CONFIG_SAE_SUPPORT
                                            ( record->security == RTW_SECURITY_WPA3_AES_PSK) ? "WP3-SAE AES" :
            #endif
                                            "Unknown" );

            LOG(INFO, " %s ", record->SSID.val );
            LOG(INFO, "\r\n" );
        } else {
            *done = 1;
        }
        return RTW_SUCCESS;
    }, (void*)&done);
    while (!done) {
        HAL_Delay_Milliseconds(100);
    }
    LOG(INFO, "scan done %d", r);
    return 0;
}

int RealtekNcpClient::getMacAddress(MacAddress* addr) {
    addr->data[0] = 0xaa;
    addr->data[1] = 0xbb;
    addr->data[2] = 0xcc;
    addr->data[3] = 0xdd;
    addr->data[4] = 0xee;
    addr->data[5] = 0xfe;
    /* Drop 'multicast' bit */
    addr->data[0] &= 0b11111110;
    /* Set 'locally administered' bit */
    addr->data[0] |= 0b10;
    return 0;
}


void RealtekNcpClient::ncpState(NcpState state) {
    if (ncpState_ == NcpState::DISABLED) {
        return;
    }
    if (state == NcpState::OFF) {
        connectionState(NcpConnectionState::DISCONNECTED);
    }

    if (ncpState_ == state) {
        return;
    }
    ncpState_ = state;
    LOG(TRACE, "NCP state changed: %d", (int)ncpState_);

    const auto handler = conf_.eventHandler();
    if (handler) {
        NcpStateChangedEvent event = {};
        event.type = NcpEvent::NCP_STATE_CHANGED;
        event.state = ncpState_;
        handler(event, conf_.eventHandlerData());
    }
}

void RealtekNcpClient::ncpPowerState(NcpPowerState state) {
    if (pwrState_ == state) {
        return;
    }
    pwrState_ = state;
    const auto handler = conf_.eventHandler();
    if (handler) {
        NcpPowerStateChangedEvent event = {};
        event.type = NcpEvent::POWER_STATE_CHANGED;
        event.state = pwrState_;
        handler(event, conf_.eventHandlerData());
    }
}

void RealtekNcpClient::connectionState(NcpConnectionState state) {
    if (ncpState_ == NcpState::DISABLED) {
        return;
    }
    if (connState_ == state) {
        return;
    }
    LOG(TRACE, "NCP connection state changed: %d", (int)state);
    connState_ = state;
    const auto handler = conf_.eventHandler();
    if (handler) {
        NcpConnectionStateChangedEvent event = {};
        event.type = NcpEvent::CONNECTION_STATE_CHANGED;
        event.state = connState_;
        handler(event, conf_.eventHandlerData());
    }
}

int RealtekNcpClient::rltkOff() {
    LOG(INFO, "rltkOff");
    wifi_off();
    LOG(INFO, "rltkOff done");
    ncpPowerState(NcpPowerState::OFF);
    return 0;
}


int RealtekNcpClient::rltkOn() {
    LOG(INFO, "rltkOn");
    rtw_efuse_boot_write();
    volatile uint32_t rtlContinue = 1;
    while (!rtlContinue) {
        asm volatile ("nop");
    }
    RCC_PeriphClockCmd(APBPeriph_WL, APBPeriph_WL_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_GDMA0, APBPeriph_GDMA0_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_LCDC, APBPeriph_LCDC_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_I2S0, APBPeriph_I2S0_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_SECURITY_ENGINE, APBPeriph_SEC_ENG_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_LXBUS, APBPeriph_LXBUS_CLOCK, ENABLE);
    wifi_on(RTW_MODE_STA);
    LOG(INFO, "rltkOn done");
    ncpPowerState(NcpPowerState::ON);
    return 0;
}

int RealtekNcpClient::getFirmwareVersionString(char* buf, size_t size) {
    return SYSTEM_ERROR_NONE;
}
int RealtekNcpClient::getFirmwareModuleVersion(uint16_t* ver) {
    *ver = 1;
    return SYSTEM_ERROR_NONE;
}
int RealtekNcpClient::updateFirmware(InputStream* file, size_t size) {
    return SYSTEM_ERROR_NONE;
}
int RealtekNcpClient::dataChannelWrite(int id, const uint8_t* data, size_t size) {
	// struct eth_drv_sg sg_list[MAX_ETH_DRV_SG];
	// int sg_len = 0;
	// for (struct pbuf* q = p; q != NULL && sg_len < MAX_ETH_DRV_SG; q = q->next) {
	// 	sg_list[sg_len].buf = (unsigned int) q->payload;
	// 	sg_list[sg_len++].len = q->len;
	// }
    (void) id;

    struct eth_drv_sg sg_list[1];
    int sg_len = 1;
    sg_list[0].buf = (unsigned int)data;
    sg_list[0].len = size;

    // LOG(INFO, "lwip output, size: %ld, sg_len: %d, p->if_idx: %d", size, sg_len, p->if_idx);
    LOG(INFO, "lwip output, size: %ld, sg_len: %d", size, sg_len);

	if (sg_len) {
        if (rltk_wlan_send(0, sg_list, sg_len, size) == 0) {
            return SYSTEM_ERROR_NONE;
        } else {
            LOG(INFO, "rltk_wlan_send ERROR!!!, size: %d", size);
            return SYSTEM_ERROR_INTERNAL;	// return a non-fatal error
        }
    }

    return SYSTEM_ERROR_NONE;
}

int RealtekNcpClient::dataChannelFlowControl(bool state) {
    return SYSTEM_ERROR_NONE;
}
void RealtekNcpClient::processEvents() {
}
int RealtekNcpClient::checkParser() {
    return SYSTEM_ERROR_NONE;
}
AtParser* RealtekNcpClient::atParser() {
    return nullptr;
}

} // particle
