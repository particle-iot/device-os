#include "Particle.h"
#include "unit-test/unit-test.h"

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

test(BLE_Peripheral_Advertising_Connected_Data_Transfer) {
    int ret;
    BleCharacteristic temp;

    // Make sure that the serial terminal is connected to device before printing message.
    delay(5000);

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

    size_t wait = 200; // Wait 20s for establishing connection.
    while(!BLE.connected() && wait > 0) {
        delay(100);
        wait--;
    }
    assertTrue(wait > 0);

    Serial.println("BLE connected.");

    // Make sure that the central has discovered services and characteristics.
    delay(1000);

    Serial.println("Sends data according to the characteristic property.");
    size_t len = txCharacteristic.setValue(str1);
    assertTrue(len == str1.length());

    Serial.println("Sends data with explicit ACK required.");
    ret = txCharacteristic.setValue(str2, BleTxRxType::ACK);
    assertTrue(ret == str2.length());

    Serial.println("Sends data without ACK required.");
    ret = txCharacteristic.setValue(str3, BleTxRxType::NACK);
    assertTrue(ret == str3.length());

    wait = 10; //Wait for 10s to receive the data from BLE central.
    while (!(str1Rec && str2Rec && str3Rec) && wait > 0) {
        delay(100);
        wait--;
    }
}

