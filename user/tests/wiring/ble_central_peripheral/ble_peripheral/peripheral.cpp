#include "Particle.h"
#include "unit-test/unit-test.h"

#if Wiring_BLE == 1
void onDataReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

const char* serviceUuid = "6E400000-B5A3-F393-E0A9-E50E24DCCA9E";
const char* charReadUuid = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
const char* charWriteUuid = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
const char* charWriteWoRspUuid = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";
const char* charWriteAndWriteWoRspUuid = "6E400004-B5A3-F393-E0A9-E50E24DCCA9E";
const char* charNotifyUuid = "6E400005-B5A3-F393-E0A9-E50E24DCCA9E";
const char* charIndicateUuid = "6E400006-B5A3-F393-E0A9-E50E24DCCA9E";
const char* charNotifyAndIndicateUuid = "6E400007-B5A3-F393-E0A9-E50E24DCCA9E";

const String str1("6b4bf92a37f3");
const String str2("df3b41caedac");
const String str3("2ad4bffbb8c7");
const String str4("203a02992be0");
const String str5("86d7a840079f");
const String str6("77982c283c65");
const String str7("21ec57d28a0c");

bool str1Rec = false, str2Rec = false, str3Rec = false, str4Rec = false, str5Rec = false, str6Rec = false, str7Rec = false;
bool pairingStarted = false;

BleCharacteristic charRead("read", BleCharacteristicProperty::READ, charReadUuid, serviceUuid);

BleCharacteristic charWrite("write", BleCharacteristicProperty::WRITE, charWriteUuid, serviceUuid, onDataReceived, &charWrite);
BleCharacteristic charWriteWoRsp("write_wo_rsp", BleCharacteristicProperty::WRITE_WO_RSP, charWriteWoRspUuid, serviceUuid, onDataReceived, &charWriteWoRsp);
BleCharacteristic charWriteAndWriteWoRsp("write_write_wo_rsp", BleCharacteristicProperty::WRITE_WO_RSP | BleCharacteristicProperty::WRITE, charWriteAndWriteWoRspUuid, serviceUuid, onDataReceived, &charWriteAndWriteWoRsp);

BleCharacteristic charNotify("notify", BleCharacteristicProperty::NOTIFY, charNotifyUuid, serviceUuid);
BleCharacteristic charIndicate("indicate", BleCharacteristicProperty::INDICATE, charIndicateUuid, serviceUuid);
BleCharacteristic charNotifyAndIndicate("notify_indicate", BleCharacteristicProperty::NOTIFY | BleCharacteristicProperty::INDICATE, charNotifyAndIndicateUuid, serviceUuid);

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
    if (str == "deadbeef") {
        pairingStarted = true;
    }
}

test(BLE_01_Peripheral_Advertising) {
    int ret;
    BleCharacteristic temp;

    temp = BLE.addCharacteristic(charRead);
    assertTrue(temp.isValid());
    ret = charRead.setValue("6dd902629e1d");
    assertEqual(ret, 12);
    temp = BLE.addCharacteristic(charWrite);
    assertTrue(temp.isValid());
    temp = BLE.addCharacteristic(charWriteWoRsp);
    assertTrue(temp.isValid());
    temp = BLE.addCharacteristic(charWriteAndWriteWoRsp);
    assertTrue(temp.isValid());
    temp = BLE.addCharacteristic(charNotify);
    assertTrue(temp.isValid());
    temp = BLE.addCharacteristic(charIndicate);
    assertTrue(temp.isValid());
    temp = BLE.addCharacteristic(charNotifyAndIndicate);
    assertTrue(temp.isValid());

    BleAdvertisingData advData;
    advData.appendServiceUUID(serviceUuid);
    BleAdvertisingData srData;
    uint8_t uuids[] = {0x34, 0x12, 0x78, 0x56}; // little endian, i.e. 0x1234 and 0x5678.
    srData.append(BleAdvertisingDataType::SERVICE_UUID_16BIT_MORE_AVAILABLE, uuids, sizeof(uuids));
    ret = BLE.advertise(&advData, &srData);
    assertEqual(ret, 0);

    Serial.println("BLE starts advertising...");

    assertTrue(BLE.advertising());
}

test(BLE_02_Peripheral_Connected) {
    assertTrue(waitFor(BLE.connected, 20000));
    Serial.println("BLE connected.");
}

// For the first data transmission, we need to wait longer to make sure
// the Central device has discovered the services and characteristics.
test(BLE_03_Peripheral_Receive_Characteristic_With_Write_Property_Auto) {
    assertTrue(waitFor([]{ return str1Rec; }, 10000));
}

test(BLE_04_Peripheral_Receive_Characteristic_With_Write_Property_Ack) {
    assertTrue(waitFor([]{ return str2Rec; }, 2000));
}

test(BLE_05_Peripheral_Receive_Characteristic_With_Write_Wo_Rsp_Property_Auto) {
    assertTrue(waitFor([]{ return str3Rec; }, 2000));
}

test(BLE_06_Peripheral_Receive_Characteristic_With_Write_Wo_Rsp_Property_Nack) {
    assertTrue(waitFor([]{ return str4Rec; }, 2000));
}

test(BLE_07_Peripheral_Receive_Characteristic_With_Write_Write_Wo_Rsp_Property_Auto) {
    assertTrue(waitFor([]{ return str5Rec; }, 2000));
}

test(BLE_08_Peripheral_Receive_Characteristic_With_Write_Write_Wo_Rsp_Property_Ack) {
    assertTrue(waitFor([]{ return str6Rec; }, 2000));
}

test(BLE_09_Peripheral_Receive_Characteristic_With_Write_Write_Wo_Rsp_Property_Nack) {
    assertTrue(waitFor([]{ return str7Rec; }, 2000));
}

test(BLE_10_Peripheral_Notify_Characteristic_With_Notify_Property_Auto) {
    const String str("05a9ae9588dd");
    int ret = charNotify.setValue(str);
    assertTrue(ret == (int)str.length());
}

test(BLE_11_Peripheral_Notify_Characteristic_With_Notify_Property_Ack) {
    const String str("b551f6ca1329");
    int ret = charNotify.setValue(str, BleTxRxType::ACK);
    assertTrue(ret < 0);
}

test(BLE_12_Peripheral_Notify_Characteristic_With_Notify_Property_Nack) {
    const String str("4f62bd40046a");
    int ret = charNotify.setValue(str, BleTxRxType::NACK);
    assertTrue(ret == (int)str.length());
}

test(BLE_13_Peripheral_Notify_Characteristic_With_Indicate_Property_Auto) {
    const String str("6619918032a2");
    int ret = charIndicate.setValue(str);
    assertTrue(ret == (int)str.length());
}

test(BLE_14_Peripheral_Notify_Characteristic_With_Indicate_Property_Ack) {
    const String str("359b4deb3f37");
    int ret = charIndicate.setValue(str, BleTxRxType::ACK);
    assertTrue(ret == (int)str.length());
}

test(BLE_15_Peripheral_Notify_Characteristic_With_Indicate_Property_Nack) {
    const String str("06d0be9e185b");
    int ret = charIndicate.setValue(str, BleTxRxType::NACK);
    assertTrue(ret < 0);
}

test(BLE_16_Peripheral_Notify_Characteristic_With_Notify_Indicate_Property_Auto) {
    const String str("a2ef18ec6eaa");
    int ret = charNotifyAndIndicate.setValue(str);
    assertTrue(ret == (int)str.length());
}

test(BLE_17_Peripheral_Notify_Characteristic_With_Notify_Indicate_Property_Ack) {
    const String str("7223b5dd3342");
    int ret = charNotifyAndIndicate.setValue(str, BleTxRxType::ACK);
    assertTrue(ret == (int)str.length());
}

test(BLE_18_Peripheral_Notify_Characteristic_With_Notify_Indicate_Property_Nack) {
    const String str("d4b4249bbbe3");
    int ret = charNotifyAndIndicate.setValue(str, BleTxRxType::NACK);
    assertTrue(ret == (int)str.length());
}

test(BLE_19_Peripheral_Pairing_Sync) {
    // The central will perform some service and characteristic discovery tests
    // before starting the pairing tests
    assertTrue(waitFor([]{ return pairingStarted; }, 20000));
    assertTrue(waitFor([]{ return !BLE.connected(); }, 5000));
}

static bool pairingRequested = false;
static int pairingStatus = -1;
static bool lesc;
static BlePeerDevice peer;
const BlePairingIoCaps pairingIoCaps[5] = {
    BlePairingIoCaps::NONE,
    BlePairingIoCaps::DISPLAY_ONLY,
    BlePairingIoCaps::DISPLAY_YESNO,
    BlePairingIoCaps::KEYBOARD_ONLY,
    BlePairingIoCaps::KEYBOARD_DISPLAY
};
const char* ioCapsStr[5] = {
    "BlePairingIoCaps::NONE",
    "BlePairingIoCaps::DISPLAY_ONLY",
    "BlePairingIoCaps::DISPLAY_YESNO",
    "BlePairingIoCaps::KEYBOARD_ONLY",
    "BlePairingIoCaps::KEYBOARD_DISPLAY"
};

static void pairingTestRoutine(bool request, BlePairingAlgorithm algorithm) {
    pairingRequested = false;
    pairingStatus = -1;

    BLE.onPairingEvent([&](const BlePairingEvent& event) {
        if (event.type == BlePairingEventType::REQUEST_RECEIVED) {
            pairingRequested = true;
            peer = event.peer;
            // Serial.println("Request received");
        } else if (event.type == BlePairingEventType::STATUS_UPDATED) {
            pairingStatus = event.payload.status.status;
            lesc = event.payload.status.lesc;
            // Serial.println("status updateed");
        } else if (event.type == BlePairingEventType::PASSKEY_DISPLAY || event.type == BlePairingEventType::NUMERIC_COMPARISON) {
            Serial.print("Passkey display: ");
            for (uint8_t i = 0; i < BLE_PAIRING_PASSKEY_LEN; i++) {
                Serial.printf("%c", event.payload.passkey[i]);
            }
            Serial.println("");
            if (event.type == BlePairingEventType::NUMERIC_COMPARISON) {
                while (Serial.available()) {
                    Serial.read();
                }
                Serial.print("Please confirm if the passkey is identical (y/n): ");
                while (!Serial.available());
                char c = Serial.read();
                Serial.write(c);
                Serial.println("");
                BLE.setPairingNumericComparison(event.peer, (c == 'y') ? true : false);
            }
        } else if (event.type == BlePairingEventType::PASSKEY_INPUT) {
            while (Serial.available()) {
                Serial.read();
            }
            Serial.print("Passkey input (must be identical to the peer's): ");
            uint8_t i = 0;
            uint8_t passkey[BLE_PAIRING_PASSKEY_LEN];
            while (i < BLE_PAIRING_PASSKEY_LEN) {
                if (Serial.available()) {
                    passkey[i] = Serial.read();
                    Serial.write(passkey[i++]);
                }
            }
            Serial.println("");
            BLE.setPairingPasskey(event.peer, passkey);
        }
    });

    assertTrue(waitFor(BLE.connected, 20000));

    if (request) {
        peer = BLE.peerCentral();
        assertEqual(BLE.startPairing(peer), (int)SYSTEM_ERROR_NONE);
    } else {
        assertTrue(waitFor([&]{ return pairingRequested; }, 20000));
    }
    assertTrue(BLE.isPairing(peer));
    assertTrue(waitFor([&]{ return !BLE.isPairing(peer); }, 20000));
    assertTrue(BLE.isPaired(peer));
    assertEqual(pairingStatus, (int)SYSTEM_ERROR_NONE);
    if (algorithm != BlePairingAlgorithm::LEGACY_ONLY) {
        assertTrue(lesc);
    } else {
        assertFalse(lesc);
    }
    
    assertTrue(waitFor([]{ return !BLE.connected(); }, 5000));
    assertFalse(BLE.connected());
}

test(BLE_20_Peripheral_Pairing_Algorithm_Auto_Io_Caps) {
    for (uint8_t p = 0; p < 5; p++) { // Peer I/O capabilities
        for (uint8_t l = 0; l < 5; l++) {
            assertEqual(BLE.setPairingIoCaps(pairingIoCaps[l]), (int)SYSTEM_ERROR_NONE); // Local I/O capabilities
            Serial.printlnf("Local I/O Caps: %s", ioCapsStr[l]);
            pairingTestRoutine(!(p % 2), BlePairingAlgorithm::AUTO);
        }
    }
}

test(BLE_21_Peripheral_Pairing_Algorithm_Legacy_Only) {
    assertEqual(BLE.setPairingIoCaps(BlePairingIoCaps::NONE), (int)SYSTEM_ERROR_NONE);
    assertEqual(BLE.setPairingAlgorithm(BlePairingAlgorithm::LEGACY_ONLY), (int)SYSTEM_ERROR_NONE);
    pairingTestRoutine(false, BlePairingAlgorithm::LEGACY_ONLY);
}

test(BLE_22_Peripheral_Pairing_Algorithm_Legacy_Only_Being_Rejected) {
    pairingRequested = false;
    BLE.onPairingEvent([&](const BlePairingEvent& event) {
        if (event.type == BlePairingEventType::REQUEST_RECEIVED) {
            pairingRequested = true;
            peer = event.peer;
            // Serial.println("Request received");
        }
    });
    assertEqual(BLE.setPairingIoCaps(BlePairingIoCaps::NONE), (int)SYSTEM_ERROR_NONE);
    assertEqual(BLE.setPairingAlgorithm(BlePairingAlgorithm::LEGACY_ONLY), (int)SYSTEM_ERROR_NONE);

    assertTrue(waitFor(BLE.connected, 20000));

    assertTrue(waitFor([&]{ return pairingRequested; }, 20000));
    assertTrue(BLE.isPairing(peer));
    assertTrue(waitFor([&]{ return !BLE.isPairing(peer); }, 20000));
    assertFalse(BLE.isPaired(peer));

    assertTrue(waitFor([]{ return !BLE.connected(); }, 5000));
    assertFalse(BLE.connected());
}

test(BLE_23_Peripheral_Pairing_Receiption_Reject) {
    pairingStatus = 0;
    BLE.onPairingEvent([&](const BlePairingEvent& event) {
        if (event.type == BlePairingEventType::REQUEST_RECEIVED) {
            Serial.println("Reject pairing request.");
            BLE.rejectPairing(event.peer);
        } else if (event.type == BlePairingEventType::STATUS_UPDATED) {
            pairingStatus = event.payload.status.status;
        }
    });

    assertTrue(waitFor(BLE.connected, 20000));

    assertTrue(waitFor([]{ return pairingStatus != 0; }, 5000));

    assertTrue(waitFor([]{ return !BLE.connected(); }, 5000));
    assertFalse(BLE.connected());
}

test(BLE_24_Peripheral_Initiate_Pairing_Being_Rejected) {
    assertTrue(waitFor(BLE.connected, 20000));

    peer = BLE.peerCentral();
    assertEqual(BLE.startPairing(peer), (int)SYSTEM_ERROR_NONE);

    assertTrue(BLE.isPairing(peer));
    assertTrue(waitFor([&]{ return !BLE.isPairing(peer); }, 20000));
    assertFalse(BLE.isPaired(peer));

    assertTrue(waitFor([]{ return !BLE.connected(); }, 5000));
    assertFalse(BLE.connected());
}

#endif // #if Wiring_BLE == 1

