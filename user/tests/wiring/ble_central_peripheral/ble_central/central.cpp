#include "Particle.h"
#include "unit-test/unit-test.h"

#if Wiring_BLE == 1

#define SCAN_RESULT_COUNT       20

BleScanResult results[SCAN_RESULT_COUNT];

BleCharacteristic peerCharRead;
BleCharacteristic peerCharWrite;
BleCharacteristic peerCharWriteWoRsp;
BleCharacteristic peerCharWriteAndWriteWoRsp;
BleCharacteristic peerCharNotify;
BleCharacteristic peerCharIndicate;
BleCharacteristic peerCharNotifyAndIndicate;
BlePeerDevice peer;

const String str1("05a9ae9588dd");
const String str2("4f62bd40046a");
const String str3("6619918032a2");
const String str4("359b4deb3f37");
const String str5("a2ef18ec6eaa");
const String str6("7223b5dd3342");
const String str7("d4b4249bbbe3");

bool str1Rec = false, str2Rec = false, str3Rec = false, str4Rec = false, str5Rec = false, str6Rec = false, str7Rec = false;

static void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
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
    if (str == str4) {
        str4Rec = true;
    }
    if (str == str5) {
        str5Rec = true;
    }
    if (str == str6) {
        str6Rec = true;
    }
    if (str == str7) {
        str7Rec = true;
    }
}

test(BLE_01_Central_Scan_And_Connect) {
    peerCharNotify.onDataReceived(onDataReceived, &peerCharNotify);
    peerCharIndicate.onDataReceived(onDataReceived, &peerCharIndicate);
    peerCharNotifyAndIndicate.onDataReceived(onDataReceived, &peerCharNotifyAndIndicate);

    int ret = BLE.setScanTimeout(100); // Scan timeout: 1s
    assertEqual(ret, 0);

    Serial.println("BLE starts scanning...");

    size_t wait = 20; // Scanning for 20s.
    while (!BLE.connected() && wait > 0) {
        size_t count = BLE.scan(results, SCAN_RESULT_COUNT);
        if (count > 0) {
            for (uint8_t i = 0; i < count; i++) {
                BleUuid foundServiceUUID;
                size_t svcCount = results[i].advertisingData.serviceUUID(&foundServiceUUID, 1);
                if (svcCount > 0 && foundServiceUUID == "6E400000-B5A3-F393-E0A9-E50E24DCCA9E") {
                    peer = BLE.connect(results[i].address);
                    if (peer.connected()) {
                        assertTrue(peer.getCharacteristicByDescription(peerCharRead, "read"));
                        assertTrue(peer.getCharacteristicByDescription(peerCharWrite, "write"));
                        assertTrue(peer.getCharacteristicByDescription(peerCharWriteWoRsp, "write_wo_rsp"));
                        assertTrue(peer.getCharacteristicByDescription(peerCharWriteAndWriteWoRsp, "write_write_wo_rsp"));
                        assertTrue(peer.getCharacteristicByUUID(peerCharNotify, "6E400005-B5A3-F393-E0A9-E50E24DCCA9E"));
                        assertTrue(peer.getCharacteristicByUUID(peerCharIndicate, "6E400006-B5A3-F393-E0A9-E50E24DCCA9E"));
                        assertTrue(peer.getCharacteristicByUUID(peerCharNotifyAndIndicate, "6E400007-B5A3-F393-E0A9-E50E24DCCA9E"));
                    }
                    break;
                }
            }
        }
        wait--;
    }
    assertTrue(wait > 0);

    Serial.println("BLE connected.");
}

test(BLE_02_Central_Read_Peer_characteristic_With_Read_Property) {
    const char* str = "6dd902629e1d";
    String getStr;
    int ret = peerCharRead.getValue(getStr);
    assertTrue(ret > 0);
    assertTrue(getStr == str);
}

test(BLE_03_Central_Write_Characteristic_With_Write_Property_Auto) {
    const String str("6b4bf92a37f3");
    int ret = peerCharWrite.setValue(str);
    assertTrue(ret == (int)str.length());
}

test(BLE_04_Central_Write_Characteristic_With_Write_Property_Ack) {
    const String str("df3b41caedac");
    int ret = peerCharWrite.setValue(str, BleTxRxType::ACK);
    assertTrue(ret == (int)str.length());
}

test(BLE_05_Central_Write_Characteristic_With_Write_Property_Nack) {
    const String str("febe08cc1f96");
    int ret = peerCharWrite.setValue(str, BleTxRxType::NACK);
    assertTrue(ret < 0);
}

test(BLE_06_Central_Write_Characteristic_With_Write_Wo_Rsp_Property_Auto) {
    const String str("2ad4bffbb8c7");
    int ret = peerCharWriteWoRsp.setValue(str);
    assertTrue(ret == (int)str.length());
}

test(BLE_07_Central_Write_Characteristic_With_Write_Wo_Rsp_Property_Ack) {
    const String str("ad2cb5697c37");
    int ret = peerCharWriteWoRsp.setValue(str, BleTxRxType::ACK);
    assertTrue(ret < 0);
}

test(BLE_08_Central_Write_Characteristic_With_Write_Wo_Rsp_Property_Nack) {
    const String str("203a02992be0");
    int ret = peerCharWriteWoRsp.setValue(str, BleTxRxType::NACK);
    assertTrue(ret == (int)str.length());
}

test(BLE_09_Central_Write_Characteristic_With_Write_Write_Wo_Rsp_Property_Auto) {
    const String str("86d7a840079f");
    int ret = peerCharWriteAndWriteWoRsp.setValue(str);
    assertTrue(ret == (int)str.length());
}

test(BLE_10_Central_Write_Characteristic_With_Write_Write_Wo_Rsp_Property_Ack) {
    const String str("77982c283c65");
    int ret = peerCharWriteAndWriteWoRsp.setValue(str, BleTxRxType::ACK);
    assertTrue(ret == (int)str.length());
}

test(BLE_11_Central_Write_Characteristic_With_Write_Write_Wo_Rsp_Property_Nack) {
    const String str("21ec57d28a0c");
    int ret = peerCharWriteAndWriteWoRsp.setValue(str, BleTxRxType::NACK);
    assertTrue(ret == (int)str.length());
}

test(BLE_12_Central_Received_Characteristic_With_Notify_Property_Auto) {
    size_t wait = 20; //Wait for 2s to receive the data from BLE peripheral.
    while (!str1Rec && wait > 0) {
        delay(100);
        wait--;
    }
    assertTrue(wait > 0);
}

test(BLE_13_Central_Received_Characteristic_With_Notify_Property_Nack) {
    size_t wait = 20; //Wait for 2s to receive the data from BLE peripheral.
    while (!str2Rec && wait > 0) {
        delay(100);
        wait--;
    }
    assertTrue(wait > 0);
}

test(BLE_14_Central_Received_Characteristic_With_Indicate_Property_Auto) {
    size_t wait = 20; //Wait for 2s to receive the data from BLE peripheral.
    while (!str3Rec && wait > 0) {
        delay(100);
        wait--;
    }
    assertTrue(wait > 0);
}

test(BLE_15_Central_Received_Characteristic_With_Indicate_Property_Ack) {
    size_t wait = 20; //Wait for 2s to receive the data from BLE peripheral.
    while (!str4Rec && wait > 0) {
        delay(100);
        wait--;
    }
    assertTrue(wait > 0);
}

test(BLE_16_Central_Received_Characteristic_With_Notify_Indicate_Property_Auto) {
    size_t wait = 20; //Wait for 2s to receive the data from BLE peripheral.
    while (!str5Rec && wait > 0) {
        delay(100);
        wait--;
    }
    assertTrue(wait > 0);
}

test(BLE_17_Central_Received_Characteristic_With_Notify_Indicate_Property_Ack) {
    size_t wait = 20; //Wait for 2s to receive the data from BLE peripheral.
    while (!str6Rec && wait > 0) {
        delay(100);
        wait--;
    }
    assertTrue(wait > 0);
}

test(BLE_18_Central_Received_Characteristic_With_Notify_Indicate_Property_Nack) {
    size_t wait = 20; //Wait for 2s to receive the data from BLE peripheral.
    while (!str7Rec && wait > 0) {
        delay(100);
        wait--;
    }
    assertTrue(wait > 0);
}

#endif // #if Wiring_BLE == 1

