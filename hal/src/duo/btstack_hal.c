

/**
 * Include Files
 */
#include "wiced.h"
#include "btstack.h"
#include "btstack_chipset_bcm.h"
#include "btstack_config.h"
#include "hci_dump.h"
#include "btstack_run_loop_wiced.h"

#include "btstack_hal.h"
#include "wlan_hal.h"
#include "usb_hal.h"

typedef enum {
    GATT_CLIENT_IDLE,
    GATT_CLIENT_SERVICE_RESULT,
    GATT_CLIENT_INCLUDE_SERVICE_RESULT,
    GATT_CLIENT_CHARACTERISTIC_RESULT,
    GATT_CLIENT_CHARACTERISTIC_DESICIPTORS_RESULT,
    GATT_CLIENT_READ_VALUE_RESULT,
    GATT_CLIENT_WRITE_VALUE_RESULT,
    GATT_CLIENT_READ_DESCRIPTOR_RESULT,
    GATT_CLIENT_WRITE_DESCRIPTOR_RESULT,
    GATT_CLIENT_WRITE_CLIENT_CHARS_CONFIG_RESULT,
} gatt_client_operation_t;

/**
 * Local Variables
 */
/**@brief Configure of hci_usart. */
const hci_transport_config_uart_t hci_uart_config = {
    HCI_TRANSPORT_CONFIG_UART,
    HCI_INIT_BAUDRATE,
    HCI_MAIN_BAUDRATE,
    0,
    NULL
};

/**@brief notify/indicate data node */
typedef struct{
    uint16_t handle;
    uint8_t  data_flag_len; //7-bit:1 indicate,0 notify. 6~0-bit:length of data.
    uint8_t  data[20];
}hal_notifyData_t;

/**@brief Queue for notify/indicate */
typedef struct{
    hal_notifyData_t queue[MAX_NR_NOTIFY_DATA_QUEUE];
    uint8_t head;
    uint8_t tail;
}hal_notifyDataQueue_t;

static hal_notifyDataQueue_t notify_queue={.head=0,.tail=0};
static bool pending_notify_complete;

/**@prepared write. */
static uint8_t prepared_request[MAX_PREPARED_WRITE_SIZE];
static uint16_t prepared_request_len;
static uint16_t prepared_request_att_handle;

/**@brief btstack state. */
static int btstack_state;
static uint8_t hci_init_flag = 0;
static hci_con_handle_t connect_handle = 0xFFFF;

static btstack_packet_callback_registration_t hci_event_callback_registration;
/**@brief connect timeout. */
static btstack_timer_source_t connection_timer;

/**@brief Gatt client action. */
static gatt_client_operation_t gatt_client_operation = GATT_CLIENT_IDLE;

/**@brief Gatt client notification queue. */
typedef struct{
    gatt_client_notification_t client_notify;
    uint8_t used_flag;
}client_notification_t;

static client_notification_t client_notify_queue[MAX_NR_CLIENT_NOTIFY_QUEUE];

/**@brief BD address. */
static bool have_custom_addr;
static bd_addr_t public_bd_addr;

/**@brief Thread for loop. */
static wiced_thread_t hal_btstack_thread_;
static uint8_t hal_btstack_thread_quit=1;

/**@brief */
static void hal_stack_thread(uint32_t arg);

/**@brief Gatt read/write callback handler. */
static gattReadCallback_t        gattReadCallback = NULL;
static gattWriteCallback_t       gattWriteCallback = NULL;

static bleAdvertismentCallback_t bleAdvertismentCallback = NULL;

static bleConnectedCallback_t    bleConnectedCallback = NULL;
static bleDisconnectedCallback_t bleDisconnectedCallback = NULL;

static gattServicesDiscoveredCallback_t    gattServiceDiscoveredCallback = NULL;
static gattCharsDiscoveredCallback_t       gattCharsDiscoveredCallback = NULL;
static gattDescriptorsDiscoveredCallback_t gattDescriptorsDiscoveredCallback = NULL;

static gattCharacteristicReadCallback_t    gattCharacteristicReadCallback = NULL;
static gattCharacteristicWrittenCallback_t gattCharacteristicWrittenCallback = NULL;
static gattDescriptorReadCallback_t        gattDescriptorReadCallback = NULL;
static gattDescriptorWrittenCallback_t     gattDescriptorticWrittenCallback = NULL;

static gattWriteCCCDCallback_t        gattWriteCCCDCallback = NULL;
static gattNotifyUpdateCallback_t     gattNotifyUpdateCallback = NULL;
static gattIndicateUpdateCallback_t   gattIndicateUpdateCallback = NULL;

static uint8_t notify_queueFreeSize(void);
static uint8_t notify_queneUsedSize(void);
static uint8_t notify_queneWrite(hal_notifyData_t *dat);
static uint8_t notify_queneRead(hal_notifyData_t *dat);

static void    client_notification_init(void);
static uint8_t client_notification_add(btstack_packet_handler_t callback, uint16_t con_handle, gatt_client_characteristic_t *characteristic);
static void    client_notification_remove(uint16_t con_handle, gatt_client_characteristic_t *characteristic);

/********************************************
 *         Function Declare
 *******************************************/
/**
 * @brief Thread for BLE loop.
 */
static void hal_stack_thread(uint32_t arg) {
    while(!hal_btstack_thread_quit)
    {
        hal_btstack_loop_execute();
    }
    WICED_END_OF_CURRENT_THREAD( );
}

/**
 * @brief Hardware error handler.
 */
static void parseAdvertisemetReport(advertisementReport_t *report, uint8_t *data)
{
    report->advEventType = data[2];
    report->peerAddrType = data[3];
    report->rssi         = data[10]-256;
    report->advDataLen   = data[11];
    memcpy(report->advData, &data[12], report->advDataLen);
    reverse_bd_addr(&data[4],report->peerAddr);
}

/**
 * @brief extract service from packet.
 */
static void extract_service(gatt_client_service_t * service, uint8_t * packet){
    service->start_group_handle = little_endian_read_16(packet, 4);
    service->end_group_handle   = little_endian_read_16(packet, 6);
    service->uuid16 = 0;
    reverse_128(&packet[8], service->uuid128);
    if (uuid_has_bluetooth_prefix(service->uuid128)){
        service->uuid16 = big_endian_read_32(service->uuid128, 0);
    }
}

/**
 * @brief extract characteristic from packet.
 */
static void extract_characteristic(gatt_client_characteristic_t * characteristic, uint8_t * packet){
    characteristic->start_handle = little_endian_read_16(packet, 4);
    characteristic->value_handle = little_endian_read_16(packet, 6);
    characteristic->end_handle =   little_endian_read_16(packet, 8);
    characteristic->properties =   little_endian_read_16(packet, 10);
    characteristic->uuid16 = 0;
    reverse_128(&packet[12], characteristic->uuid128);
    if (uuid_has_bluetooth_prefix(characteristic->uuid128)){
        characteristic->uuid16 = big_endian_read_32(characteristic->uuid128, 0);
    }
}

/**
 * @brief extract characteristic descriptor from packet.
 */
static void extract_characteristic_descriptors(gatt_client_characteristic_descriptor_t *descriptor, uint8_t *packet){
    descriptor->handle  = little_endian_read_16(packet, 4);
    descriptor->uuid16 = 0;
    reverse_128(&packet[6], descriptor->uuid128);
    if (uuid_has_bluetooth_prefix(descriptor->uuid128)){
        descriptor->uuid16 = big_endian_read_32(descriptor->uuid128, 0);
    }
}


/**
 * @brief Hardware error handler.
 */
void bluetooth_hardware_error(){
    log_info("Bluetooth Hardware Error event. Restarting...\n\n\n");
    while(1);
}

/**@brief ATT Client Read Callback for Dynamic Data
 *
 * @Note  If buffer == NULL, don't copy data, just return size of value
 *        If buffer != NULL, copy data and return number bytes copied
 *
 * @param con_handle of hci le connection
 * @param attribute_handle to be written
 * @param offset into the value - used for queued writes and long attributes
 * @param buffer
 * @param buffer_size
 *
 * @returns 0 if write was ok, ATT_ERROR_PREPARE_QUEUE_FULL if no space in queue, ATT_ERROR_INVALID_OFFSET if offset is larger than max buffer
 */
static uint16_t att_read_callback(uint16_t con_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size){
    if(gattReadCallback) {
        return gattReadCallback(att_handle, buffer, buffer_size);
    }
    return 0;
}

/**@brief ATT Client Write Callback for Dynamic Data
 *
 * @param con_handle of hci le connection
 * @param attribute_handle to be written
 * @param transaction - ATT_TRANSACTION_MODE_NONE for regular writes, ATT_TRANSACTION_MODE_ACTIVE for prepared writes and ATT_TRANSACTION_MODE_EXECUTE
 * @param offset into the value - used for queued writes and long attributes
 * @param buffer
 * @param buffer_size
 *
 * @returns 0 if write was ok, ATT_ERROR_PREPARE_QUEUE_FULL if no space in queue, ATT_ERROR_INVALID_OFFSET if offset is larger than max buffer
 */
static int att_write_callback(uint16_t con_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size){
    if (gattWriteCallback) {
        if (transaction_mode == ATT_TRANSACTION_MODE_ACTIVE) {
            if (prepared_request_len > 0 && prepared_request_att_handle != att_handle)
                return ATT_ERROR_UNLIKELY_ERROR;
            if (offset != prepared_request_len)
                return ATT_ERROR_INVALID_OFFSET;
            if (buffer_size + prepared_request_len >= sizeof(prepared_request))
                return ATT_ERROR_PREPARE_QUEUE_FULL;
            prepared_request_att_handle = att_handle;
            memcpy(prepared_request + prepared_request_len, buffer, buffer_size);
            prepared_request_len += buffer_size;
            return 0;
        } else if (transaction_mode == ATT_TRANSACTION_MODE_CANCEL) {
            prepared_request_len = 0;
            prepared_request_att_handle = 0;
            return 0;
        } else if (transaction_mode == ATT_TRANSACTION_MODE_EXECUTE) {
            if (prepared_request_att_handle == 0 || prepared_request_len == 0)
                return ATT_ERROR_UNLIKELY_ERROR;
            buffer = prepared_request;
            buffer_size = prepared_request_len;
            att_handle = prepared_request_att_handle;
            prepared_request_len = 0;
            prepared_request_att_handle = 0;        
        }
        return gattWriteCallback(att_handle, buffer, buffer_size);
    }
    return 0;
}

/**@brief Packet handle.
 *
 * @param packet_type
 * @param channel
 * @param *packet
 * @param size
 */
static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){

    if (packet_type != HCI_EVENT_PACKET)
        return;
    bd_addr_t addr;
    uint16_t handle;
    uint8_t event = hci_event_packet_get_type(packet);
    //log_info("packet_handler type 0x%02x", event);
    switch (event)
    {
        case BTSTACK_EVENT_STATE:
            btstack_state = packet[2];
            // bt stack activated, get started
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            handle = little_endian_read_16(packet, 3);
            connect_handle = 0xFFFF;
            if (bleDisconnectedCallback) {
                (*bleDisconnectedCallback)(handle);
            }
            break;

        case GAP_EVENT_ADVERTISING_REPORT:
            if(bleAdvertismentCallback) {
                advertisementReport_t report;
                parseAdvertisemetReport(&report, packet);
                (*bleAdvertismentCallback)(&report);
            }
            break;

        case HCI_EVENT_COMMAND_COMPLETE:
            if (HCI_EVENT_IS_COMMAND_COMPLETE(packet, hci_read_bd_addr)) {
                bd_addr_copy(addr, &packet[OFFSET_OF_DATA_IN_COMMAND_COMPLETE + 1]);
                log_info("Local Address: %s\n", bd_addr_to_str(addr));
                break;
            }
            break;

        case HCI_EVENT_LE_META:
            switch (packet[2])
            {
                case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                    handle = little_endian_read_16(packet, 4);
                    log_info("Connection complete, handle 0x%04x\n", handle);
                    btstack_run_loop_remove_timer(&connection_timer);
                    if (!bleConnectedCallback)
                        break;
                    if (packet[3])
                        (*bleConnectedCallback)(BLE_STATUS_CONNECTION_ERROR, 0xFFFF);
                    else
                        (*bleConnectedCallback)(BLE_STATUS_OK, handle);
                        connect_handle = handle;
                    break;
                default:
                    break;
            }
            break;

        case ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE:
            pending_notify_complete = false;
            break;

        default:
            break;
     }
}

static void handle_gatt_client_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    // if (hci) event is not 4-byte aligned, event->handle causes crash
    // workaround: check event type, assuming GATT event types are contagious
    if (packet[0] < GATT_EVENT_QUERY_COMPLETE)
        return;
    if (packet[0] > GATT_EVENT_MTU)
        return;

    //log_info("The packet is : ");
    //for(uint8_t index=0; index<size; index++)
    //    log_info("0x%02x", packet[index]);

    uint16_t con_handle = little_endian_read_16(packet, 2);
    uint8_t  status;
    uint8_t  *value;
    uint16_t value_handle;
    uint16_t value_length;

    log_info("gatt_client_event type 0x%02x", hci_event_packet_get_type(packet));

    switch(hci_event_packet_get_type(packet))
    {   // Scan primary services.
        case GATT_EVENT_SERVICE_QUERY_RESULT:
            if(gattServiceDiscoveredCallback)
            {
                gatt_client_service_t service;
                extract_service(&service, packet);
                gattServiceDiscoveredCallback(BLE_STATUS_OK, con_handle, &service);
            }
            break;
        // Scan characteristics of services.
        case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT:
            if(gattCharsDiscoveredCallback)
            {
                gatt_client_characteristic_t characteristic;
                extract_characteristic(&characteristic,packet);
                gattCharsDiscoveredCallback(BLE_STATUS_OK, con_handle, &characteristic);
            }
            break;
        // Scan descriptors of characteristic.
        case GATT_EVENT_ALL_CHARACTERISTIC_DESCRIPTORS_QUERY_RESULT:
            if(gattDescriptorsDiscoveredCallback)
            {
                gatt_client_characteristic_descriptor_t chars_descriptor;
                extract_characteristic_descriptors(&chars_descriptor, packet);
                gattDescriptorsDiscoveredCallback(BLE_STATUS_OK, con_handle, &chars_descriptor);
            }
            break;
        // Read characteristic value.
        case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT:
            if(gattCharacteristicReadCallback)
            {
                value_handle = little_endian_read_16(packet, 4);
                value_length = little_endian_read_16(packet, 6);
                value = &packet[8];
                gattCharacteristicReadCallback(BLE_STATUS_OK, con_handle, value_handle, value, value_length);
            }
            break;
        // Read long characteristic value.
        case GATT_EVENT_LONG_CHARACTERISTIC_VALUE_QUERY_RESULT:
            break;
        // Read characteristic descriptors.
        case GATT_EVENT_CHARACTERISTIC_DESCRIPTOR_QUERY_RESULT:
            if(gattDescriptorReadCallback)
            {
                value_handle = little_endian_read_16(packet, 4);
                value_length = little_endian_read_16(packet, 6);
                value = &packet[8];
                gattDescriptorReadCallback(BLE_STATUS_OK, con_handle, value_handle, value, value_length);
            }
            break;
        // Read long characteristic descriptors.
        case GATT_EVENT_LONG_CHARACTERISTIC_DESCRIPTOR_QUERY_RESULT:
            break;
        // Notify update event.
        case GATT_EVENT_NOTIFICATION:
            if(gattNotifyUpdateCallback)
            {
                value_handle = little_endian_read_16(packet, 4);
                value_length = little_endian_read_16(packet, 6);
                value = &packet[8];
                gattNotifyUpdateCallback(BLE_STATUS_OK, con_handle, value_handle, value, value_length);
            }
            break;
        // Indicate update event.
        case GATT_EVENT_INDICATION:
            if(gattIndicateUpdateCallback)
            {
                value_handle = little_endian_read_16(packet, 4);
                value_length = little_endian_read_16(packet, 6);
                value = &packet[8];
                gattIndicateUpdateCallback(BLE_STATUS_OK, con_handle, value_handle, value, value_length);
            }
            break;
        // Event complete.
        case GATT_EVENT_QUERY_COMPLETE:
            status = little_endian_read_16(packet, 4);
            switch(gatt_client_operation)
            {
                case GATT_CLIENT_SERVICE_RESULT:
                    gatt_client_operation = GATT_CLIENT_IDLE;
                    if(gattServiceDiscoveredCallback)
                        gattServiceDiscoveredCallback(BLE_STATUS_DONE, con_handle, NULL);
                    break;
                case GATT_CLIENT_CHARACTERISTIC_RESULT:
                    gatt_client_operation = GATT_CLIENT_IDLE;
                    if(gattCharsDiscoveredCallback)
                        gattCharsDiscoveredCallback(BLE_STATUS_DONE, con_handle, NULL);
                    break;
                case GATT_CLIENT_CHARACTERISTIC_DESICIPTORS_RESULT:
                    gatt_client_operation = GATT_CLIENT_IDLE;
                    if(gattDescriptorsDiscoveredCallback)
                        gattDescriptorsDiscoveredCallback(BLE_STATUS_DONE, con_handle, NULL);
                    break;
                case GATT_CLIENT_READ_VALUE_RESULT:
                    // packet:0xa0 0x03 con_handle status.
                    gatt_client_operation = GATT_CLIENT_IDLE;
                    if(gattCharacteristicReadCallback)
                        gattCharacteristicReadCallback((status ? BLE_STATUS_OTHER_ERROR : BLE_STATUS_DONE), 0xFFFF, 0xFFFF, NULL, 0);
                    break;
                case GATT_CLIENT_WRITE_VALUE_RESULT:
                    gatt_client_operation = GATT_CLIENT_IDLE;
                    if(gattCharacteristicWrittenCallback)
                        gattCharacteristicWrittenCallback(status ? BLE_STATUS_OTHER_ERROR : BLE_STATUS_DONE, con_handle);
                    break;

                case GATT_CLIENT_READ_DESCRIPTOR_RESULT:
                    gatt_client_operation = GATT_CLIENT_IDLE;
                    if(gattDescriptorReadCallback)
                        gattDescriptorReadCallback((status ? BLE_STATUS_OTHER_ERROR : BLE_STATUS_DONE), 0xFFFF, 0xFFFF, NULL, 0);
                    break;

                case GATT_CLIENT_WRITE_DESCRIPTOR_RESULT:
                    gatt_client_operation = GATT_CLIENT_IDLE;
                    if(gattDescriptorticWrittenCallback)
                        gattDescriptorticWrittenCallback(status ? BLE_STATUS_OTHER_ERROR : BLE_STATUS_DONE, con_handle);
                    break;

                case GATT_CLIENT_WRITE_CLIENT_CHARS_CONFIG_RESULT:
                    gatt_client_operation = GATT_CLIENT_IDLE;
                    if(gattWriteCCCDCallback)
                        gattWriteCCCDCallback(status ? BLE_STATUS_OTHER_ERROR : BLE_STATUS_DONE, con_handle);
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }
}


/**
 * @brief BTstack initialize.
 */
void hal_btstack_init(void)
{
    if(!hci_init_flag)
    {
        wlan_activate();

        // reset handler
        bleAdvertismentCallback           = NULL;
        bleConnectedCallback              = NULL;
        bleDisconnectedCallback           = NULL;
        gattServiceDiscoveredCallback     = NULL;
        gattCharsDiscoveredCallback       = NULL;
        gattDescriptorsDiscoveredCallback = NULL;
        gattCharacteristicReadCallback    = NULL;
        gattCharacteristicWrittenCallback = NULL;
        gattDescriptorReadCallback        = NULL;
        gattDescriptorticWrittenCallback  = NULL;
        gattWriteCCCDCallback             = NULL;
        gattNotifyUpdateCallback          = NULL;
        gattIndicateUpdateCallback        = NULL;

        att_db_util_init();

        btstack_memory_init();
        btstack_run_loop_init(btstack_run_loop_wiced_get_instance());

        hci_init(hci_transport_h4_instance(), (void*)&hci_uart_config);
        hci_set_link_key_db(btstack_link_key_db_memory_instance());
        hci_set_chipset(btstack_chipset_bcm_instance());

        if (have_custom_addr){
            hci_set_bd_addr(public_bd_addr);
        }

        hci_set_hardware_error_callback(&bluetooth_hardware_error);

        // register for HCI events
        hci_event_callback_registration.callback = &packet_handler;
        hci_add_event_handler(&hci_event_callback_registration);

        l2cap_init();

        // setup central device db
        le_device_db_init();

        sm_init();

        att_server_init(att_db_util_get_address(),att_read_callback, att_write_callback);
        att_server_register_packet_handler(packet_handler);

        gatt_client_init();

        client_notification_init();
        memset(&notify_queue, 0x00, sizeof(hal_notifyDataQueue_t));
        pending_notify_complete = false;

        // turn on!
        btstack_state = 0;
        hci_power_control(HCI_POWER_ON);
        // poll until working
        while (btstack_state != HCI_STATE_WORKING){
            btstack_run_loop_execute();
        }

        hal_btstack_thread_quit = 0;
        hci_init_flag = 1;
        connect_handle = 0xFFFF;
        wiced_rtos_create_thread(&hal_btstack_thread_, WICED_APPLICATION_PRIORITY, "BLE provision", hal_stack_thread, 1024*3, NULL);
    }
}

void hal_btstack_deInit(void)
{
    if(hci_init_flag)
    {
        hci_init_flag = 0;
        have_custom_addr = false;
        hal_btstack_thread_quit = 1;
	wiced_rtos_thread_join(&hal_btstack_thread_);
        btstack_run_loop_deInit();
	hci_power_control(HCI_POWER_OFF);
        hci_close();
    }
}

/**
 * @brief BTstack loop.
 */
void hal_btstack_loop_execute(void)
{
    if(hci_init_flag)
    {
        if (!pending_notify_complete && notify_queneUsedSize() && att_server_can_send_packet_now(connect_handle))
        {
            hal_notifyData_t data;
            int ret;
            notify_queneRead(&data);
            if ((data.data_flag_len & 0x80) == 0x00)
                ret = att_server_notify(connect_handle, data.handle, data.data, data.data_flag_len&0x7F);
            else
                ret = att_server_indicate(connect_handle, data.handle, data.data, data.data_flag_len&0x7F);
            if (ret == 0)
                pending_notify_complete = true;
        }
        btstack_run_loop_execute();
    }
}


/**
 * @brief Run loop set timer.
 *
 * @param[in]  *ts
 * @param[in]  timeout_in_ms
 */
void hal_btstack_setTimer(btstack_timer_source_t *ts, uint32_t timeout_in_ms)
{
    btstack_run_loop_set_timer(ts, timeout_in_ms);
}

void hal_btstack_setTimerHandler(btstack_timer_source_t *ts, btstack_timer_handler_t handler)
{
    btstack_run_loop_set_timer_handler(ts, handler);
}

void hal_btstack_addTimer(btstack_timer_source_t *timer)
{
    btstack_run_loop_add_timer(timer);
}

int hal_btstack_removeTimer(btstack_timer_source_t *timer)
{
    return btstack_run_loop_remove_timer(timer);
}

uint32_t hal_btstack_getTimeMs(void)
{
    return btstack_run_loop_get_time_ms();
}

/**
 * @brief Open debugger.
 */
void hal_btstack_Log_info(uint8_t flag)
{
    hci_dump_enable_log_level(LOG_LEVEL_INFO, flag);
}
void hal_btstack_error_info(uint8_t flag)
{
    hci_dump_enable_log_level(LOG_LEVEL_ERROR, flag);
}
void hal_btstack_enable_packet_info(void)
{
    hci_dump_open(NULL, HCI_DUMP_STDOUT);
}

/***************************************************************
 * Gap API
***************************************************************/

/**
 * @brief Set mode of random address.
 *
 * @param[in]  random_address_type
 */
void hal_btstack_setRandomAddressMode(gap_random_address_type_t random_address_type)
{
    gap_random_address_set_mode(random_address_type);
}

/**
 * @brief Sets update period for random address
 *
 * @param period_ms
 */
void hal_btstack_setUpdatePeriodForRamdomAddr(int period_ms)
{
    gap_random_address_set_update_period(period_ms);
}

/**
 * @brief Set random address.
 *
 * @note Sets random address mode to type off
 *
 * @param[in]  addr
 */
void hal_btstack_setRandomAddr(bd_addr_t addr)
{
    gap_random_address_set(addr);
}

/**
 * @brief Set public bd address.
 *
 * @param[in]  public_bd_addr
 */
void hal_btstack_setPublicBdAddr(bd_addr_t addr)
{
    have_custom_addr = true;
    memcpy(public_bd_addr, addr ,6);
}

/**
 * @brief Get local bd address.
 *
 * @param[out]  local_bd_addr
 */
void hal_btstack_getLocalBdAddr(bd_addr_t address_buffer)
{
    gap_local_bd_addr(address_buffer);
}

/**
 * @brief Get the address at advertisement when advertising.
 *
 * @param[out]  addr_type
 * @param[out]  addr
 */
void hal_btstack_getAddrOfAdvertisement(uint8_t *addr_type, bd_addr_t addr)
{
    gap_advertisements_get_address(addr_type, addr);
}

/**
 * @brief Set local name.
 *
 * @note has to be done before stack starts up
 *
 * @param[in]  *local_name  Name string,name is not copied, make sure memory is accessible during stack startup.
 */
void hal_btstack_setLocalName(const char *local_name)
{
    gap_set_local_name(local_name);
}

/**
 * @brief Set advertisement data.
 *
 * @param[in]  size  The size of advertising data, no more than 31bytes.
 * @param[in]  data  Advertising data.
 *
 */
void hal_btstack_setAdvertisementData(uint16_t size, uint8_t *data)
{
    gap_advertisements_set_data(size, data);
}

/**
 * @brief Set scan respons data.
 *
 * @param[in]  size  The size of scanResponse data, no more than 31bytes.
 * @param[in]  data  The buffer pointer of scanResponse data.
 *
 */
void hal_btstack_setScanResponseData(uint16_t size, uint8_t *data)
{
    gap_scan_response_set_data(size, data);
}

/**
 * @brief Set Advertisement Paramters.
 *
 * @note
 *
 * @param[in]  adv_int_min
 * @param[in]  adv_int_max
 * @param[in]  adv_type
 * @param[in]  dir_addr_type
 * @param[in]  dir_addr
 * @param[in]  channel_map
 * @param[in]  filter_policy
 *
 */
void hal_btstack_setAdvertisementParams(uint16_t adv_int_min, uint16_t adv_int_max, uint8_t adv_type, uint8_t dir_addr_type, bd_addr_t dir_addr, uint8_t channel_map, uint8_t filter_policy)
{
    gap_advertisements_set_params(adv_int_min,adv_int_max,adv_type,dir_addr_type,dir_addr,channel_map,filter_policy);
}

/**
 * @brief Start advertising.
 */
void hal_btstack_startAdvertising(void)
{
    gap_advertisements_enable(1);
}

/**
 * @brief Stop advertising.
 */
void hal_btstack_stopAdvertising(void)
{
    gap_advertisements_enable(0);
}

/**
 * @brief Set connected Callback.
 */
void hal_btstack_setConnectedCallback(bleConnectedCallback_t cb)
{
    bleConnectedCallback = cb;
}

/**
 * @brief Set disconnected Callback.
 */
void hal_btstack_setDisconnectedCallback(bleDisconnectedCallback_t cb)
{
    bleDisconnectedCallback = cb;
}

/**
 * @brief Disconnect.
 */
static btstack_timer_source_t disconnect_time;
static uint16_t disconnect_handle = 0xFFFF;

static void disconnect_task(struct btstack_timer_source *ts)
{
    if(disconnect_handle != 0xFFFF)
        gap_disconnect(disconnect_handle);
    disconnect_handle = 0xFFFF;
    memset(&notify_queue, 0x00, sizeof(hal_notifyDataQueue_t));
    pending_notify_complete = false;
}

void hal_btstack_disconnect(uint16_t handle)
{
    disconnect_handle = handle;
    disconnect_time.process = &disconnect_task;
    btstack_run_loop_set_timer(&disconnect_time, 20);
    btstack_run_loop_add_timer(&disconnect_time);
}

/**
 * @brief Connect to remote device.
 */
uint8_t hal_btstack_connect(bd_addr_t addr, bd_addr_type_t type)
{
    return gap_connect(addr, type);
}

/**
 * @brief Parameters of connection.
 */
void hal_btstack_setConnParamsRange(le_connection_parameter_range_t range)
{
    le_connection_parameter_range_t temp_range = range;
    gap_set_connection_parameter_range(&temp_range);
}

/**
 * @brief Update the connection parameters.
 */
int hal_btstack_updateConnectionParameters(hci_con_handle_t con_handle, uint16_t conn_interval_min,
        uint16_t conn_interval_max, uint16_t conn_latency, uint16_t supervision_timeout)
{
    return gap_update_connection_parameters(con_handle, conn_interval_min, conn_interval_max, conn_latency, supervision_timeout);
}

/**
 * @brief Request connection parameters update.
 */
int hal_btstack_requestConnectionParametersUpdate(hci_con_handle_t con_handle, uint16_t conn_interval_min,
    uint16_t conn_interval_max, uint16_t conn_latency, uint16_t supervision_timeout)
{
    return gap_request_connection_parameter_update(con_handle, conn_interval_min, conn_interval_max, conn_latency, supervision_timeout);
}

/**
 * @brief Start scanning.
 */
void hal_btstack_startScanning(void)
{
    gap_start_scan();
}

/**
 * @brief Stop scanning.
 */
void hal_btstack_stopScanning(void)
{
    gap_stop_scan();
}

/**
 * @brief Parameters of scanning.
 */
void hal_btstack_setScanParams(uint8_t scan_type, uint16_t scan_interval, uint16_t scan_window)
{
    gap_set_scan_parameters(scan_type, scan_interval, scan_window);
}

/**
 * @brief Set advertisement report callback for scanning device.
 */
void hal_btstack_setBLEAdvertisementCallback(bleAdvertismentCallback_t cb)
{
    bleAdvertismentCallback = cb;
}

/***************************************************************
 * Gatt server API
***************************************************************/
/**
 * @brief Check if a notification or indication can be send right now
 *
 * @return 1, if packet can be sent
 */
int hal_btstack_attServerCanSend(void)
{
    if(!notify_queueFreeSize())
        return 0;
        
    return 1;
}

/**
 * @brief Send notify to client.
 *
 * @note
 *
 * @param[in]  value_handle
 * @param[in]  *value
 * @param[in]  length
 *
 * @return 0 Success.
 */
int hal_btstack_attServerSendNotify(uint16_t value_handle, uint8_t *value, uint16_t length)
{
    hal_notifyData_t data;
    log_info("send notify!");
    data.handle        = value_handle;
    data.data_flag_len = length;
    memset(data.data, 0x00, 20);
    memcpy(data.data, value,  length);

    return notify_queneWrite(&data);
}

/**
 * @brief Send indicate to client.
 *
 * @param[in]  value_handle
 * @param[in]  *value
 * @param[in]  length
 *
 * @return 0 Success.
 */
int hal_btstack_attServerSendIndicate(uint16_t value_handle, uint8_t *value, uint16_t length)
{
    hal_notifyData_t data;

    data.handle        = value_handle;
    data.data_flag_len = length | 0x80;
    memset(data.data, 0x00, 20);
    memcpy(data.data, value,  length);

    return notify_queneWrite(&data);
}

/**
 * @brief Set read callback of gatt characteristic.
 *
 * @param[in]  cb
 */
void hal_btstack_setGattCharsRead(gattReadCallback_t cb)
{
    gattReadCallback = cb;
}

/**
 * @brief Set write callback of gatt characteristic.
 *
 * @param[in]  cb
 */
void hal_btstack_setGattCharsWrite(gattWriteCallback_t cb)
{
    gattWriteCallback = cb;
}

/**
 * @brief Add a 16bits-UUID service.
 *
 * @param[in]  uuid
 *
 */
void hal_btstack_addServiceUUID16bits(uint16_t uuid)
{
    att_db_util_add_service_uuid16(uuid);
}

/**
 * @brief Add a 128bits-UUID service.
 *
 * @param[in]  *uuid  Buffer of 128bits-UUID.
 *
 */
void hal_btstack_addServiceUUID128bits(uint8_t *uuid)
{
    att_db_util_add_service_uuid128(uuid);
}

/**
 * @brief Add a 16bits-UUID characteristic.
 *
 * @param[in]  uuid
 *
 * @return ::Attribute handle.
 */
uint16_t hal_btstack_addCharsUUID16bits(uint16_t uuid, uint16_t flags, uint8_t *data, uint16_t data_len)
{
    return att_db_util_add_characteristic_uuid16(uuid, flags, data, data_len);
}

uint16_t hal_btstack_addCharsDynamicUUID16bits(uint16_t uuid, uint16_t flags, uint8_t *data, uint16_t data_len)
{
    return att_db_util_add_characteristic_uuid16(uuid, flags|ATT_PROPERTY_DYNAMIC, data, data_len);
}

/**
 * @brief Add a 128bits-UUID characteristic.
 *
 * @param[in]  *uuid  Buffer of 128bits-UUID.
 *
 * @return ::Attribute handle.
 */
uint16_t hal_btstack_addCharsUUID128bits(uint8_t *uuid, uint16_t flags, uint8_t *data, uint16_t data_len)
{
    return att_db_util_add_characteristic_uuid128(uuid, flags, data, data_len);
}

uint16_t hal_btstack_addCharsDynamicUUID128bits(uint8_t *uuid, uint16_t flags, uint8_t *data, uint16_t data_len)
{
    return att_db_util_add_characteristic_uuid128(uuid, flags|ATT_PROPERTY_DYNAMIC, data, data_len);
}

/***************************************************************
 * Gatt client API
***************************************************************/
/**
 * @brief Register callback for discovering service.
 */
void hal_btstack_setGattServiceDiscoveredCallback(gattServicesDiscoveredCallback_t cb)
{
    gattServiceDiscoveredCallback = cb;
}

/**
 * @brief Register callback for discovering characteristic.
 */
void hal_btstack_setGattCharsDiscoveredCallback(gattCharsDiscoveredCallback_t cb)
{
    gattCharsDiscoveredCallback = cb;
}

/**
 * @brief Register callback for discovering descriptors of characteristic.
 */
void hal_btstack_setGattDescriptorsDiscoveredCallback(gattDescriptorsDiscoveredCallback_t cb)
{
    gattDescriptorsDiscoveredCallback = cb;
}

/**
 * @brief Register callback for reading characteristic value.
 */
void hal_btstack_setGattCharacteristicReadCallback(gattCharacteristicReadCallback_t cb)
{
    gattCharacteristicReadCallback = cb;
}

/**
 * @brief Register callback for writing characteristic value.
 */
void hal_btstack_setGattCharacteristicWrittenCallback(gattCharacteristicWrittenCallback_t cb)
{
    gattCharacteristicWrittenCallback = cb;
}

/**
 * @brief Register callback for reading characteristic descriptor value.
 */
void hal_btstack_setGattDescriptorReadCallback(gattDescriptorReadCallback_t cb)
{
    gattDescriptorReadCallback = cb;
}

/**
 * @brief Register callback for writing characteristic descriptor value.
 */
void hal_btstack_setGattDescriptorWrittenCallback(gattDescriptorWrittenCallback_t cb)
{
    gattDescriptorticWrittenCallback = cb;
}

/**
 * @brief Register callback for enable/disable CCCD.
 */
void hal_btstack_setGattWriteCCCDCallback(gattWriteCCCDCallback_t cb)
{
    gattWriteCCCDCallback = cb;
}

/**
 * @brief Register callback for value update.
 */
void hal_btstack_setGattNotifyUpdateCallback(gattNotifyUpdateCallback_t cb)
{
    gattNotifyUpdateCallback = cb;
}

void hal_btstack_setGattIndicateUpdateCallback(gattIndicateUpdateCallback_t cb)
{
    gattIndicateUpdateCallback = cb;
}


/**
 * @brief Discover primary service by conn_handle.
 *        For each found service, an le_service_event_t with type set to GATT_EVENT_SERVICE_QUERY_RESULT will be generated and passed to the registered callback.
 *        The gatt_complete_event_t, with type set to GATT_EVENT_QUERY_COMPLETE, marks the end of discovery.
 *
 * @param[in]  con_handle
 *
 * @return BTSTACK_MEMORY_ALLOC_FAILED
 *         GATT_CLIENT_IN_WRONG_STATE
 *         0::SUCCESS
 */
uint8_t hal_btstack_discoverPrimaryServices(uint16_t con_handle)
{
    gatt_client_operation = GATT_CLIENT_SERVICE_RESULT;
    return gatt_client_discover_primary_services(handle_gatt_client_event, con_handle);
}

uint8_t hal_btstack_discoverPrimaryServicesByUUID16(uint16_t con_handle, uint16_t uuid16)
{
    gatt_client_operation = GATT_CLIENT_SERVICE_RESULT;
    return gatt_client_discover_primary_services_by_uuid16(handle_gatt_client_event, con_handle, uuid16);
}

uint8_t hal_btstack_discoverPrimaryServicesByUUID128(uint16_t con_handle, const uint8_t * uuid)
{
    gatt_client_operation = GATT_CLIENT_SERVICE_RESULT;
    return gatt_client_discover_primary_services_by_uuid128(handle_gatt_client_event, con_handle, uuid);
}

/**
 * @brief Discover all characteristics of a service.
 *        For each found characteristic, an le_characteristics_event_t with type set to GATT_EVENT_CHARACTERISTIC_QUERY_RESULT will be generated and passed to the registered callback.
 *        The gatt_complete_event_t with type set to GATT_EVENT_QUERY_COMPLETE, marks the end of discovery.
 *
 * @param[in]  con_handle
 * @param[in]  *service
 *
 * @return BTSTACK_MEMORY_ALLOC_FAILED
 *         GATT_CLIENT_IN_WRONG_STATE
 *         0::SUCCESS
 */
uint8_t hal_btstack_discoverCharsForService(uint16_t con_handle, gatt_client_service_t  *service)
{
    gatt_client_operation = GATT_CLIENT_CHARACTERISTIC_RESULT;
    return gatt_client_discover_characteristics_for_service(handle_gatt_client_event, con_handle, service);
}

uint8_t hal_btstack_discoverCharsForHandleRangeByUUID16(uint16_t con_handle, uint16_t start_handle, uint16_t end_handle, uint16_t uuid16)
{
    gatt_client_operation = GATT_CLIENT_CHARACTERISTIC_RESULT;
    return gatt_client_discover_characteristics_for_handle_range_by_uuid16(handle_gatt_client_event, con_handle, start_handle, end_handle, uuid16);
}

uint8_t hal_btstack_discoverCharsForHandleRangeByUUID128(uint16_t con_handle, uint16_t start_handle, uint16_t end_handle, uint8_t  * uuid)
{
    gatt_client_operation = GATT_CLIENT_CHARACTERISTIC_RESULT;
    return gatt_client_discover_characteristics_for_handle_range_by_uuid128(handle_gatt_client_event, con_handle, start_handle, end_handle, uuid);
}

uint8_t hal_btstack_discoverCharsForServiceByUUID16(uint16_t con_handle, gatt_client_service_t  *service, uint16_t  uuid16)
{
    gatt_client_operation = GATT_CLIENT_CHARACTERISTIC_RESULT;
    return gatt_client_discover_characteristics_for_service_by_uuid16 (handle_gatt_client_event, con_handle, service, uuid16);
}

uint8_t hal_btstack_discoverCharsForServiceByUUID128(uint16_t con_handle, gatt_client_service_t  *service, uint8_t  * uuid128)
{
    gatt_client_operation = GATT_CLIENT_CHARACTERISTIC_RESULT;
    return gatt_client_discover_characteristics_for_service_by_uuid128(handle_gatt_client_event, con_handle, service, uuid128);
}

/**
 * @brief Discover descriptors of a characteristic.
 *        For each found descriptor, an le_characteristic_descriptor_event_t with type set to GATT_EVENT_ALL_CHARACTERISTIC_DESCRIPTORS_QUERY_RESULT will be generated and passed to the registered callback.
 *        The gatt_complete_event_t with type set to GATT_EVENT_QUERY_COMPLETE, marks the end of discovery.
 *
 * @param[in]  con_handle
 * @param[in]  *characteristic
 *
 * @return BTSTACK_MEMORY_ALLOC_FAILED
 *         GATT_CLIENT_IN_WRONG_STATE
 *         0::SUCCESS
 */
uint8_t hal_btstack_discoverCharsDescriptors(uint16_t con_handle, gatt_client_characteristic_t  *characteristic)
{
    gatt_client_operation = GATT_CLIENT_CHARACTERISTIC_DESICIPTORS_RESULT;
    return gatt_client_discover_characteristic_descriptors(handle_gatt_client_event, con_handle, characteristic);
}

/**
 * @brief Reads the characteristic value using the characteristic's value handle.
 *        If the characteristic value is found, an le_characteristic_value_event_t with type set to GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT will be generated and passed to the registered callback.
 *        The gatt_complete_event_t with type set to GATT_EVENT_QUERY_COMPLETE, marks the end of read.
 *
 * @param[in]  con_handle
 * @param[in]  *characteristic
 *
 * @return BTSTACK_MEMORY_ALLOC_FAILED
 *         GATT_CLIENT_IN_WRONG_STATE
 *         0::SUCCESS
 */
uint8_t hal_btstack_readValueOfCharacteristic(uint16_t con_handle, gatt_client_characteristic_t  *characteristic)
{
    gatt_client_operation = GATT_CLIENT_READ_VALUE_RESULT;
    return gatt_client_read_value_of_characteristic(handle_gatt_client_event, con_handle, characteristic);
}

uint8_t hal_btstack_readValueOfCharacteristicUsingValueHandle(uint16_t con_handle, uint16_t characteristic_value_handle)
{
    gatt_client_operation = GATT_CLIENT_READ_VALUE_RESULT;
    return gatt_client_read_value_of_characteristic_using_value_handle(handle_gatt_client_event, con_handle, characteristic_value_handle);
}

uint8_t hal_btstack_readValueOfCharacteristicByUUID16(uint16_t con_handle, uint16_t start_handle, uint16_t end_handle, uint16_t uuid16)
{
    gatt_client_operation = GATT_CLIENT_READ_VALUE_RESULT;
    return gatt_client_read_value_of_characteristics_by_uuid16(handle_gatt_client_event, con_handle, start_handle, end_handle, uuid16);
}

uint8_t hal_btstack_readValueOfCharacteristicByUUID128(uint16_t con_handle, uint16_t start_handle, uint16_t end_handle, uint8_t *uuid128)
{
    gatt_client_operation = GATT_CLIENT_READ_VALUE_RESULT;
    return gatt_client_read_value_of_characteristics_by_uuid128(handle_gatt_client_event, con_handle, start_handle, end_handle, uuid128);
}

uint8_t hal_btstack_readLongValueOfCharacteristic(uint16_t con_handle, gatt_client_characteristic_t *characteristic)
{
    gatt_client_operation = GATT_CLIENT_READ_VALUE_RESULT;
    return gatt_client_read_long_value_of_characteristic(handle_gatt_client_event, con_handle, characteristic);
}

uint8_t hal_btstack_readLongValueOfCharacteristicUsingValueHandle(uint16_t con_handle, uint16_t characteristic_value_handle)
{
    gatt_client_operation = GATT_CLIENT_READ_VALUE_RESULT;
    return gatt_client_read_long_value_of_characteristic_using_value_handle(handle_gatt_client_event, con_handle, characteristic_value_handle);
}

uint8_t hal_btstack_readLongValueOfCharacteristicUsingValueHandleWithOffset(uint16_t con_handle, uint16_t characteristic_value_handle, uint16_t offset)
{
    gatt_client_operation = GATT_CLIENT_READ_VALUE_RESULT;
    return gatt_client_read_long_value_of_characteristic_using_value_handle_with_offset(handle_gatt_client_event, con_handle, characteristic_value_handle, offset);
}

/**
 * @brief Writes the characteristic value using the characteristic's value handle without an acknowledgment that the write was successfully performed.
 */
uint8_t hal_btstack_writeValueOfChracteristicWithoutResponse(uint16_t con_handle, uint16_t characteristic_value_handle, uint16_t length, uint8_t  *data)
{
    gatt_client_operation = GATT_CLIENT_WRITE_VALUE_RESULT;
    return gatt_client_write_value_of_characteristic_without_response(con_handle, characteristic_value_handle, length, data);
}

/**
 * @brief Writes the characteristic value.
 *        The gatt_complete_event_t with type set to GATT_EVENT_QUERY_COMPLETE, marks the end of write.
 *        The write is successfully performed, if the event's status field is set to 0.
 *
 * @param[in]  con_handle
 * @param[in]  characteristic_value_handle
 * @param[in]  length
 * @param[in]  *data
 *
 * @return BTSTACK_MEMORY_ALLOC_FAILED
 *         GATT_CLIENT_IN_WRONG_STATE
 *         0::SUCCESS
 */
uint8_t hal_btstack_writeValueOfCharacteristic(uint16_t con_handle, uint16_t characteristic_value_handle, uint16_t length, uint8_t *data)
{
    gatt_client_operation = GATT_CLIENT_WRITE_VALUE_RESULT;
    return gatt_client_write_value_of_characteristic(handle_gatt_client_event, con_handle, characteristic_value_handle, length, data);
}

uint8_t hal_btstack_writeLongValueOfCharacteristic(uint16_t con_handle, uint16_t characteristic_value_handle, uint16_t length, uint8_t *data)
{
    gatt_client_operation = GATT_CLIENT_WRITE_VALUE_RESULT;
    return gatt_client_write_long_value_of_characteristic(handle_gatt_client_event, con_handle, characteristic_value_handle, length, data);
}

uint8_t hal_btstack_writeLongValueOfCharacteristicWithOffset(uint16_t con_handle, uint16_t characteristic_value_handle, uint16_t offset, uint16_t length, uint8_t *data)
{
    gatt_client_operation = GATT_CLIENT_WRITE_VALUE_RESULT;
    return gatt_client_write_long_value_of_characteristic_with_offset(handle_gatt_client_event, con_handle, characteristic_value_handle, offset, length, data);
}

/**
 * @brief Reads the characteristic descriptor.
 *        If the characteristic descriptor is found, an le_characteristic_descriptor_event_t with type set to GATT_EVENT_CHARACTERISTIC_DESCRIPTOR_QUERY_RESULT will be generated and passed to the registered callback.
 *        The gatt_complete_event_t with type set to GATT_EVENT_QUERY_COMPLETE, marks the end of read.
 *
 * @param[in]  con_handle
 * @param[in]  *descriptor
 *
 * @return BTSTACK_MEMORY_ALLOC_FAILED
 *         GATT_CLIENT_IN_WRONG_STATE
 *         0::SUCCESS
 */
uint8_t hal_btstack_readCharacteristicDescriptor(uint16_t con_handle, gatt_client_characteristic_descriptor_t *descriptor)
{
    gatt_client_operation = GATT_CLIENT_READ_DESCRIPTOR_RESULT;
    return gatt_client_read_characteristic_descriptor(handle_gatt_client_event, con_handle, descriptor);
}

uint8_t hal_btstack_readCharacteristicDescriptorUsingDescriptorHandle(uint16_t con_handle, uint16_t descriptor_handle)
{
    gatt_client_operation = GATT_CLIENT_READ_DESCRIPTOR_RESULT;
    return gatt_client_read_characteristic_descriptor_using_descriptor_handle(handle_gatt_client_event, con_handle, descriptor_handle);
}

uint8_t hal_btstack_readLongCharacteristicDescriptor(uint16_t con_handle, gatt_client_characteristic_descriptor_t *descriptor)
{
    gatt_client_operation = GATT_CLIENT_READ_DESCRIPTOR_RESULT;
    return gatt_client_read_long_characteristic_descriptor(handle_gatt_client_event, con_handle, descriptor);
}

uint8_t hal_btstack_readLongCharacteristicDescriptorUsingDescriptorHandle(uint16_t con_handle, uint16_t descriptor_handle)
{
    gatt_client_operation = GATT_CLIENT_READ_DESCRIPTOR_RESULT;
    return gatt_client_read_long_characteristic_descriptor_using_descriptor_handle(handle_gatt_client_event, con_handle, descriptor_handle);
}

uint8_t hal_btstack_readLongCharacteristicDescriptorUsingDescriptorHandleWithOffset(uint16_t con_handle, uint16_t descriptor_handle, uint16_t offset)
{
    gatt_client_operation = GATT_CLIENT_READ_DESCRIPTOR_RESULT;
    return gatt_client_read_long_characteristic_descriptor_using_descriptor_handle_with_offset(handle_gatt_client_event, con_handle, descriptor_handle, offset);
}

/**
 * @brief Write the characteristic descriptor.
 *        The gatt_complete_event_t with type set to GATT_EVENT_QUERY_COMPLETE, marks the end of write. The write is successfully performed, if the event's status field is set to 0.
 * @param[in]  con_handle
 * @param[in]  *descriptor
 * @param[in]
 * @param[in]
 *
 * @return BTSTACK_MEMORY_ALLOC_FAILED
 *         GATT_CLIENT_IN_WRONG_STATE
 *         0::SUCCESS
 */
uint8_t hal_btstack_writeCharacteristicDescriptor(uint16_t con_handle, gatt_client_characteristic_descriptor_t *descriptor, uint16_t length, uint8_t *data)
{
    gatt_client_operation = GATT_CLIENT_WRITE_DESCRIPTOR_RESULT;
    return gatt_client_write_characteristic_descriptor(handle_gatt_client_event, con_handle, descriptor, length, data);
}

uint8_t hal_btstack_writeCharacteristicDescriptorUsingDescriptorHandle(uint16_t con_handle, uint16_t descriptor_handle, uint16_t length, uint8_t *data)
{
    gatt_client_operation = GATT_CLIENT_WRITE_DESCRIPTOR_RESULT;
    return gatt_client_write_characteristic_descriptor_using_descriptor_handle(handle_gatt_client_event, con_handle, descriptor_handle, length, data);
}

uint8_t hal_btstack_writeLongCharacteristicDescriptor(uint16_t con_handle, gatt_client_characteristic_descriptor_t *descriptor, uint16_t length, uint8_t *data)
{
    gatt_client_operation = GATT_CLIENT_WRITE_DESCRIPTOR_RESULT;
    return gatt_client_write_long_characteristic_descriptor(handle_gatt_client_event, con_handle, descriptor, length, data);
}

uint8_t hal_btstack_writeLongCharacteristicDescriptorUsingDescriptorHandle(uint16_t con_handle, uint16_t descriptor_handle, uint16_t length, uint8_t *data)
{
    gatt_client_operation = GATT_CLIENT_WRITE_DESCRIPTOR_RESULT;
    return gatt_client_write_long_characteristic_descriptor_using_descriptor_handle(handle_gatt_client_event, con_handle, descriptor_handle, length, data);
}

uint8_t hal_btstack_writeLongCharacteristicDescriptorUsingDescriptorHandleWithOffset(uint16_t con_handle, uint16_t descriptor_handle, uint16_t offset, uint16_t length, uint8_t *data)
{
    gatt_client_operation = GATT_CLIENT_WRITE_DESCRIPTOR_RESULT;
    return gatt_client_write_long_characteristic_descriptor_using_descriptor_handle_with_offset(handle_gatt_client_event, con_handle, descriptor_handle, offset, length, data);
}

/**
 * @brief Writes the client characteristic configuration of the specified characteristic. It is used to subscribe for notifications or indications of the characteristic value. For notifications or indications specify: GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION resp. GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_INDICATION as configuration value.
 */
uint8_t hal_btstack_writeClientCharacteristicConfiguration(uint16_t con_handle, gatt_client_characteristic_t * characteristic, uint16_t configuration)
{
    gatt_client_operation = GATT_CLIENT_WRITE_CLIENT_CHARS_CONFIG_RESULT;
    if(configuration == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NONE)
    {
        client_notification_remove(con_handle, characteristic);
        return gatt_client_write_client_characteristic_configuration(handle_gatt_client_event, con_handle, characteristic, configuration);
    }
    else
    {
        uint8_t ret = client_notification_add(handle_gatt_client_event, con_handle, characteristic);
        if(ret == 0)
            return BTSTACK_MEMORY_ALLOC_FAILED;
        else if(ret == 1)
            return 0;
        else
            return gatt_client_write_client_characteristic_configuration(handle_gatt_client_event, con_handle, characteristic, configuration);
    }
}

void hal_btstack_listenForCharacteristicValueUpdates(gatt_client_notification_t *notification, uint16_t con_handle, gatt_client_characteristic_t *characteristic)
{
    gatt_client_listen_for_characteristic_value_updates(notification, handle_gatt_client_event, con_handle, characteristic);
}


/***************************************************************
 * Other API
***************************************************************/
/**
 * @brief check whether has available space.
 *
 * @return 0    : full
 *         else : number of free.
 */
static uint8_t notify_queueFreeSize(void)
{
    return ( MAX_NR_NOTIFY_DATA_QUEUE - 1 - ((MAX_NR_NOTIFY_DATA_QUEUE + notify_queue.head - notify_queue.tail) % MAX_NR_NOTIFY_DATA_QUEUE));
}

/**
 * @brief Check whether has data to be sent.
 *
 * @return 0    : empty
 *         else : number of data.
 */
static uint8_t notify_queneUsedSize(void)
{
    return ((MAX_NR_NOTIFY_DATA_QUEUE + notify_queue.head - notify_queue.tail) % MAX_NR_NOTIFY_DATA_QUEUE);
}

/**
 * @brief Put a notify data to queue.
 *
 * @return 0:FAIL.
 *         1:SUCCESS.
 */
static uint8_t notify_queneWrite(hal_notifyData_t *dat)
{
    if(!notify_queueFreeSize())
        return 0;

    notify_queue.queue[notify_queue.head] = *dat;
    notify_queue.head = (notify_queue.head + 1) % MAX_NR_NOTIFY_DATA_QUEUE;

    return 1;
}

/**
 * @brief Read a notify data from queue.
 *
 * @return 0:FAIL.
 *         1:SUCCESS.
 */
static uint8_t notify_queneRead(hal_notifyData_t *dat)
{
    if(!notify_queneUsedSize())
        return 0;

    *dat = notify_queue.queue[notify_queue.tail];
    notify_queue.tail = (notify_queue.tail + 1) % MAX_NR_NOTIFY_DATA_QUEUE;

    return 1;
}

/**
 * @brief Operations of client_notify_queue.
 */
static void client_notification_init(void)
{
    memset(client_notify_queue, 0x00, sizeof(client_notify_queue));
}

/**
 * @brief Add element to list.
 *
 * @return 0:FAIL.
 *         1:EXIST.
 *         2:SUCCESS.
 */
static uint8_t client_notification_add(btstack_packet_handler_t callback, uint16_t con_handle, gatt_client_characteristic_t *characteristic)
{
    uint8_t index = 0;

    for(index=0; index<MAX_NR_CLIENT_NOTIFY_QUEUE; index++) {
        if(client_notify_queue[index].client_notify.con_handle == con_handle &&
           client_notify_queue[index].client_notify.attribute_handle == characteristic->value_handle) {
            // Exist
            log_info("client notify exist.");
            return 1;
        }
    }
    // Get a available index.
    for(index=0; index<MAX_NR_CLIENT_NOTIFY_QUEUE; index++) {
        if(client_notify_queue[index].used_flag == 0)
            break;
    }
    if(index >= MAX_NR_CLIENT_NOTIFY_QUEUE)
        return 0;

    client_notify_queue[index].client_notify.callback = callback;
    client_notify_queue[index].used_flag = 1;
    log_info("client notify listen.");
    gatt_client_listen_for_characteristic_value_updates(&client_notify_queue[index].client_notify, handle_gatt_client_event, con_handle, characteristic);

    return 2;
}

/**
 * @brief Remove element from list.
 *
 * @return 0:FAIL.
 *         1:SUCCESS.
 */
static void client_notification_remove(uint16_t con_handle, gatt_client_characteristic_t *characteristic)
{
    uint8_t index = 0;

    for(index=0; index<MAX_NR_CLIENT_NOTIFY_QUEUE; index++) {
        if(client_notify_queue[index].client_notify.con_handle == con_handle &&
           client_notify_queue[index].client_notify.attribute_handle == characteristic->value_handle)
        {
            log_info("client notify clean.");
            client_notify_queue[index].client_notify.con_handle = 0;
            client_notify_queue[index].client_notify.attribute_handle = 0;
            client_notify_queue[index].client_notify.callback = NULL;
            client_notify_queue[index].used_flag = 0;
        }
    }
}
