/**
 ******************************************************************************
 * @file    wlan_hal.c
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    27-Sept-2014
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


#include "wlan_hal.h"
#include "delay_hal.h"
#include "core_msg.h"
#include <string.h>
#include "service_debug.h"
#include <lwip/tcpip.h>
#include <mutex>
#include <lwip/netifapi.h>
#include "ppp_client.h"
#include "usart_hal.h"
#include "concurrent_hal.h"

using namespace particle::net::ppp;

namespace {
class UsartDataChannel : public DataChannel {
public:
  UsartDataChannel(HAL_USART_Serial serial, uint32_t baudrate = 115200)
      : serial_(serial),
        baudrate_(baudrate) {
    HAL_USART_Init(serial_, &rx_, &tx_);
    HAL_USART_Begin(serial_, baudrate);
  }

  virtual int readSome(char* data, size_t size) {
    size_t pos = 0;
    while (HAL_USART_Available_Data(serial_) > 0 && pos < size) {
      char c = HAL_USART_Read_Data(serial_);
      data[pos++] = c;
    }
    return pos;
  }

  virtual int writeSome(const char* data, size_t size) override {
    size_t pos = 0;
    while (HAL_USART_Available_Data_For_Write(serial_) > 0 && pos < size) {
      HAL_USART_Write_NineBitData(serial_, data[pos++]);
    }
    return pos;
  }

  virtual int waitEvent(unsigned flags, unsigned timeout = 0) override {
    system_tick_t now = HAL_Timer_Get_Milli_Seconds();
    unsigned retFlags = 0;
    do {
      if (flags & READABLE) {
        if (HAL_USART_Available_Data(serial_) > 0) {
          retFlags |= READABLE;
        }
      }
      if (flags & WRITABLE) {
        if (HAL_USART_Available_Data_For_Write(serial_) > 0) {
          retFlags |= WRITABLE;
        }
      }
      if (flags && retFlags == 0 && timeout > 0) {
        os_thread_yield();
      }
    } while (flags && retFlags == 0 && HAL_Timer_Get_Milli_Seconds() - now < timeout);

    return retFlags;
  }

private:
  HAL_USART_Serial serial_;
  uint32_t baudrate_;
  Ring_Buffer rx_;
  Ring_Buffer tx_;
};

} /* anonymous */

static Client* s_client = nullptr;
static UsartDataChannel* s_datachannel = nullptr;

static void s_ppp_client_event_handler(Client* c, uint64_t ev, void* ctx) {
  if (ev == Client::EVENT_UP) {
    HAL_NET_notify_connected();
    HAL_NET_notify_dhcp(true);
  } else {
    HAL_NET_notify_disconnected();
  }
}

uint32_t HAL_NET_SetNetWatchDog(uint32_t timeOutInMS)
{
    return 0;
}

int wlan_clear_credentials()
{
    return 0;
}

int wlan_has_credentials()
{
    return 0;
}

int wlan_connect_init()
{
    INFO("Virtual WLAN connecting");
    return 0;
}

wlan_result_t wlan_activate() {
    INFO("Virtual WLAN on");
    static std::once_flag f;
    std::call_once(f, [](){
      tcpip_init([](void* arg) {
        LOG(TRACE, "LwIP started");
      }, /* &sem */ nullptr);
      /* FIXME */
      HAL_Delay_Milliseconds(1000);

      s_client = new Client();
      s_datachannel = new UsartDataChannel(HAL_USART_SERIAL1);
      s_client->setDataChannel(s_datachannel);
      s_client->setNotifyCallback(&s_ppp_client_event_handler, nullptr);
      s_client->start();
      s_client->notifyEvent(Client::EVENT_LOWER_UP);
    });

    return 0;
}

wlan_result_t wlan_deactivate() {
    INFO("Virtual WLAN off");
    return 0;
}

bool wlan_reset_credentials_store_required()
{
    return false;
}

wlan_result_t wlan_reset_credentials_store()
{
    return 0;
}

/**
 * Do what is needed to finalize the connection.
 * @return
 */
wlan_result_t wlan_connect_finalize()
{
    s_client->connect();
    return 0;
}


void Set_NetApp_Timeout(void)
{
}

wlan_result_t wlan_disconnect_now()
{
    s_client->disconnect();
    return 0;
}

wlan_result_t wlan_connected_rssi()
{
    return 0;
}

int wlan_connected_info(void* reserved, wlan_connected_info_t* inf, void* reserved1)
{
    return -1;
}

int wlan_set_credentials(WLanCredentials* c)
{
  return -1;
}

void wlan_smart_config_init() {

}

bool wlan_smart_config_finalize() {
    return false;
}



void wlan_smart_config_cleanup()
{
}


void wlan_setup()
{
    INFO("Virtual WLAN init");
}


void wlan_set_error_count(uint32_t errorCount)
{
}

void wlan_fetch_ipconfig(WLanConfig* config)
{

/*
    memcpy(config->nw.aucIP, "\xC0\x0\x1\x68", 4);
    memcpy(config->nw.aucSubnetMask, "\xFF\xFF\xFF\x0", 4);
    memcpy(config->nw.aucDefaultGateway, "\xC0\x0\x1\x1", 4);
    memcpy(config->nw.aucDHCPServer, "\xC0\x0\x1\x1", 4);
    memcpy(config->nw.aucDNSServer, "\xC0\x0\x1\x1", 4);
    memcpy(config->nw.uaMacAddr, "\x08\x00\x27\x00\x7C\xAC", 6);
    memcpy(config->uaSSID, "WLAN", 5);
 */
}

void SPARK_WLAN_SmartConfigProcess()
{
}

void wlan_connect_cancel(bool called_from_isr)
{
}

int wlan_select_antenna(WLanSelectAntenna_TypeDef antenna)
{
    return 0;
}

WLanSelectAntenna_TypeDef wlan_get_antenna(void* reserved)
{
    return ANT_NONE;
}


bool fetch_or_generate_setup_ssid(void* SSID)
{
    return false;
}



/**
 * Sets the IP source - static or dynamic.
 */
void wlan_set_ipaddress_source(IPAddressSource source, bool persist, void* reserved)
{
}

/**
 * Sets the IP Addresses to use when the device is in static IP mode.
 * @param device
 * @param netmask
 * @param gateway
 * @param dns1
 * @param dns2
 * @param reserved
 */
void wlan_set_ipaddress(const HAL_IPAddress* device, const HAL_IPAddress* netmask,
        const HAL_IPAddress* gateway, const HAL_IPAddress* dns1, const HAL_IPAddress* dns2, void* reserved)
{
}

IPAddressSource wlan_get_ipaddress_source(void* reserved)
{
    return DYNAMIC_IP;
}

int wlan_get_ipaddress(IPConfig* conf, void* reserved)
{
    return -1;
}

int wlan_scan(wlan_scan_result_t callback, void* cookie)
{
    return -1;
}

int wlan_get_credentials(wlan_scan_result_t callback, void* callback_data)
{
	return -1;
}

int wlan_restart(void* reserved)
{
  return -1;
}

int wlan_get_hostname(char* buf, size_t len, void* reserved)
{
    // Unsupported
    if (buf) {
        buf[0] = '\0';
    }
    return -1;
}

int wlan_set_hostname(const char* hostname, void* reserved)
{
    // Unsupported
    return -1;
}
