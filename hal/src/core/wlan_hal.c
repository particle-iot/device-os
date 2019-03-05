/**
 ******************************************************************************
 * @file    wlan_hal.c
 * @author  Matthew McGowan
 * @version V1.0.0
 * @date    25-Sept-2014
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

// Skip conflicting sockaddr declaration
#define HAL_SOCKET_HAL_COMPAT_NO_SOCKADDR (1)
#include "wlan_hal.h"
#include "delay_hal.h"
#include "timer_hal.h"
#include "socket_hal.h"
#include "socket.h"
#include "netapp.h"
#include "cc3000_common.h"
#include "cc3000_spi.h"
#include "nvmem.h"
#include "hw_config.h"
#include "spark_macros.h"
#include "security.h"
#include "evnt_handler.h"
#include "timer_hal.h"
#include <sys/param.h>
#include "system_error.h"
#include <limits.h>

/* Smart Config Prefix */
static char aucCC3000_prefix[] = {'T', 'T', 'T'};
/* AES key "sparkdevices2013" */
static const unsigned char smartconfigkey[] = "sparkdevices2013";	//16 bytes
/* device name used by smart config response */
static char device_name[] = "CC3000";

#if defined(DEBUG_WIFI)
uint32_t lastEvent = 0;
#endif

#define SMART_CONFIG_PROFILE_SIZE       67

/* CC3000 EEPROM - Spark File Data Storage */
#define NVMEM_SPARK_FILE_ID		14	//Do not change this ID
#define NVMEM_SPARK_FILE_SIZE		16	//Change according to requirement
#define WLAN_PROFILE_FILE_OFFSET	0
#define WLAN_POLICY_FILE_OFFSET		1       //Not used henceforth
#define WLAN_TIMEOUT_FILE_OFFSET	2
#define ERROR_COUNT_FILE_OFFSET		3


uint32_t HAL_NET_SetNetWatchDog(uint32_t timeOutInMS)
{
    uint32_t rv = cc3000__event_timeout_ms;
    cc3000__event_timeout_ms = timeOutInMS;
    return rv;
}

unsigned char NVMEM_Spark_File_Data[NVMEM_SPARK_FILE_SIZE];

void recreate_spark_nvmem_file();

void Clear_NetApp_Dhcp();


int wlan_clear_credentials()
{
    if(wlan_ioctl_del_profile(255) == 0)
    {
        recreate_spark_nvmem_file();
        Clear_NetApp_Dhcp();
        return 0;
    }
    return 1;
}

int wlan_has_credentials()
{
    if(NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET] != 0)
    {
        return 0;
    }
    return 1;
}

int wlan_connect_init()
{
    wlan_set_event_mask(HCI_EVNT_WLAN_KEEPALIVE | HCI_EVNT_WLAN_UNSOL_INIT);
    return 0;
}

wlan_result_t wlan_activate() {
    wlan_start(0);
    wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);
    return 0;
}

wlan_result_t wlan_deactivate() {
    wlan_stop();
    return 0;
}

bool wlan_reset_credentials_store_required()
{
	wlan_start(0);	// needed for nvmem_read
	bool has_nvmem = nvmem_read(NVMEM_SPARK_FILE_ID, NVMEM_SPARK_FILE_SIZE, 0, NVMEM_Spark_File_Data) == NVMEM_SPARK_FILE_SIZE;
    return (NVMEM_SPARK_Reset_SysFlag == 0x0001 || !has_nvmem);
}

wlan_result_t wlan_reset_credentials_store()
{
    /* Delete all previously stored wlan profiles */
    wlan_clear_credentials();
    NVMEM_SPARK_Reset_SysFlag = 0x0000;
    Save_SystemFlags();
    return 0;
}


/**
 * Do what is needed to finalize the connection.
 * @return
 */
wlan_result_t wlan_connect_finalize()
{
    // enable connection from stored profiles
    return wlan_ioctl_set_connection_policy(DISABLE, DISABLE, ENABLE);//Enable auto connect
}

void Set_NetApp_Timeout(void)
{
    unsigned long aucDHCP = 14400;
    unsigned long aucARP = 3600;
    unsigned long aucKeepalive = 10;
    unsigned long aucInactivity = DEFAULT_SEC_INACTIVITY;
    HAL_NET_SetNetWatchDog(S2M(DEFAULT_SEC_NETOPS)+ (DEFAULT_SEC_INACTIVITY ? 250 : 0) );
    netapp_timeout_values(&aucDHCP, &aucARP, &aucKeepalive, &aucInactivity);
}

void Clear_NetApp_Dhcp(void)
{
    // Clear out the DHCP settings
    unsigned long pucSubnetMask = 0;
    unsigned long pucIP_Addr = 0;
    unsigned long pucIP_DefaultGWAddr = 0;
    unsigned long pucDNS = 0;

    netapp_dhcp(&pucIP_Addr, &pucSubnetMask, &pucIP_DefaultGWAddr, &pucDNS);
}

wlan_result_t wlan_disconnect_now()
{
    wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);//Disable auto connect
    return wlan_disconnect();
}

int wlan_connected_rssi()
{
    int _returnValue = 0;

    system_tick_t _functionStart = HAL_Timer_Get_Milli_Seconds();
    while ((HAL_Timer_Get_Milli_Seconds() - _functionStart) < 1000) {
        tNetappIpconfigRetArgs config;
        netapp_ipconfig((void*)&config);
        int l;
        for (l=0; l<16; l++)
        {
            char wlan_scan_results_table[50];
            if(wlan_ioctl_get_scan_results(0, (unsigned char*)wlan_scan_results_table) != 0) {
                _returnValue = 1;
                break;
            }
            if (wlan_scan_results_table[0] == 0)
                break;
            if (!strcmp(wlan_scan_results_table+12, config.uaSSID)) {
                _returnValue = ((wlan_scan_results_table[8] >> 1) - 127);
                break;
            }
        }
        if (_returnValue != 0) {
            break;
        }
    }
    return _returnValue;
}

int wlan_connected_info(void* reserved, wlan_connected_info_t* inf, void* reserved1)
{
    system_error_t ret = SYSTEM_ERROR_NONE;

    int32_t rssi = wlan_connected_rssi();
    if (rssi >= 0) {
        // Error
        return SYSTEM_ERROR_UNKNOWN;
    }

    inf->rssi = rssi * 100;
    inf->snr = INT_MIN;
    inf->noise = INT_MIN;

    inf->strength = MIN(MAX(2 * (rssi + 100), 0L), 100L) * 65535 / 100;
    inf->quality = INT_MIN;
    return ret;
}

netapp_pingreport_args_t ping_report;
uint8_t ping_report_num;

int inet_ping(const HAL_IPAddress* ip, network_interface_t nif, uint8_t nTries, void* reserved) {
    int result = 0;
    uint32_t pingIPAddr = ip->ipv4;
    unsigned long pingSize = 32UL;
    unsigned long pingTimeout = 500UL; // in milliseconds

    memset(&ping_report,0,sizeof(netapp_pingreport_args_t));
    ping_report_num = 0;

    long psend = netapp_ping_send(&pingIPAddr, (unsigned long)nTries, pingSize, pingTimeout);
    system_tick_t lastTime = HAL_Timer_Get_Milli_Seconds();
    while( ping_report_num==0 && (HAL_Timer_Get_Milli_Seconds() < lastTime+2*nTries*pingTimeout)) {}
    if (psend==0L && ping_report_num) {
        result = (int)ping_report.packets_received;
    }
    return result;
}

int wlan_set_credentials_internal(const char *ssid, uint16_t ssidLen, const char *password,
    uint16_t passwordLen, WLanSecurityType security)
{
    int wlan_profile_index = -1;
  // add a profile
  switch (security)
  {
  case WLAN_SEC_UNSEC://None
    {
      wlan_profile_index = wlan_add_profile(WLAN_SEC_UNSEC,   // Security type
        (unsigned char *)ssid,                                // SSID
        ssidLen,                                              // SSID length
        NULL,                                                 // BSSID
        1,                                                    // Priority
        0, 0, 0, 0, 0);

      break;
    }

  case WLAN_SEC_WEP://WEP
    {
        char buf[32];

        // Get WEP key from string, needs converting
        passwordLen = passwordLen / 2;
        char byteStr[3];
        byteStr[2] = '\0';
        memset(buf, 0, sizeof (buf));
        unsigned i;
        for (i = 0; i < passwordLen; i++)
        { // Basic loop to convert text-based WEP key to byte array, can definitely be improved
            byteStr[0] = password[2 * i];
            byteStr[1] = password[(2 * i) + 1];
            buf[i] = strtoul(byteStr, NULL, 16);
        }
        password = buf;

     wlan_profile_index = wlan_add_profile(WLAN_SEC_WEP,    // Security type
        (unsigned char *)ssid,                                // SSID
        ssidLen,                                              // SSID length
        NULL,                                                 // BSSID
        1,                                                    // Priority
        passwordLen,                                          // KEY length
        0,                                                    // KEY index
        0,
        (unsigned char *)password,                            // KEY
        0);

      break;
    }

  case WLAN_SEC_WPA://WPA
  case WLAN_SEC_WPA2://WPA2
    {
      wlan_profile_index = wlan_add_profile(WLAN_SEC_WPA2,    // Security type
        (unsigned char *)ssid,                                // SSID
        ssidLen,                                              // SSID length
        NULL,                                                 // BSSID
        1,                                                    // Priority
        0x18,                                                 // PairwiseCipher
        0x1e,                                                 // GroupCipher
        2,                                                    // KEY management
        (unsigned char *)password,                            // KEY
        passwordLen);                                         // KEY length

      break;
    }
    default:
          return WLAN_SET_CREDENTIALS_UNKNOWN_SECURITY_TYPE;
  }

  if(wlan_profile_index != -1)
  {
    NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET] = wlan_profile_index + 1;
  }

  /* write count of wlan profiles stored */
  nvmem_write(NVMEM_SPARK_FILE_ID, 1, WLAN_PROFILE_FILE_OFFSET, &NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET]);
  return 0;
}

int wlan_set_credentials(WLanCredentials* c)
{
    return wlan_set_credentials_internal(c->ssid, c->ssid_len, c->password, c->password_len, c->security);
}


/**
 * Rebuilds the eeprom storage file.
 */
void recreate_spark_nvmem_file(void)
{
    // Spark file IO on old TI Driver was corrupting nvmem
    // so remove the entry for Spark file in CC3000 EEPROM
    nvmem_create_entry(NVMEM_SPARK_FILE_ID, 0);

    // Create new entry for Spark File in CC3000 EEPROM
    nvmem_create_entry(NVMEM_SPARK_FILE_ID, NVMEM_SPARK_FILE_SIZE);

    // Zero out our array copy of the EEPROM
    memset(NVMEM_Spark_File_Data, 0, NVMEM_SPARK_FILE_SIZE);

    // Write zeroed-out array into the EEPROM
    nvmem_write(NVMEM_SPARK_FILE_ID, NVMEM_SPARK_FILE_SIZE, 0, NVMEM_Spark_File_Data);
}

void wlan_smart_config_init() {

    /* Create new entry for AES encryption key */
    nvmem_create_entry(NVMEM_AES128_KEY_FILEID,16);

    /* Write AES key to NVMEM */
    aes_write_key((unsigned char *)(&smartconfigkey[0]));

    wlan_smart_config_set_prefix((char*)aucCC3000_prefix);

    /* Start the SmartConfig start process */
    wlan_smart_config_start(1);
}

bool wlan_smart_config_finalize() {
    /* read count of wlan profiles stored */
    nvmem_read(NVMEM_SPARK_FILE_ID, 1, WLAN_PROFILE_FILE_OFFSET, &NVMEM_Spark_File_Data[WLAN_PROFILE_FILE_OFFSET]);
    return false;
}


/* WLAN Application related callbacks passed to wlan_init */
void WLAN_Async_Callback(long lEventType, char *data, unsigned char length)
{
    SET_LAST_EVENT(lEventType);
    switch (lEventType)
    {
    default:
        break;
    case HCI_EVNT_WLAN_ASYNC_SIMPLE_CONFIG_DONE:
        HAL_WLAN_notify_simple_config_done();
        break;

    case HCI_EVNT_WLAN_UNSOL_CONNECT:
        HAL_NET_notify_connected();
        break;

    case HCI_EVNT_WLAN_UNSOL_DISCONNECT:
        HAL_NET_notify_disconnected();
        break;

    case HCI_EVNT_WLAN_UNSOL_DHCP:
        HAL_NET_notify_dhcp(*(data + 20) == 0);
        break;

    case HCI_EVENT_CC3000_CAN_SHUT_DOWN:
        HAL_NET_notify_can_shutdown();
        break;

    case HCI_EVNT_WLAN_ASYNC_PING_REPORT:
        memcpy(&ping_report,data,length);
        ping_report_num++;
        break;

    case HCI_EVNT_BSD_TCP_CLOSE_WAIT:
        {
            sock_handle_t socket = -1;
            STREAM_TO_UINT32(data,0,socket);
            set_socket_active_status(socket, SOCKET_STATUS_INACTIVE);
            HAL_NET_notify_socket_closed(socket);
            break;
        }
    }
}

char *WLAN_Firmware_Patch(unsigned long *length)
{
	*length = 0;
	return NULL;
}

char *WLAN_Driver_Patch(unsigned long *length)
{
	*length = 0;
	return NULL;
}

char *WLAN_BootLoader_Patch(unsigned long *length)
{
	*length = 0;
	return NULL;
}

void wlan_smart_config_cleanup()
{
    unsigned char loop_index = 0;

    while (loop_index < 3)
    {
        mdnsAdvertiser(1,device_name,strlen(device_name));
        loop_index++;
    }
}


void wlan_setup()
{
    /* Initialize CC3000's CS, EN and INT pins to their default states */
    CC3000_WIFI_Init();

    /* Configure & initialize CC3000 SPI_DMA Interface */
    CC3000_SPI_DMA_Init();

    /* WLAN On API Implementation */
    wlan_init(WLAN_Async_Callback, WLAN_Firmware_Patch, WLAN_Driver_Patch, WLAN_BootLoader_Patch,
                            CC3000_Read_Interrupt_Pin, CC3000_Interrupt_Enable, CC3000_Interrupt_Disable, CC3000_Write_Enable_Pin);

    HAL_Delay_Microseconds(100);
}


void wlan_set_error_count(uint32_t errorCount)
{
    NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET] = errorCount;
    nvmem_write(NVMEM_SPARK_FILE_ID, 1, ERROR_COUNT_FILE_OFFSET, &NVMEM_Spark_File_Data[ERROR_COUNT_FILE_OFFSET]);
}

int wlan_fetch_ipconfig(WLanConfig* config) {
    HAL_Delay_Milliseconds(100);

    // the WLanConfig and the CC3000 structure are identical
    netapp_ipconfig((void*)&config->nw);
    // the MAC address isn't available until after the first WLAN connection is made, so fetch it from nvmem
    nvmem_get_mac_address(config->nw.uaMacAddr);

    return 0;
}

void SPARK_WLAN_SmartConfigProcess()
{
    unsigned int ssidLen, keyLen;
    unsigned char *decKeyPtr;
    unsigned char *ssidPtr;
    extern unsigned char profileArray[];

    // read the received data from fileID #13 and parse it according to the followings:
    // 1) SSID LEN - not encrypted
    // 2) SSID - not encrypted
    // 3) KEY LEN - not encrypted. always 32 bytes long
    // 4) Security type - not encrypted
    // 5) KEY - encrypted together with true key length as the first byte in KEY
    //       to elaborate, there are two corner cases:
    //              1) the KEY is 32 bytes long. In this case, the first byte does not represent KEY length
    //              2) the KEY is 31 bytes long. In this case, the first byte represent KEY length and equals 31
    if(SMART_CONFIG_PROFILE_SIZE != nvmem_read(NVMEM_SHARED_MEM_FILEID, SMART_CONFIG_PROFILE_SIZE, 0, profileArray))
    {
      return;
    }

    ssidPtr = &profileArray[1];

    ssidLen = profileArray[0];

    decKeyPtr = &profileArray[profileArray[0] + 3];

    UINT8 expandedKey[176];
    aes_decrypt(decKeyPtr, (unsigned char *)smartconfigkey, expandedKey);
    if (profileArray[profileArray[0] + 1] > 16)
    {
      aes_decrypt((UINT8 *)(decKeyPtr + 16), (unsigned char *)smartconfigkey, expandedKey);
    }

    if (*(UINT8 *)(decKeyPtr +31) != 0)
    {
            if (*decKeyPtr == 31)
            {
                    keyLen = 31;
                    decKeyPtr++;
            }
            else
            {
                    keyLen = 32;
            }
    }
    else
    {
            keyLen = *decKeyPtr;
            decKeyPtr++;
    }

    wlan_set_credentials_internal((const char*)ssidPtr, ssidLen, (const char*)decKeyPtr, keyLen, profileArray[profileArray[0] + 2]);
}

void wlan_connect_cancel(bool called_from_isr)
{
    // no-op cc3000 connection is asynchronous
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
    // Unsupported
    return -1;
}

typedef struct CC3000ScanResult {
    uint32_t networks;
    uint32_t status;
    uint8_t rssi;
    uint8_t security_ssidlen;
    uint16_t time;
    uint8_t ssid[32];
    uint8_t bssid[6];
} CC3000ScanResult;


int wlan_scan(wlan_scan_result_t callback, void* cookie)
{
   unsigned long intervalTime[16] = { 2000, 2000, 2000, 2000,  2000,
        2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000 };

    wlan_ioctl_set_scan_params(4000, 20, 100, 5, 0x7FF, -120, 0, 300, intervalTime);

    CC3000ScanResult result;
    memset(&result, 0, sizeof(result));

    int count = 0;
    int err = wlan_ioctl_get_scan_results(0, (uint8_t*)&result);
    while (!err) {
        int sum = 0;
        for (int i=0; i<6; i++) {
            sum += result.bssid[i];
        }
        if (sum==0) // bssid not set
            break;

        WiFiAccessPoint out;
        memset(&out, 0, sizeof(out));
        out.security = result.security_ssidlen & 3;
        out.ssidLength = result.security_ssidlen >> 2;
        out.rssi = -(result.rssi >> 1);
        memcpy(out.ssid, result.ssid, out.ssidLength);
        out.ssid[out.ssidLength] = 0;
        memcpy(out.bssid, result.bssid, sizeof(out.bssid));
        callback(&out, cookie);
        count++;
        memset(result.bssid, 0, sizeof(result.bssid));
        err = wlan_ioctl_get_scan_results(0, (uint8_t*)&result);
    }
    return err < 0 ? err : count;
}

/**
 * Lists all WLAN credentials currently stored on the device
 */
int wlan_get_credentials(wlan_scan_result_t callback, void* callback_data)
{
    // Reading credentials from the CC3000 is not possible
    return 0;
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
