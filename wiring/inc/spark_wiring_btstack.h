
#ifndef __SPARK_WIRING_BTSTACK_H
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

    // Device API.
    void init(void);
    void deInit(void);

    void setTimer(btstack_timer_source_t *ts, uint32_t timeout_in_ms);
    void setTimerHandler(btstack_timer_source_t *ts, void (*process)(btstack_timer_source_t *_ts));
    void addTimerToLoop(btstack_timer_source_t *timer);
    int  removeTimerFromLoop(btstack_timer_source_t *timer);
    uint32_t getTimeMs(void);

    void debugLogger(bool flag);
    void debugError(bool flag);
    void enablePacketLogger(void);

    // Gap API.
    void setRandomAddrMode(gap_random_address_type_t random_addr_type);
    void setRandomAddr(bd_addr_t addr);
    void setPublicBDAddr(bd_addr_t addr);
    void setLocalName(const char *local_name);
    void setAdvParams(advParams_t *adv_params);
    void setAdvData(uint16_t size, uint8_t *data);

    void onConnectedCallback(void (*callback)(BLEStatus_t status, uint16_t handle));
    void onDisconnectedCallback(void (*callback)(uint16_t handle));

    void startAdvertising(void);
    void stopAdvertising(void);

    void disconnect(uint16_t conn_handle);
    uint8_t connect(bd_addr_t addr, bd_addr_type_t type);

    void setConnParams(le_connection_parameter_range_t range);

    void startScanning(void);
    void stopScanning(void);

    void setScanParams(uint8_t scan_type, uint16_t scan_interval, uint16_t scan_window);
    void onScanReportCallback(void (*cb)(advertisementReport_t *advertisement_report));

    // Gatt server API.
    void addService(uint16_t uuid);
    void addService(uint8_t *uuid);

    uint16_t addCharacteristic(uint16_t uuid, uint16_t flags, uint8_t *data, uint16_t data_len);
    uint16_t addCharacteristic(uint8_t *uuid, uint16_t flags, uint8_t *data, uint16_t data_len);
    uint16_t addCharacteristicDynamic(uint16_t uuid, uint16_t flags, uint8_t *data, uint16_t data_len);
    uint16_t addCharacteristicDynamic(uint8_t *uuid, uint16_t flags, uint8_t *data, uint16_t data_len);

    void onDataReadCallback(uint16_t (*cb)(uint16_t handle, uint8_t *buffer, uint16_t buffer_size));
    void onDataWriteCallback(int (*cb)(uint16_t handle, uint8_t *buffer, uint16_t buffer_size));

    int attServerCanSendPacket(void);
    int sendNotify(uint16_t value_handle, uint8_t *value, uint16_t length);
    int sendIndicate(uint16_t value_handle, uint8_t *value, uint16_t length);

    // Gatt client API.
    void onServiceDiscoveredCallback(void (*cb)(BLEStatus_t status, gatt_client_service_t *service));
    void onCharacteristicDiscoveredCallback(void (*cb)(BLEStatus_t status, gatt_client_characteristic_t *characteristic));
    void onCharsDescriptorDiscoveredCallback(void (*cb)(BLEStatus_t status, gatt_client_characteristic_descriptor_t *characteristic));

    uint8_t discoverPrimaryServices(uint16_t con_handle);
    uint8_t discoverPrimaryServicesByUUID16(uint16_t con_handle, uint16_t uuid16);
    uint8_t discoverPrimaryServicesByUUID128(uint16_t con_handle, const uint8_t *uuid);

    uint8_t discoverCharacteristicsForService(uint16_t con_handle, gatt_client_service_t  *service);
    uint8_t discoverCharacteristicsForHandleRangeByUUID16(uint16_t con_handle, uint16_t start_handle, uint16_t end_handle, uint16_t uuid16);
    uint8_t discoverCharacteristicsForHandleRangeByUUID128(uint16_t con_handle, uint16_t start_handle, uint16_t end_handle, uint8_t *uuid);
    uint8_t discoverCharacteristicsForServiceByUUID16(uint16_t con_handle, gatt_client_service_t *service, uint16_t uuid16);
    uint8_t discoverCharacteristicsForServiceByUUID128(uint16_t con_handle, gatt_client_service_t *service, uint8_t *uuid128);

    uint8_t discoverCharacteristicDescriptors(uint16_t con_handle, gatt_client_characteristic_t *characteristic);
};



extern BLEDevice ble;

#endif

#endif
