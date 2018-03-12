
#include <cstdlib>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "wiced.h"
#include "wlan_hal.h"
#include "rgbled.h"
#include "dct.h"
#include "deviceid_hal.h"
#include "spark_macros.h"
#include "gpio_hal.h"
#include "core_hal.h"
#include "core_hal_stm32f2xx.h"
#include "hci_usart_hal.h"
#include "btstack_hal.h"
#include "debug.h"
#include "ble_provision.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
#define BLE_TX_RX_STREAM_MAX              128
#define TX_RX_STREAM_CMD_IDX              1
#define TX_RX_STREAM_LEN_IDX              0

#define CONFIG_AP_ENTRY_LEN_MIN           (9)

#define BLE_DEVICE_NAME_MAX_LEN            20

/******************************************************
 *                   Enumerations
 ******************************************************/
typedef enum {
    GapStatus_Connected,
    GapStatus_Disconnect,
} GapStatus;

typedef enum {
    PROVISION_CMD_SCAN_REQUEST = 0xA0,
    PROVISION_CMD_CONFIG_AP_ENTRY,
    PROVISION_CMD_CONNECT_AP,
    PROVISION_CMD_NOTIFY_AP,
    PROVISION_CMD_NOTIFY_SYS_INFO,
    PROVISION_CMD_NOTIFY_IP_CONFIG,
} BLEProvisionCmd_t;

typedef enum {
    PROVISION_STA_IDLE = 0xB0,
    PROVISION_STA_SCANNING,
    PROVISION_STA_SCAN_COMPLETE,
    PROVISION_STA_CONFIG_AP,
    PROVISION_STA_CONNECTING,
    PROVISION_STA_CONNECTED,
    PROVISION_STA_CONNECT_FAILED,
} BLEProvisionSta_t;

typedef enum {
    CMD_PIPE_AVAILABLE,
    CMD_PIPE_BUSY_WRITE,
    CMD_PIPE_BUSY_NOTIFY,
} Cmd_pipe_state_t;

typedef enum {
    AP_CONFIGURED = 0xD0,
    AP_SCANNED,
} BLEProvisionAPState_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/
typedef struct {
    uint8_t                channel;
    wiced_security_t       security;
    uint8_t                SSID_len;
    uint8_t                SSID[SSID_NAME_SIZE];
    uint8_t                security_key_length;
    char                   security_key[ SECURITY_KEY_SIZE ];
} Config_ap_entry_t;

/******************************************************
 *               Function Declarations
 ******************************************************/
static void ble_provision_init_variables(void);
static void ble_provision_notify(uint16_t attr_handle, uint8_t *pbuf, uint8_t len);
static void ble_provision_send_ap_details(wiced_scan_result_t* record);
static void ble_provision_send_ip_config(void);
static void ble_provision_send_sys_info(void);
static void ble_provision_parse_cmd(uint8_t *stream, uint8_t stream_len);

static wiced_result_t ble_provision_scan_result_handler(wiced_scan_handler_result_t* malloced_scan_result);
static void deviceConnectedCallback(BLEStatus_t status, uint16_t handle);
static void deviceDisconnectedCallback(uint16_t handle);
static int gattWriteCallback(uint16_t value_handle, uint8_t *new_value, uint16_t new_value_len);
static uint16_t gattReadCallback(uint16_t value_handle, uint8_t * buffer, uint16_t buffer_size);

#ifdef __cplusplus
extern "C" {
#endif
bool fetch_or_generate_setup_ssid(wiced_ssid_t* SSID);
wiced_result_t add_wiced_wifi_credentials(const char *ssid, uint16_t ssidLen, const char *password,
        uint16_t passwordLen, wiced_security_t security, unsigned channel);
#ifdef __cplusplus
}
#endif


/******************************************************
 *               Variable Definitions
 ******************************************************/
static uint8_t  BLE_PROVISION_SERVICE_UUID[16]  = { 0x3E,0xC6,0x14,0x00,0x89,0xCD,0x49,0xC3,0xA0,0xD9,0x7A,0x85,0x66,0x9E,0x90,0x1E };
static uint8_t  BLE_PROVISION_CMD_CHAR_UUID[16] = { 0x3E,0xC6,0x14,0x01,0x89,0xCD,0x49,0xC3,0xA0,0xD9,0x7A,0x85,0x66,0x9E,0x90,0x1E };
static uint8_t  BLE_PROVISION_STA_CHAR_UUID[16] = { 0x3E,0xC6,0x14,0x02,0x89,0xCD,0x49,0xC3,0xA0,0xD9,0x7A,0x85,0x66,0x9E,0x90,0x1E };

static uint8_t  ble_device_name[BLE_DEVICE_NAME_MAX_LEN] = { 0x00 };
static uint8_t  appearance[2]      = { 0x00, 0x02 };
static uint8_t  change[2]          = { 0x00, 0x00 };
static uint8_t  conn_param[8]      = { 0x28, 0x00, 0x90, 0x01, 0x00, 0x00, 0x90, 0x01 };

/* BLE connection variable */
static uint16_t     connect_handle = 0xFFFF;
static uint16_t     command_value_handle = 0x0000;
static uint16_t     status_value_handle = 0x0000;

static GapStatus    connect_status = GapStatus_Disconnect;

/* GATT attribute values */
static uint8_t      command_value[20];
static uint8_t      command_value_len;
static uint16_t     command_notify_flag = 0x0000;
static uint8_t      status_value[1];
static uint16_t     status_notify_flag = 0x0000;

//Advertising Data.
static uint8_t adv_data[31] = {
    0x02,
    0x01,0x06,
    0x11,
    0x07,0x1E,0x90,0x9E,0x66,0x85,0x7A,0xD9,0xA0,0xC3,0x49,0xCD,0x89,0x00,0x14,0xC6,0x3E,
    0x09,
    0x09,'D','u','o','-','?','?','?','?',
};

static uint8_t      ble_tx_rx_stream[BLE_TX_RX_STREAM_MAX];
static uint8_t      ble_tx_rx_stream_len = 0;
static uint8_t      rec_len = 0;

static Cmd_pipe_state_t cmd_pipe_state = CMD_PIPE_AVAILABLE;

static wiced_bool_t device_configured = WICED_FALSE;
static uint8_t provision_status = PROVISION_STA_IDLE;


/******************************************************
 *               Function Definitions
 ******************************************************/
void ble_provision_init(void)
{
    static uint8_t init_done = 0;

    if(!init_done) {
        init_done = 1;

        hal_btstack_deInit();

        HAL_Pin_Mode(BT_RTS, OUTPUT);
        HAL_GPIO_Write(BT_RTS, 1);
        wiced_rtos_delay_milliseconds(50);

        HAL_Pin_Mode(BT_POWER, OUTPUT);
        HAL_GPIO_Write(BT_POWER, 0);
        wiced_rtos_delay_milliseconds(100);
        HAL_GPIO_Write(BT_POWER, 1);
        wiced_rtos_delay_milliseconds(100);

        // Get device name.
        wiced_ssid_t name;
        fetch_or_generate_setup_ssid(&name);
        name.value[name.length] = '\0';
        memcpy(&adv_data[23], name.value, MIN(name.length, 8));
        memcpy(ble_device_name, name.value, MIN(name.length, 20));

        //hal_btstack_Log_info(true);
        //hal_btstack_enable_packet_info();

        hal_btstack_init();

        hal_btstack_setConnectedCallback(deviceConnectedCallback);
        hal_btstack_setDisconnectedCallback(deviceDisconnectedCallback);
        hal_btstack_setGattCharsRead(gattReadCallback);
        hal_btstack_setGattCharsWrite(gattWriteCallback);

        hal_btstack_addServiceUUID16bits(0x1800);
        hal_btstack_addCharsUUID16bits(0x2A00, ATT_PROPERTY_READ|ATT_PROPERTY_WRITE, ble_device_name, sizeof(ble_device_name));
        hal_btstack_addCharsUUID16bits(0x2A01, ATT_PROPERTY_READ, appearance, sizeof(appearance));
        hal_btstack_addCharsUUID16bits(0x2A04, ATT_PROPERTY_READ, conn_param, sizeof(conn_param));
        hal_btstack_addServiceUUID16bits(0x1801);
        hal_btstack_addCharsUUID16bits(0x2A05, ATT_PROPERTY_INDICATE, change, sizeof(change));

        hal_btstack_addServiceUUID128bits(BLE_PROVISION_SERVICE_UUID);
        command_value_handle = hal_btstack_addCharsDynamicUUID128bits(BLE_PROVISION_CMD_CHAR_UUID, ATT_PROPERTY_WRITE_WITHOUT_RESPONSE|ATT_PROPERTY_NOTIFY, command_value, command_value_len);
        status_value_handle = hal_btstack_addCharsDynamicUUID128bits(BLE_PROVISION_STA_CHAR_UUID, ATT_PROPERTY_NOTIFY, status_value, 1);

        // setup advertisements params
        uint16_t adv_int_min = 0x0030;
        uint16_t adv_int_max = 0x0030;
        uint8_t adv_type = 0;
        bd_addr_t null_addr;
        memset(null_addr, 1, 6);
        hal_btstack_setAdvertisementParams(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);

        // set ble advertising.
        hal_btstack_setAdvertisementData(sizeof(adv_data), (uint8_t *)adv_data);

        INFO("BLE Provisioning begin.\n");
    }
    hal_btstack_startAdvertising();
}

void ble_provision_finalize() {
    provision_status = PROVISION_STA_CONNECTED;

    if(connect_handle != 0xFFFF) {
        ble_provision_notify(status_value_handle, &provision_status, 1);
        wiced_rtos_delay_milliseconds(100);
        ble_provision_send_ip_config();
        wiced_rtos_delay_milliseconds(500);
        hal_btstack_disconnect(connect_handle);
    }

    wiced_rtos_delay_milliseconds( 500 );

    HAL_USB_Detach();
    HAL_Core_System_Reset();
}

void ble_provision_on_failed() {
    provision_status = PROVISION_STA_CONNECT_FAILED;

    if(connect_handle != 0xFFFF) {
        ble_provision_notify(status_value_handle, &provision_status, 1);
        wiced_rtos_delay_milliseconds(100);
        hal_btstack_disconnect(connect_handle);
    }
    WARN("Connect to AP failed.\n");
}


/******************************************************
 *            Static Local Function Definitions
 ******************************************************/
static void ble_provision_init_variables(void) {
    provision_status = PROVISION_STA_IDLE;
    device_configured = WICED_FALSE;
}

static void ble_provision_send_sys_info(void) {

    DEBUG_D("Send system info. \r\n");

    if((command_notify_flag == 0x0001) && (cmd_pipe_state == CMD_PIPE_AVAILABLE)) {
        uint16_t ver[4] = {0x0000, 0x0000, 0x0000, 0x0000};

        ver[0] = FLASH_ModuleVersion(FLASH_INTERNAL, 0x08000000);
        ver[1] = FLASH_ModuleVersion(FLASH_INTERNAL, 0x08020000);
        ver[2] = FLASH_ModuleVersion(FLASH_INTERNAL, 0x08040000);
        ver[3] = FLASH_ModuleVersion(FLASH_INTERNAL, 0x080C0000);

        ble_tx_rx_stream_len = 0;
        memset(ble_tx_rx_stream, '\0', BLE_TX_RX_STREAM_MAX);

        ble_tx_rx_stream[ble_tx_rx_stream_len++] = 0;                                                   // TX stream length
        ble_tx_rx_stream[ble_tx_rx_stream_len++] = PROVISION_CMD_NOTIFY_SYS_INFO;                       // Command

        HAL_device_ID(&ble_tx_rx_stream[ble_tx_rx_stream_len], 12);                                     // Device ID
        ble_tx_rx_stream_len += 12;

        memcpy(&ble_tx_rx_stream[ble_tx_rx_stream_len], (uint8_t *)ver, sizeof(ver));                   // Module versions
        ble_tx_rx_stream_len += sizeof(ver);

        const char *release_str = stringify(SYSTEM_VERSION_STRING);
        ble_tx_rx_stream[ble_tx_rx_stream_len++] = strlen(release_str);                                 // Release string length
        memcpy(&ble_tx_rx_stream[ble_tx_rx_stream_len], release_str, strlen(release_str));              // Release string
        ble_tx_rx_stream_len += strlen(release_str);

        ble_tx_rx_stream[TX_RX_STREAM_LEN_IDX] = ble_tx_rx_stream_len;
        ble_provision_notify(command_value_handle, ble_tx_rx_stream, ble_tx_rx_stream_len);
    }
}

static void ble_provision_send_ap_details(wiced_scan_result_t* record) {

    DEBUG_D("Notify scanned AP: %s\r\n", record->SSID.value);

    if((command_notify_flag == 0x0001) && (cmd_pipe_state == CMD_PIPE_AVAILABLE)) {
        ble_tx_rx_stream_len = 0;
        memset(ble_tx_rx_stream, '\0', BLE_TX_RX_STREAM_MAX);

        ble_tx_rx_stream[ble_tx_rx_stream_len++] = 0;                                                   // TX stream length
        ble_tx_rx_stream[ble_tx_rx_stream_len++] = (uint8_t)PROVISION_CMD_NOTIFY_AP;                    // Command

        ble_tx_rx_stream[ble_tx_rx_stream_len++] = AP_SCANNED;                                          // AP state
        if(wlan_has_credentials() == 0) {
            platform_dct_wifi_config_t* wifi_config = NULL;
            wiced_result_t result = wiced_dct_read_lock( (void**) &wifi_config, WICED_FALSE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(*wifi_config));
            if (result == WICED_SUCCESS) {
                if(!memcmp(wifi_config->stored_ap_list[0].details.BSSID.octet, record->BSSID.octet, 6))
                    ble_tx_rx_stream[ble_tx_rx_stream_len-1] = AP_CONFIGURED;
            }
            wiced_dct_read_unlock(wifi_config, WICED_FALSE);
        }

        memcpy(&ble_tx_rx_stream[ble_tx_rx_stream_len], (uint8_t *)&record->signal_strength, sizeof(int16_t));     // Signal strength
        ble_tx_rx_stream_len += sizeof(int16_t);

        ble_tx_rx_stream[ble_tx_rx_stream_len++] = record->channel;                                     // Channel

        memcpy(&ble_tx_rx_stream[ble_tx_rx_stream_len], record->BSSID.octet, sizeof(wiced_mac_t));      // BSSID
        ble_tx_rx_stream_len += sizeof(wiced_mac_t);

        memcpy(&ble_tx_rx_stream[ble_tx_rx_stream_len], (uint8_t *)&record->security, sizeof(wiced_security_t));   // Security type
        ble_tx_rx_stream_len += sizeof(wiced_security_t);

        ble_tx_rx_stream[ble_tx_rx_stream_len++] = record->SSID.length;                                 // SSID length
        memcpy(&ble_tx_rx_stream[ble_tx_rx_stream_len], record->SSID.value, record->SSID.length);       // SSID
        ble_tx_rx_stream_len += record->SSID.length;

        ble_tx_rx_stream[TX_RX_STREAM_LEN_IDX] = ble_tx_rx_stream_len;
        ble_provision_notify(command_value_handle, ble_tx_rx_stream, ble_tx_rx_stream_len);
    }
}

static void ble_provision_send_ip_config(void) {

    DEBUG_D("Send IP config.\r\n");

    if((command_notify_flag == 0x0001) && (cmd_pipe_state == CMD_PIPE_AVAILABLE)) {
        wiced_security_t   sec;
        wiced_bss_info_t   ap_info;
        wiced_ip_address_t station_ip, gateway_ip;
        wiced_mac_t        gateway_mac;

        ble_tx_rx_stream_len = 0;
        memset(ble_tx_rx_stream, '\0', BLE_TX_RX_STREAM_MAX);

        ble_tx_rx_stream[ble_tx_rx_stream_len++] = 0;                                                   // TX stream length
        ble_tx_rx_stream[ble_tx_rx_stream_len++] = (uint8_t)PROVISION_CMD_NOTIFY_IP_CONFIG;             // Command

        wiced_ip_get_ipv4_address( WICED_STA_INTERFACE, &station_ip );
        wiced_ip_get_gateway_address( WICED_STA_INTERFACE, &gateway_ip );
        wwd_wifi_get_mac_address( &gateway_mac, WWD_AP_INTERFACE );
        wwd_wifi_get_ap_info( &ap_info, &sec );

        if(station_ip.version == WICED_IPV4) {                                                          // Device's IP
            ble_tx_rx_stream[ble_tx_rx_stream_len++] = 0x04;
            memcpy(&ble_tx_rx_stream[ble_tx_rx_stream_len], (uint8_t *)&station_ip.ip.v4, sizeof(uint32_t));
            ble_tx_rx_stream_len += sizeof(uint32_t);
        }
        else {
            ble_tx_rx_stream[ble_tx_rx_stream_len++] = 0x06;
            memcpy(&ble_tx_rx_stream[ble_tx_rx_stream_len], (uint8_t *)&station_ip.ip.v6, sizeof(station_ip.ip.v6));
            ble_tx_rx_stream_len += sizeof(station_ip.ip.v6);
        }

        if(gateway_ip.version == WICED_IPV4) {                                                          // Gateway's IP
            ble_tx_rx_stream[ble_tx_rx_stream_len++] = 0x04;
            memcpy(&ble_tx_rx_stream[ble_tx_rx_stream_len], (uint8_t *)&gateway_ip.ip.v4, sizeof(uint32_t));
            ble_tx_rx_stream_len += sizeof(uint32_t);
        }
        else {
            ble_tx_rx_stream[ble_tx_rx_stream_len++] = 0x06;
            memcpy(&ble_tx_rx_stream[ble_tx_rx_stream_len], (uint8_t *)gateway_ip.ip.v6, sizeof(gateway_ip.ip.v6));
            ble_tx_rx_stream_len += sizeof(gateway_ip.ip.v6);
        }

        memcpy(&ble_tx_rx_stream[ble_tx_rx_stream_len], gateway_mac.octet, sizeof(wiced_mac_t));        // Gateway MAC address
        ble_tx_rx_stream_len += sizeof(wiced_mac_t);

        ble_tx_rx_stream[ble_tx_rx_stream_len++] = ap_info.SSID_len;                                    // Connected SSID length
        memcpy(&ble_tx_rx_stream[ble_tx_rx_stream_len], ap_info.SSID, ap_info.SSID_len);                // Connected SSID
        ble_tx_rx_stream_len += ap_info.SSID_len;

        ble_tx_rx_stream[TX_RX_STREAM_LEN_IDX] = ble_tx_rx_stream_len;
        ble_provision_notify(command_value_handle, ble_tx_rx_stream, ble_tx_rx_stream_len);

        wiced_rtos_delay_milliseconds(200);
    }
}

static void ble_provision_notify(uint16_t attr_handle, uint8_t *pbuf, uint8_t len) {

    if(attr_handle != command_value_handle && attr_handle != status_value_handle)
        return;

    cmd_pipe_state = CMD_PIPE_BUSY_NOTIFY;

    if(connect_status == GapStatus_Connected) {
        uint8_t i=0, tx_len;
        while(i < len) {
            if(len-i >= 20)
                tx_len = 20;
            else
                tx_len = len - i;

            if(attr_handle == command_value_handle) {
                memcpy(command_value, &pbuf[i], tx_len);
                command_value_len = tx_len;
            }
            else {
                status_value[0] = *pbuf;
            }
            hal_btstack_attServerSendNotify(attr_handle, &pbuf[i], tx_len);
            i += tx_len;

            wiced_rtos_delay_milliseconds(20);
        }
    }
    cmd_pipe_state = CMD_PIPE_AVAILABLE;
}

static void ble_provision_parse_cmd(uint8_t *data, uint8_t data_len) {

    BLEProvisionCmd_t provision_cmd = (BLEProvisionCmd_t)data[0];

    switch(provision_cmd)
    {
        case PROVISION_CMD_NOTIFY_SYS_INFO:
            DEBUG_D("Command: Notify system information.\r\n");
            cmd_pipe_state = CMD_PIPE_AVAILABLE;
            if(provision_status != PROVISION_STA_SCANNING) {
                ble_provision_send_sys_info();
            }
            break;

        case PROVISION_CMD_SCAN_REQUEST:
            DEBUG_D("Command: Scan network.\r\n");
            cmd_pipe_state = CMD_PIPE_AVAILABLE;
            if(provision_status != PROVISION_STA_SCANNING) {
                provision_status = PROVISION_STA_SCANNING;
                ble_provision_notify(status_value_handle, &provision_status, 1);
                wiced_wifi_scan_networks(ble_provision_scan_result_handler, NULL);
            }
            break;

        case PROVISION_CMD_CONFIG_AP_ENTRY:
            DEBUG_D("Command: Config AP entry.\r\n");
            if((data_len-1) >= CONFIG_AP_ENTRY_LEN_MIN) {
                Config_ap_entry_t config_ap_entry;
                uint32_t security = 0x00000000;
                uint8_t i = 1; // The first byte is command

                config_ap_entry.channel = data[i++];                                                 // Channel
                security = (uint32_t)(data[i++]);
                security |= (uint32_t)(data[i++]<<8);
                security |= (uint32_t)(data[i++]<<16);
                security |= (uint32_t)(data[i++]<<24);
                config_ap_entry.security = (wiced_security_t)security;                               // Security
                config_ap_entry.SSID_len = data[i++];                                                // SSID length
                memcpy(config_ap_entry.SSID, &data[i], config_ap_entry.SSID_len);                    // SSID
                config_ap_entry.SSID[config_ap_entry.SSID_len] = '\0';
                i += config_ap_entry.SSID_len;
                config_ap_entry.security_key_length = data[i++];                                     // Security key length
                memcpy(config_ap_entry.security_key, &data[i], config_ap_entry.security_key_length); // Security key

                add_wiced_wifi_credentials((const char *)config_ap_entry.SSID, config_ap_entry.SSID_len, \
                                           (const char *)config_ap_entry.security_key, config_ap_entry.security_key_length, \
                                           config_ap_entry.security, config_ap_entry.channel);

                device_configured = WICED_TRUE;

                provision_status = PROVISION_STA_CONFIG_AP;
                ble_provision_notify(status_value_handle, &provision_status, 1);

                DEBUG_D("WiFi credentials saved.\r\n");
            }
            cmd_pipe_state = CMD_PIPE_AVAILABLE;
            break;

        case PROVISION_CMD_CONNECT_AP:
            DEBUG_D("Command: Connect to AP.\r\n");
            cmd_pipe_state = CMD_PIPE_AVAILABLE;
            if(device_configured == WICED_TRUE) {
                HAL_WLAN_notify_simple_config_done();

                provision_status = PROVISION_STA_CONNECTING;
                ble_provision_notify(status_value_handle, &provision_status, 1);

                DEBUG_D("Connecting...\r\n");
            }
            break;

        default:
            DEBUG_D("Command: Unknown.\r\n");
            break;
    }
}


/******************************************************
 *            BLE Callback Function Definitions
 ******************************************************/
static void deviceConnectedCallback(BLEStatus_t status, uint16_t handle) {
    switch (status) {
        case BLE_STATUS_OK:
            DEBUG_D("BLE device connected.\r\n");

            connect_status = GapStatus_Connected;
            connect_handle = handle;

            ble_provision_init_variables();

            /* Disable connectability. */
            hal_btstack_stopAdvertising();
            break;

        default:
            break;
    }
}

static void deviceDisconnectedCallback(uint16_t handle) {

    DEBUG_D("BLE device disconnect.\r\n");

    connect_status = GapStatus_Disconnect;
    connect_handle = 0xFFFF;
    command_notify_flag = 0x0000;
    status_notify_flag = 0x0000;

    /* Connection released. Re-enable BLE connectability. */
    //if(device_configured != WICED_TRUE)
        hal_btstack_startAdvertising();
}

static int gattWriteCallback(uint16_t value_handle, uint8_t *new_value, uint16_t new_value_len) {

    /* Client Characteristic Configuration Descriptor */
    if(value_handle == (command_value_handle+1)) {
        if(new_value_len >= 2) {
            command_notify_flag = new_value[1];    // Low byte comes first
            command_notify_flag = (command_notify_flag<<8) + new_value[0];
            DEBUG_D("Write command characteristic CCCD: %d\r\n", command_notify_flag);
        }
    }
    else if(value_handle == (status_value_handle+1)) {
        if(new_value_len >= 2) {
            status_notify_flag = new_value[1];
            status_notify_flag = (status_notify_flag<<8) + new_value[0];
            DEBUG_D("Write status characteristic CCCD: %d\r\n", status_notify_flag);
        }
    }
    /* Server characteristic attribute value */
    else if(value_handle == command_value_handle) {
        if(cmd_pipe_state == CMD_PIPE_AVAILABLE) {
            ble_tx_rx_stream_len = new_value[0];
            if(ble_tx_rx_stream_len >= 2) {        // At least 2 byte: stream_len + command + [...]
                rec_len = 0;
                memset(ble_tx_rx_stream, '\0', BLE_TX_RX_STREAM_MAX);
                cmd_pipe_state = CMD_PIPE_BUSY_WRITE;
                DEBUG_D("Receiving command stream. Length: %d\r\n", ble_tx_rx_stream_len);
            }
        }

        if(cmd_pipe_state == CMD_PIPE_BUSY_WRITE) {
            memcpy(&ble_tx_rx_stream[rec_len], new_value, new_value_len);
            rec_len += new_value_len;
            if(rec_len >= ble_tx_rx_stream_len) {
                DEBUG_D("Received command stream complete.\r\n");
                ble_provision_parse_cmd(&ble_tx_rx_stream[1], ble_tx_rx_stream_len-1);
                ble_tx_rx_stream_len = 0;
                cmd_pipe_state = CMD_PIPE_AVAILABLE;
            }
        }
    }

    return 0;
}

static uint16_t gattReadCallback(uint16_t value_handle, uint8_t *value, uint16_t value_len) {

    uint8_t characteristic_len=0;

    DEBUG_D("Read attribute callback.\r\n");

    if(value_handle == (command_value_handle+1)) {
        value = (uint8_t *)&command_notify_flag;    // Little Endian, so low byte sent first
        characteristic_len = sizeof(command_notify_flag);
    }
    else if(value_handle == (status_value_handle+1)) {
        value = (uint8_t *)&status_notify_flag;
        characteristic_len = sizeof(status_notify_flag);
    }
    if(command_value_handle == value_handle) {
        memcpy(value, command_value, command_value_len);
        characteristic_len = command_value_len;
    }
    else if(status_value_handle == value_handle) {
        *value = status_value[0];
        characteristic_len = 1;
    }
    return characteristic_len;
}

/* Callback function to handle scan results */
static wiced_result_t ble_provision_scan_result_handler( wiced_scan_handler_result_t* malloced_scan_result ) {
    malloc_transfer_to_curr_thread( malloced_scan_result );

    if ( malloced_scan_result->status == WICED_SCAN_INCOMPLETE ) {
        wiced_scan_result_t* record = &malloced_scan_result->ap_details;
        record->SSID.value[record->SSID.length] = 0; /* Ensure the SSID is null terminated */
        ble_provision_send_ap_details(record);
    }
    else {
        provision_status = PROVISION_STA_SCAN_COMPLETE;
        ble_provision_notify(status_value_handle, &provision_status, 1);
    }

    free( malloced_scan_result );

    return WICED_SUCCESS;
}

