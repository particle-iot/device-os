
#ifndef __SPARK_WIRING_BTSTACK_H_
#define __SPARK_WIRING_BTSTACK_H_

#if PLATFORM_ID == 88 // Duo

#include "spark_wiring.h"
#include "btstack_hal.h"

class BLEDevice
{
public:
    BLEDevice(){
        //do nothing.
    }

    /**
     * @brief BTstack initialize.
     */
    void init(void);

    void deInit(void);

    /**
     * @brief Set timer based on current time in milliseconds.
     */
    void setTimer(btstack_timer_source_t *ts, uint32_t timeout_in_ms);

    /**
     * @brief Set callback that will be executed when timer expires.
     */
    void setTimerHandler(btstack_timer_source_t *ts, btstack_timer_handler_t handler);

    /**
     * @brief Add timer to run_loop (keep list sorted).
     */
    void addTimer(btstack_timer_source_t *timer);

    /**
     * @brief Remove timer from run loop
     */
    int  removeTimer(btstack_timer_source_t *timer);

    /**
     * @brief Get current time in ms
     */
    uint32_t getTimeMs(void);

    /**
     * @brief Debugger.
     */
    void debugLogger(bool flag);

    void debugError(bool flag);

    void enablePacketLogger(void);

    /***************************************************************
     *
     * Gap API
     *
    ***************************************************************/
    /**
     * @brief Set mode of random address.
     *
     * @param[in]  random_address_type
     */
    void setRandomAddrMode(gap_random_address_type_t random_addr_type);

    /**
     * @brief Set random address.
     *
     * @param[in]  addr
     */
    void setRandomAddr(bd_addr_t addr);

    /**
     * @brief Set public bd address.
     *
     * @note  Must be called befor ble.init().
     *
     * @param[in]  public_bd_addr
     */
    void setPublicBDAddr(bd_addr_t addr);

    /**
     * @brief Get local bd address.
     *
     * @param[out]  local_bd_addr
     */
    void getLocalBdAddr(bd_addr_t address_buffer);

    /**
     * @brief Get the address at advertisement when advertising.
     *
     * @param[out]  addr_type
     * @param[out]  addr
     */
    void getAddrOfAdvertisement(uint8_t *addr_type, bd_addr_t addr);

    /**
     * @brief Set local name.
     *
     * @note has to be done before stack starts up
     *
     * @param[in]  *local_name  Name string,name is not copied, make sure memory is accessible during stack startup.
     */
    void setLocalName(const char *local_name);

    /**
     * @brief Set advertisement parameters.
     *
     * @note  advertising_interval_min ([0x0020,0x4000], default: 0x0800, unit: 0.625 msec)
     *        advertising_interval_max ([0x0020,0x4000], default: 0x0800, unit: 0.625 msec)
     *        advertising_type (enum from 0: ADV_IND, ADC_DIRECT_IND, ADV_SCAN_IND, ADV_NONCONN_IND)
     *        own_address_type (enum from 0: BLE_GAP_ADDR_TYPE_PUBLIC, BLE_GAP_ADDR_TYPE_RANDOM)
     *        advertising_channel_map (flags: chan_37(1), chan_38(2), chan_39(4), all(7))
     *        filter policies (enum from 0: BLE_GAP_ADV_FP_ANY, BLE_GAP_ADV_FP_FILTER_SCANREQ, BLE_GAP_ADV_FP_FILTER_CONNREQ, BLE_GAP_ADV_FP_FILTER_BOTH)
     * @note  If the advertising_type is set to ADV_SCAN_IND or ADV_NONCONN_IND,advertising_interval_min and advertising_interval_max shal not be set to less than 0x00A0.
     *
     */
    void setAdvertisementParams(advParams_t *adv_params);

    /**
     * @brief Set advertising data.
     *
     * @param[in]  size  The size of advertising data, no more than 31bytes.
     * @param[in]  data  Advertising data. Data is not copied, pointer has to stay valid.
     */
    void setAdvertisementData(uint16_t size, uint8_t *data);

    /**
     * @brief Set scanResponse data.
     *
     * @param[in]  size  The size of scanResponse data, no more than 31bytes.
     * @param[in]  data  The buffer pointer of scanResponse data. Data is not copied, pointer has to stay valid.
     */
    void setScanResponseData(uint16_t size, uint8_t *data);

    /**
     * @brief Connected callback.
     */
    void onConnectedCallback(bleConnectedCallback_t cb);

    /**
     * @brief Disconnected callback.
     */
    void onDisconnectedCallback(bleDisconnectedCallback_t cb);

    /**
     * @brief Start advertising.
     */
    void startAdvertising(void);

    /**
     * @brief Stop advertising.
     */
    void stopAdvertising(void);

    /**
     * @brief Disconnect from remote device.
     */
    void disconnect(uint16_t conn_handle);

    /**
     * @brief Connect to a remote device.
     */
    uint8_t connect(bd_addr_t addr, bd_addr_type_t type);

    /**
     * @brief Set accepted connection parameter range.
     */
    void setConnParams(le_connection_parameter_range_t range);

    /**
     * @brief Start scanning.
     */
    void startScanning(void);

    /**
     * @brief Stop scanning.
     */
    void stopScanning(void);

    /**
     * @brief Set parameters for LE Scan
     */
    void setScanParams(uint8_t scan_type, uint16_t scan_interval, uint16_t scan_window);

    /**
     * @brief Set advertisement report callback for scanning device.
     */
    void onScanReportCallback(bleAdvertismentCallback_t cb);


    /***************************************************************
     *
     * Gatt server API
     *
    ***************************************************************/
    /**
     * @brief Add a 16bits-UUID service.
     */
    void addService(uint16_t uuid);

    /**
     * @brief Add a 128bits-UUID service.
     *
     * @param[in]  *uuid  Buffer of 128bits-UUID.
     */
    void addService(uint8_t *uuid);

    /**
     * @brief Add a 16bits-UUID characteristic.
     *
     * @note  Just has read permit, if need write permit, use "addCharacteristicDynamic".
     *
     * @param[in]  uuid
     * @param[in]  flags     Properties.See ATT_PROPERTY_* in ble/att_db.h
     * @param[in]  *data     Initialized data.
     * @param[in]  data_len
     *
     * @return ::Attribute handle.
     */
    uint16_t addCharacteristic(uint16_t uuid, uint16_t flags, uint8_t *data, uint16_t data_len);

    /**
     * @brief Add a 128bits-UUID characteristic.
     *
     * @note  Just has read permit, if need write permit, use "addCharacteristicDynamic".
     *
     * @param[in]  *uuid     Buffer of 128bits-UUID.
     * @param[in]  flags     Properties.See ATT_PROPERTY_* in ble/att_db.h.
     * @param[in]  *data     Initialized data.
     * @param[in]  data_len
     *
     * @return ::Attribute handle.
     */
    uint16_t addCharacteristic(uint8_t *uuid, uint16_t flags, uint8_t *data, uint16_t data_len);

    /**
     * @brief Add a 16bits-UUID characteristic.
     *
     * @param[in]  uuid       16bits-UUID.
     * @param[in]  flags      Properties.See ATT_PROPERTY_* in ble/att_db.h.
     * @param[in]  *data      Initialized data.
     * @param[in]  data_len
     *
     * @return ::Attribute handle.
     */
    uint16_t addCharacteristicDynamic(uint16_t uuid, uint16_t flags, uint8_t *data, uint16_t data_len);

    /**
     * @brief Add a 128bits-UUID characteristic.
     *
     * @param[in]  *uuid     Buffer of 128bits-UUID.
     * @param[in]  flags     Properties.See ATT_PROPERTY_* in ble/att_db.h.
     * @param[in]  *data     Initialized data.
     * @param[in]  data_len
     *
     * @return ::Attribute handle.
     */
    uint16_t addCharacteristicDynamic(uint8_t *uuid, uint16_t flags, uint8_t *data, uint16_t data_len);

    /**
     * @brief Set read callback of gatt server characteristic.
     */
    void onDataReadCallback(gattReadCallback_t cb);

    /**
     * @brief Set write callback of gatt server characteristic.
     */
    void onDataWriteCallback(gattWriteCallback_t cb);

    /**
     * @brief Check if a notification or indication can be send right now.
     *
     * @return 1, if packet can be sent
     */
    int attServerCanSendPacket(void);

    /**
     * @brief Send notify to client.
     *
     * @note  Need to check whether can be sent now.See "attServerCanSendPacket".
     *
     * @param[in]  value_handle
     * @param[in]  *value
     * @param[in]  length
     *
     * @return  0     Success.
     *          other Error.
     */
    int sendNotify(uint16_t value_handle, uint8_t *value, uint16_t length);

    /**
     * @brief Send notify to client.
     *
     * @note  Need to check whether can be sent now.See "attServerCanSendPacket".
     */
    int sendIndicate(uint16_t value_handle, uint8_t *value, uint16_t length);

    /***************************************************************
     *
     * Gatt client API
     *
    ***************************************************************/
    /**
     * @brief Register callback for discovering service.
     */
    void onServiceDiscoveredCallback(gattServicesDiscoveredCallback_t cb);

    /**
     * @brief Register callback for discovering characteristic.
     */
    void onCharacteristicDiscoveredCallback(gattCharsDiscoveredCallback_t cb);

    /**
     * @brief Register callback for discovering descriptors of characteristic.
     */
    void onDescriptorDiscoveredCallback(gattDescriptorsDiscoveredCallback_t cb);

    /**
     * @brief Register callback for reading characteristic value.
     */
    void onGattCharacteristicReadCallback(gattCharacteristicReadCallback_t cb);

    /**
     * @brief Register callback for writing characteristic value.
     */
    void onGattCharacteristicWrittenCallback(gattCharacteristicWrittenCallback_t cb);

    /**
     * @brief Register callback for reading characteristic descriptor value.
     */
    void onGattDescriptorReadCallback(gattDescriptorReadCallback_t cb);

    /**
     * @brief Register callback for writing characteristic descriptor value.
     */
    void onGattDescriptorWrittenCallback(gattDescriptorWrittenCallback_t cb);

    /**
     * @brief Register callback for enable/disable client characteristic configuration.
     */
    void onGattWriteClientCharacteristicConfigCallback(gattWriteCCCDCallback_t cb);

    /**
     * @brief Register callback for notify value update.
     */
    void onGattNotifyUpdateCallback(gattNotifyUpdateCallback_t cb);

    /**
     * @brief Register callback for indicate value update.
     */
    void onGattIndicateUpdateCallback(gattIndicateUpdateCallback_t cb);

    /**
     * @brief Discover primary service by conn_handle.
     *        BLE_STATUS_OK : Found a service, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @param[in]  con_handle
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t discoverPrimaryServices(uint16_t con_handle);

    /**
     * @brief Discover primary service given its 16bits-UUID.
     *        BLE_STATUS_OK : Found a service, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @param[in]  con_handle
     * @param[in]  uuid16
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t discoverPrimaryServices(uint16_t con_handle, uint16_t uuid16);

    /**
     * @brief Discover primary service given its 128bits-UUID.
     *        BLE_STATUS_OK : Found a service, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @param[in]  con_handle
     * @param[in]  *uuid        Buffer of 128bits-UUID.
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t discoverPrimaryServices(uint16_t con_handle, const uint8_t *uuid);

    /**
     * @brief Discover all characteristics of a service.
     *        BLE_STATUS_OK : Found a characteristic, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @param[in]  con_handle
     * @param[in]  *service
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t discoverCharacteristics(uint16_t con_handle, gatt_client_service_t *service);

    /**
     * @brief Discover the characteristics given UUID in handle range.
     *        BLE_STATUS_OK : Found a characteristic, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @param[in]  con_handle
     * @param[in]  start_handle
     * @param[in]  end_handle
     * @param[in]  uuid16
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t discoverCharacteristics(uint16_t con_handle, uint16_t start_handle, uint16_t end_handle, uint16_t uuid16);

    /**
     * @brief Discover the characteristics given UUID in handle range.
     *        BLE_STATUS_OK : Found a characteristic, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @param[in]  con_handle
     * @param[in]  start_handle
     * @param[in]  end_handle
     * @param[in]  *uuid           Buffer of 128bits-UUID.
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t discoverCharacteristics(uint16_t con_handle, uint16_t start_handle, uint16_t end_handle, uint8_t *uuid);

    /**
     * @brief Discover the characteristics given UUID of a service.
     *        BLE_STATUS_OK : Found a characteristic, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @param[in]  con_handle
     * @param[in]  *service
     * @param[in]  uuid16
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t discoverCharacteristics(uint16_t con_handle, gatt_client_service_t *service, uint16_t uuid16);

    /**
     * @brief Discover the characteristics given UUID of a service.
     *        BLE_STATUS_OK : Found a characteristic, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @param[in]  con_handle
     * @param[in]  *service
     * @param[in]  *uuid           Buffer of 128bits-UUID.
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t discoverCharacteristics(uint16_t con_handle, gatt_client_service_t *service, uint8_t *uuid128);


    /**
     * @brief Discover descriptors of a characteristic.
     *        BLE_STATUS_OK : Found a descriptor, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @param[in]  con_handle
     * @param[in]  *characteristic
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t discoverCharacteristicDescriptors(uint16_t con_handle, gatt_client_characteristic_t *characteristic);

    /**
     * @brief Reads the characteristic value.
     *        BLE_STATUS_OK : Found characteristic value, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @param[in]  con_handle
     * @param[in]  *characteristic
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t readValue(uint16_t con_handle, gatt_client_characteristic_t  *characteristic);

    /**
     * @brief Reads the characteristic value using the characteristic's value handle.
     *        BLE_STATUS_OK : Found characteristic value, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @param[in]  con_handle
     * @param[in]  characteristic_value_handle
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t readValue(uint16_t con_handle, uint16_t characteristic_value_handle);

    /**
     * @brief Reads the characteristic value given a uuid at a range.
     *        BLE_STATUS_OK : Found characteristic value, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @param[in]  con_handle
     * @param[in]  start_handle
     * @param[in]  end_handle
     * @param[in]  uuid16
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t readValue(uint16_t con_handle, uint16_t start_handle, uint16_t end_handle, uint16_t uuid16);

    /**
     * @brief Reads the characteristic value given a uuid at a range.
     *        BLE_STATUS_OK : Found characteristic value, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @param[in]  con_handle
     * @param[in]  start_handle
     * @param[in]  end_handle
     * @param[in]  *uuid128
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t readValue(uint16_t con_handle, uint16_t start_handle, uint16_t end_handle, uint8_t *uuid128);

    /**
     * @brief Reads the long characteristic value.
     *        BLE_STATUS_OK : Found characteristic value, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @param[in]  con_handle
     * @param[in]  *characteristic
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t readLongValue(uint16_t con_handle, gatt_client_characteristic_t *characteristic);

    /**
     * @brief Reads the long characteristic value.
     *        BLE_STATUS_OK : Found characteristic value, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @param[in]  con_handle
     * @param[in]  characteristic_value_handle
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t readLongValue(uint16_t con_handle, uint16_t characteristic_value_handle);

    /**
     * @brief Reads the long characteristic value with offset.
     *        BLE_STATUS_OK : Found characteristic value, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @param[in]  con_handle
     * @param[in]  characteristic_value_handle
     * @param[in]  offset
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t readLongValueWithOffset(uint16_t con_handle, uint16_t characteristic_value_handle, uint16_t offset);

    /**
     * @brief Writes the characteristic value using the characteristic's value handle without an acknowledgment that the write was successfully performed.
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         GATT_CLIENT_VALUE_TOO_LONG
     *         GATT_CLIENT_BUSY
     *         0::SUCCESS
     */
    uint8_t writeValueWithoutResponse(uint16_t con_handle, uint16_t characteristic_value_handle, uint16_t length, uint8_t *data);

    /**
     * @brief Writes the characteristic value.
     *        BLE_STATUS_DONE : Marks the end of write.
     *        BLE_STATUS_OTHER_ERROR : Error.
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
    uint8_t writeValue(uint16_t con_handle, uint16_t characteristic_value_handle, uint16_t length, uint8_t * data);

    /**
     * @brief Writes the long characteristic value.
     *        BLE_STATUS_DONE : Marks the end of write.
     *        BLE_STATUS_OTHER_ERROR : Error.
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
    uint8_t writeLongValue(uint16_t con_handle, uint16_t characteristic_value_handle, uint16_t length, uint8_t *data);

    /**
     * @brief Writes the long characteristic value.
     *        BLE_STATUS_DONE : Marks the end of write.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @param[in]  con_handle
     * @param[in]  characteristic_value_handle
     * @param[in]  offset
     * @param[in]  length
     * @param[in]  *data
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t writeLongValueWithOffset(uint16_t con_handle, uint16_t characteristic_value_handle, uint16_t offset, uint16_t length, uint8_t *data);

    /**
     * @brief Reads the descriptor value using descriptor handle.
     *        BLE_STATUS_OK : Found descriptor value, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @param[in]  con_handle
     * @param[in]  *descriptor
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t readDescriptorValue(uint16_t con_handle, gatt_client_characteristic_descriptor_t *descriptor);

    /**
     * @brief Reads the descriptor value.
     *        BLE_STATUS_OK : Found descriptor value, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @param[in]  con_handle
     * @param[in]  descriptor_handle
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t readDescriptorValue(uint16_t con_handle, uint16_t descriptor_handle);

    /**
     * @brief Reads the descriptor value.
     *        BLE_STATUS_OK : Found descriptor value, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t readLongDescriptorValue(uint16_t con_handle, gatt_client_characteristic_descriptor_t *descriptor);

    /**
     * @brief Reads the descriptor value.
     *        BLE_STATUS_OK : Found descriptor value, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t readLongDescriptorValue(uint16_t con_handle, uint16_t descriptor_handle);

    /**
     * @brief Reads the descriptor value.
     *        BLE_STATUS_OK : Found descriptor value, the status BLE_STATUS_OK will be generated and passed to the registered callback.
     *        BLE_STATUS_DONE : Marks the end of discovery.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t readLongDescriptorValueWithOffset(uint16_t con_handle, uint16_t descriptor_handle, uint16_t offset);

    /**
     * @brief Writes the descriptor value.
     *        BLE_STATUS_DONE : Marks the end of write.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @note  Waiting for testing.
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t writeDescriptorValue(uint16_t con_handle, gatt_client_characteristic_descriptor_t *descriptor, uint16_t length, uint8_t *data);

    /**
     * @brief Writes the descriptor value.
     *        BLE_STATUS_DONE : Marks the end of write.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @note  Waiting for testing.
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t writeDescriptorValue(uint16_t con_handle, uint16_t descriptor_handle, uint16_t length, uint8_t *data);

    /**
     * @brief Writes the descriptor value.
     *        BLE_STATUS_DONE : Marks the end of write.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @note  Waiting for testing.
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t writeLongDescriptorValue(uint16_t con_handle, gatt_client_characteristic_descriptor_t *descriptor, uint16_t length, uint8_t *data);

    /**
     * @brief Writes the descriptor value.
     *        BLE_STATUS_DONE : Marks the end of write.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @note  Waiting for testing.
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t writeLongDescriptorValue(uint16_t con_handle, uint16_t descriptor_handle, uint16_t length, uint8_t *data);

    /**
     * @brief Writes the descriptor value.
     *        BLE_STATUS_DONE : Marks the end of write.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @note  Waiting for testing.
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         0::SUCCESS
     */
    uint8_t writeLongDescriptorValueWithOffset(uint16_t con_handle, uint16_t descriptor_handle, uint16_t offset, uint16_t length, uint8_t *data);

    /**
     * @brief Writes the client characteristic configuration descriptor.
     *        BLE_STATUS_DONE : Marks the end of write.
     *        BLE_STATUS_OTHER_ERROR : Error.
     *
     * @param[in]  con_handle
     * @param[in]  characteristic
     * @param[in]  configuration     GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NONE
     *                               GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION
     *                               GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_INDICATION
     *
     * @return BTSTACK_MEMORY_ALLOC_FAILED
     *         GATT_CLIENT_IN_WRONG_STATE
     *         GATT_CLIENT_CHARACTERISTIC_NOTIFICATION_NOT_SUPPORTED
     *         GATT_CLIENT_CHARACTERISTIC_INDICATION_NOT_SUPPORTED
     *         0::SUCCESS
     */
    uint8_t writeClientCharsConfigDescritpor(uint16_t con_handle, gatt_client_characteristic_t *characteristic, uint16_t configuration);

};



extern BLEDevice ble;

#endif

#endif
