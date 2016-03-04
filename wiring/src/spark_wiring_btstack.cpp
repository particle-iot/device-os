
#if PLATFORM_ID == 88 // Duo

#include "spark_wiring_btstack.h"

/***************************************************************
 *
 * Device API
 *
***************************************************************/
void BLEDevice::init(void)
{
    hal_btstack_init();
}

void BLEDevice::deInit(void)
{
    hal_btstack_deInit();
}

/**
 * @brief Set timer.
 */
void BLEDevice::setTimer(btstack_timer_source_t *ts, uint32_t timeout_in_ms)
{
    hal_btstack_setTimer(ts, timeout_in_ms);
}

/**
 * @brief Set timer handler..
 */
void BLEDevice::setTimerHandler(btstack_timer_source_t *ts, void (*process)(btstack_timer_source_t *_ts))
{
    hal_btstack_setTimerHandler(ts, process);
}

/**
 * @brief Add/Remove timer task to run loop.
 */
void BLEDevice::addTimerToLoop(btstack_timer_source_t *timer)
{
    hal_btstack_addTimer(timer);
}
int BLEDevice::removeTimerFromLoop(btstack_timer_source_t *timer)
{
    return hal_btstack_removeTimer(timer);
}

/**
 * @brief Get the time of system running.
 */
uint32_t BLEDevice::getTimeMs(void)
{
    return hal_btstack_getTimeMs();
}

/**
 * @brief Enable/Disable the logger info in btstack library.
 *
 * @Note Use usb_hal.
 */
void BLEDevice::debugLogger(bool flag)
{
    hal_btstack_debugLogger((uint8_t)flag);
}
void BLEDevice::debugError(bool flag)
{
    hal_btstack_debugError(flag);
}
void BLEDevice::enablePacketLogger(void)
{
    hal_btstack_enablePacketLogger();
}

/***************************************************************
 *
 * Gap API
 *
***************************************************************/
/**
 * @brief Set random mode.
 */
void BLEDevice::setRandomAddrMode(gap_random_address_type_t random_addr_type)
{
    hal_btstack_setRandomAddressMode(random_addr_type);
}

void BLEDevice::setRandomAddr(bd_addr_t addr)
{
    hal_btstack_setRandomAddr(addr);
}

void BLEDevice::setPublicBDAddr(bd_addr_t addr)
{
    hal_btstack_setPublicBdAddr(addr);
}

void BLEDevice::setLocalName(const char *local_name)
{
    hal_btstack_setLocalName(local_name);
}

void BLEDevice::setAdvParams(advParams_t *adv_params)
{
    hal_btstack_setAdvParams(adv_params->adv_int_min, adv_params->adv_int_max, \
                             adv_params->adv_type, \
                             adv_params->dir_addr_type, adv_params->dir_addr, \
                             adv_params->channel_map, \
                             adv_params->filter_policy);
}

void BLEDevice::setAdvData(uint16_t size, uint8_t *data)
{
    hal_btstack_setAdvData(size, data);
}

void BLEDevice::onConnectedCallback(void (*callback)(BLEStatus_t status, uint16_t handle))
{
    hal_btstack_setConnectedCallback(callback);
}

void BLEDevice::onDisconnectedCallback(void (*callback)(uint16_t handle))
{
    hal_btstack_setDisconnectedCallback(callback);
}

void BLEDevice::startAdvertising(void)
{
    hal_btstack_startAdvertising();
}

void BLEDevice::stopAdvertising(void)
{
    hal_btstack_stopAdvertising();
}

void BLEDevice::disconnect(uint16_t conn_handle)
{
    hal_btstack_disconnect(conn_handle);
}

uint8_t BLEDevice::connect(bd_addr_t addr, bd_addr_type_t type)
{
    return hal_btstack_connect(addr, type);
}

void BLEDevice::setConnParams(le_connection_parameter_range_t range)
{
    hal_btstack_setConnParamsRange(range);
}

void BLEDevice::startScanning(void)
{
    hal_btstack_startScanning();
}

void BLEDevice::stopScanning(void)
{
    hal_btstack_stopScanning();
}

void BLEDevice::setScanParams(uint8_t scan_type, uint16_t scan_interval, uint16_t scan_window)
{
    hal_btstack_setScanParams(scan_type, scan_interval, scan_window);
}

void BLEDevice::onScanReportCallback(void (*cb)(advertisementReport_t *advertisement_report))
{
    hal_btstack_setBLEAdvertisementCallback(cb);
}

/***************************************************************
 *
 * Gatt server API
 *
***************************************************************/
void BLEDevice::addService(uint16_t uuid)
{
    hal_btstack_addServiceUUID16bits(uuid);
}

void BLEDevice::addService(uint8_t *uuid)
{
    hal_btstack_addServiceUUID128bits(uuid);
}

/**
 * @brief Add characteristic.
 *
 * @Note Just has read permit, if need to write,use "addCharacteristicDynamic".
 */
uint16_t BLEDevice::addCharacteristic(uint16_t uuid, uint16_t flags, uint8_t *data, uint16_t data_len)
{
    return hal_btstack_addCharsUUID16bits(uuid, flags, data, data_len);
}
uint16_t BLEDevice::addCharacteristic(uint8_t *uuid, uint16_t flags, uint8_t *data, uint16_t data_len)
{
    return hal_btstack_addCharsUUID128bits(uuid, flags, data, data_len);
}

uint16_t BLEDevice::addCharacteristicDynamic(uint16_t uuid, uint16_t flags, uint8_t *data, uint16_t data_len)
{
    return hal_btstack_addCharsDynamicUUID16bits(uuid, flags, data, data_len);
}

uint16_t BLEDevice::addCharacteristicDynamic(uint8_t *uuid, uint16_t flags, uint8_t *data, uint16_t data_len)
{
    return hal_btstack_addCharsDynamicUUID128bits(uuid, flags, data, data_len);
}

void BLEDevice::onDataReadCallback(uint16_t (*cb)(uint16_t handle, uint8_t *buffer, uint16_t buffer_size))
{
    hal_btstack_setGattCharsRead(cb);
}

void BLEDevice::onDataWriteCallback(int (*cb)(uint16_t handle, uint8_t *buffer, uint16_t buffer_size))
{
    hal_btstack_setGattCharsWrite(cb);
}

int BLEDevice::attServerCanSendPacket(void)
{
    return hal_btstack_attServerCanSend();
}

int BLEDevice::sendNotify(uint16_t value_handle, uint8_t *value, uint16_t length)
{
    return hal_btstack_attServerSendNotify(value_handle, value, length);
}

int BLEDevice::sendIndicate(uint16_t value_handle, uint8_t *value, uint16_t length)
{
    return hal_btstack_attServerSendIndicate(value_handle, value, length);
}

/***************************************************************
 *
 * Gatt client API
 *
***************************************************************/
void BLEDevice::onServiceDiscoveredCallback(void (*cb)(BLEStatus_t status, gatt_client_service_t *service))
{
	hal_btstack_setGattServiceDiscoveredCallback(cb);
}

void BLEDevice::onCharacteristicDiscoveredCallback(void (*cb)(BLEStatus_t status, gatt_client_characteristic_t *characteristic))
{
	hal_btstack_setGattCharsDiscoveredCallback(cb);
}

void BLEDevice::onCharsDescriptorDiscoveredCallback(void (*cb)(BLEStatus_t status, gatt_client_characteristic_descriptor_t *characteristic))
{
	hal_btstack_setGattCharsDescriptorsDiscoveredCallback(cb);
}

uint8_t BLEDevice::discoverPrimaryServices(uint16_t con_handle)
{
	return hal_btstack_discoverPrimaryServices(con_handle);
}

uint8_t BLEDevice::discoverPrimaryServicesByUUID16(uint16_t con_handle, uint16_t uuid16)
{
	return hal_btstack_discoverPrimaryServicesByUUID16(con_handle, uuid16);
}

uint8_t BLEDevice::discoverPrimaryServicesByUUID128(uint16_t con_handle, const uint8_t *uuid)
{
	return hal_btstack_discoverPrimaryServicesByUUID128(con_handle, uuid);
}

uint8_t BLEDevice::discoverCharacteristicsForService(uint16_t con_handle, gatt_client_service_t  *service)
{
	return hal_btstack_discoverCharsForService(con_handle, service);
}

uint8_t BLEDevice::discoverCharacteristicsForHandleRangeByUUID16(uint16_t con_handle, uint16_t start_handle, uint16_t end_handle, uint16_t uuid16)
{
	return hal_btstack_discoverCharsForHandleRangeByUUID16(con_handle, start_handle, end_handle, uuid16);
}

uint8_t BLEDevice::discoverCharacteristicsForHandleRangeByUUID128(uint16_t con_handle, uint16_t start_handle, uint16_t end_handle, uint8_t *uuid)
{
	return hal_btstack_discoverCharsForHandleRangeByUUID128(con_handle, start_handle, end_handle, uuid);
}

uint8_t BLEDevice::discoverCharacteristicsForServiceByUUID16(uint16_t con_handle, gatt_client_service_t *service, uint16_t uuid16)
{
	return hal_btstack_discoverCharsForServiceByUUID16(con_handle, service, uuid16);
}

uint8_t BLEDevice::discoverCharacteristicsForServiceByUUID128(uint16_t con_handle, gatt_client_service_t *service, uint8_t *uuid128)
{
	return hal_btstack_discoverCharsForServiceByUUID128(con_handle, service, uuid128);
}

uint8_t BLEDevice::discoverCharacteristicDescriptors(uint16_t con_handle, gatt_client_characteristic_t *characteristic)
{
	return hal_btstack_discoverCharsDescriptors(con_handle, characteristic);
}

BLEDevice ble;

#endif
