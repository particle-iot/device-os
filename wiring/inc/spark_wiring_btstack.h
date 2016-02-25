
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

	void init(void);
	void deInit(void);

	void setTimer(hal_timer_source_t *ts, uint32_t timeout_in_ms);
	void setTimerHandler(hal_timer_source_t *ts, void (*process)(void *_ts));
	void addTimerToLoop(hal_timer_source_t *timer);
	int  removeTimerFromLoop(hal_timer_source_t *timer);
	uint32_t getTimeMs(void);

    void debugLogger(bool flag);
    void debugError(bool flag);
    void enablePacketLogger(void);


    void getAdvertisementAddr(uint8_t *addr_type, addr_t addr);
    void setRandomAddrMode(uint8_t random_addr_type);
    void setRandomAddr(addr_t addr);
    void setPublicBDAddr(addr_t addr);
    void setLocalName(const char *local_name);
    void setAdvParams(advParams_t *adv_params);
    void setAdvData(uint16_t size, uint8_t *data);

    void onConnectedCallback(void (*callback)(BLEStatus_t status, uint16_t handle));
    void onDisconnectedCallback(void (*callback)(uint16_t handle));

    void startAdvertising(void);
    void stopAdvertising(void);

    void disconect(uint16_t conn_handle);

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

    void startScanning(void);
    void stopScanning(void);

    void onScanReportCallback(void (*cb)(advertisementReport_t *advertisement_report));

};



extern BLEDevice ble;

#endif

#endif
