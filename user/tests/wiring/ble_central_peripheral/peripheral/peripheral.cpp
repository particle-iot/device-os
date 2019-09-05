#include "Particle.h"
#include "unit-test/unit-test.h"

#if Wiring_BLE == 1
void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

const char* serviceUuid = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
const char* rxUuid = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
const char* txUuid = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

const String str1("This is string 1.");
const String str2("Hello from Particle.");
const String str3("Have a good day!");
bool str1Rec = false, str2Rec = false, str3Rec = false;

BleCharacteristic txCharacteristic("tx",
                                   BleCharacteristicProperty::NOTIFY | BleCharacteristicProperty::INDICATE,
                                   txUuid,
                                   serviceUuid);

BleCharacteristic rxCharacteristic("rx",
                                   BleCharacteristicProperty::WRITE_WO_RSP | BleCharacteristicProperty::WRITE,
                                   rxUuid,
                                   serviceUuid,
                                   onDataReceived, &rxCharacteristic);

void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
    String str((const char*)data, len);
    if (str == str1) {
        str1Rec = true;
    }
    if (str == str2) {
        str2Rec = true;
    }
    if (str == str3) {
        str3Rec = true;
    }
}

test(BLE_01_Peripheral_Advertising) {
    int ret;
    BleCharacteristic temp;

    temp = BLE.addCharacteristic(txCharacteristic);
    assertTrue(temp.valid());
    temp = BLE.addCharacteristic(rxCharacteristic);
    assertTrue(temp.valid());

    BleAdvertisingData data;
    data.appendServiceUUID(serviceUuid);
    ret = BLE.advertise(&data);
    assertEqual(ret, 0);

    Serial.println("BLE starts advertising...");

    assertTrue(BLE.advertising());
}

test(BLE_02_Peripheral_Connected) {
    size_t wait = 200; // Wait 20s for establishing connection.
    while(!BLE.connected() && wait > 0) {
        delay(100);
        wait--;
    }
    assertTrue(wait > 0);

    Serial.println("BLE connected.");
}

test(BLE_03_Peripheral_Receive_Characteristic_Value_String1) {
    size_t wait = 50; //Wait for 5s to receive the data from BLE central.
    while (!str1Rec && wait > 0) {
        delay(100);
        wait--;
    }
    assertTrue(wait > 0);
}

test(BLE_04_Peripheral_Receive_Characteristic_Value_String2) {
    size_t wait = 50; //Wait for 5s to receive the data from BLE central.
    while (!str2Rec && wait > 0) {
        delay(100);
        wait--;
    }
    assertTrue(wait > 0);
}

test(BLE_05_Peripheral_Receive_Characteristic_Value_String3) {
    size_t wait = 50; //Wait for 5s to receive the data from BLE central.
    while (!str3Rec && wait > 0) {
        delay(100);
        wait--;
    }
    assertTrue(wait > 0);
}

test(BLE_06_Peripheral_Send_Characteristic_Value_With_Auto_Ack) {
    int ret = txCharacteristic.setValue(str1);
    assertTrue(ret == str1.length());
}

test(BLE_07_Peripheral_Send_Characteristic_Value_With_Ack) {
    int ret = txCharacteristic.setValue(str2, BleTxRxType::ACK);
    assertTrue(ret == str2.length());
}

test(BLE_08_Peripheral_Send_Characteristic_Value_Without_Ack) {
    int ret = txCharacteristic.setValue(str3, BleTxRxType::NACK);
    assertTrue(ret == str3.length());
}

#endif // #if Wiring_BLE == 1

