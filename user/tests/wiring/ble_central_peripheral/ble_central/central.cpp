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

BleAddress peerAddr;

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
                size_t svcCount = results[i].advertisingData().serviceUUID(&foundServiceUUID, 1);
                if (svcCount > 0 && foundServiceUUID == "6E400000-B5A3-F393-E0A9-E50E24DCCA9E") {
                    assertTrue(results[i].scanResponse().length() > 0);
                    BleUuid uuids[2];
                    assertEqual(results[i].scanResponse().serviceUUID(uuids, 2), 2);
                    assertTrue(uuids[0] == 0x1234);
                    assertTrue(uuids[1] == 0x5678);
                    peerAddr = results[i].address();
                    Serial.println("Connecting...");
                    peer = BLE.connect(peerAddr);
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
    assertTrue(waitFor([]{ return str1Rec; }, 2000));
}

test(BLE_13_Central_Received_Characteristic_With_Notify_Property_Nack) {
    assertTrue(waitFor([]{ return str2Rec; }, 2000));
}

test(BLE_14_Central_Received_Characteristic_With_Indicate_Property_Auto) {
    assertTrue(waitFor([]{ return str3Rec; }, 2000));
}

test(BLE_15_Central_Received_Characteristic_With_Indicate_Property_Ack) {
    assertTrue(waitFor([]{ return str4Rec; }, 2000));
}

test(BLE_16_Central_Received_Characteristic_With_Notify_Indicate_Property_Auto) {
    assertTrue(waitFor([]{ return str5Rec; }, 2000));
}

test(BLE_17_Central_Received_Characteristic_With_Notify_Indicate_Property_Ack) {
    assertTrue(waitFor([]{ return str6Rec; }, 2000));
}

test(BLE_18_Central_Received_Characteristic_With_Notify_Indicate_Property_Nack) {
    assertTrue(waitFor([]{ return str7Rec; }, 2000));

    BLE.disconnect(peer);
    int ret = BLE.setScanTimeout(500); // Scan timeout: 5s
    assertEqual(ret, 0);
}

test(BLE_19_Central_Discover_All_Services) {
    peer = BLE.connect(peerAddr, false);
    assertTrue(peer.connected());

    Vector<BleService> services = peer.discoverAllServices();
    BleService ctrlService;
    assertTrue(peer.getServiceByUUID(ctrlService, "6FA90001-5C4E-48A8-94F4-8030546F36FC"));
    BleService customService;
    assertTrue(peer.getServiceByUUID(customService, "6E400000-B5A3-F393-E0A9-E50E24DCCA9E"));

    BLE.disconnect(peer);
}

test(BLE_20_Central_Discover_All_Characteristics) {
    peer = BLE.connect(peerAddr, false);
    assertTrue(peer.connected());

    Vector<BleCharacteristic> allCharacteristics = peer.discoverAllCharacteristics();
    BleService ctrlService;
    assertTrue(peer.getServiceByUUID(ctrlService, "6FA90001-5C4E-48A8-94F4-8030546F36FC"));
    BleService customService;
    assertTrue(peer.getServiceByUUID(customService, "6E400000-B5A3-F393-E0A9-E50E24DCCA9E"));
    
    Vector<BleCharacteristic> characteristicsOfCtrlService = peer.characteristics(ctrlService);
    assertTrue(characteristicsOfCtrlService[0].UUID() == "6FA90002-5C4E-48A8-94F4-8030546F36FC");
    assertTrue(characteristicsOfCtrlService[1].UUID() == "6FA90003-5C4E-48A8-94F4-8030546F36FC");
    assertTrue(characteristicsOfCtrlService[2].UUID() == "6FA90004-5C4E-48A8-94F4-8030546F36FC");

    Vector<BleCharacteristic> characteristicsOfCustomService = peer.characteristics(customService);
    assertTrue(characteristicsOfCustomService[0].UUID() == "6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
    assertTrue(characteristicsOfCustomService[1].UUID() == "6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
    assertTrue(characteristicsOfCustomService[2].UUID() == "6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
    assertTrue(characteristicsOfCustomService[3].UUID() == "6E400004-B5A3-F393-E0A9-E50E24DCCA9E");
    assertTrue(characteristicsOfCustomService[4].UUID() == "6E400005-B5A3-F393-E0A9-E50E24DCCA9E");
    assertTrue(characteristicsOfCustomService[5].UUID() == "6E400006-B5A3-F393-E0A9-E50E24DCCA9E");
    assertTrue(characteristicsOfCustomService[6].UUID() == "6E400007-B5A3-F393-E0A9-E50E24DCCA9E");

    BLE.disconnect(peer);
}

test(BLE_21_Central_Discover_Characteristics_Of_Service) {
    peer = BLE.connect(peerAddr, false);
    assertTrue(peer.connected());

    Vector<BleService> services = peer.discoverAllServices();
    BleService ctrlService;
    assertTrue(peer.getServiceByUUID(ctrlService, "6FA90001-5C4E-48A8-94F4-8030546F36FC"));
    BleService customService;
    assertTrue(peer.getServiceByUUID(customService, "6E400000-B5A3-F393-E0A9-E50E24DCCA9E"));

    Vector<BleCharacteristic> characteristicsOfCtrlService = peer.discoverCharacteristicsOfService(ctrlService);
    assertTrue(characteristicsOfCtrlService[0].UUID() == "6FA90002-5C4E-48A8-94F4-8030546F36FC");
    assertTrue(characteristicsOfCtrlService[1].UUID() == "6FA90003-5C4E-48A8-94F4-8030546F36FC");
    assertTrue(characteristicsOfCtrlService[2].UUID() == "6FA90004-5C4E-48A8-94F4-8030546F36FC");

    Vector<BleCharacteristic> characteristicsOfCustomService = peer.discoverCharacteristicsOfService(customService);
    assertTrue(characteristicsOfCustomService[0].UUID() == "6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
    assertTrue(characteristicsOfCustomService[1].UUID() == "6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
    assertTrue(characteristicsOfCustomService[2].UUID() == "6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
    assertTrue(characteristicsOfCustomService[3].UUID() == "6E400004-B5A3-F393-E0A9-E50E24DCCA9E");
    assertTrue(characteristicsOfCustomService[4].UUID() == "6E400005-B5A3-F393-E0A9-E50E24DCCA9E");
    assertTrue(characteristicsOfCustomService[5].UUID() == "6E400006-B5A3-F393-E0A9-E50E24DCCA9E");
    assertTrue(characteristicsOfCustomService[6].UUID() == "6E400007-B5A3-F393-E0A9-E50E24DCCA9E");

    BLE.disconnect(peer);
}

test(BLE_22_Central_Pairing_Sync) {
    // Indicate the peer device to start pairing tests.
    peer = BLE.connect(peerAddr, false);
    assertTrue(peer.connected());

    const String str("deadbeef");
    int ret = peerCharWrite.setValue(str, BleTxRxType::ACK);
    assertTrue(ret == (int)str.length());

    BLE.disconnect(peer);
    delay(2000);
}

static bool pairingRequested = false;
static int pairingStatus = -1;

static void pairingTestRoutine(bool request) {
    peer = BLE.connect(peerAddr, false);
    assertTrue(peer.connected());

    pairingStatus = -1;
    pairingRequested = false;
    BLE.onPairingEvent([&](const BlePairingEvent& event) {
        if (event.type == BlePairingEventType::REQUEST_RECEIVED) {
            // Serial.println("Request received");
            pairingRequested = true;
        } else if (event.type == BlePairingEventType::STATUS_UPDATED) {
            pairingStatus = event.payload.status;
            // Serial.println("status updateed");
        } else if (event.type == BlePairingEventType::PASSKEY_DISPLAY) {
            Serial.print("Please enter the following passkey on the other side: ");
            for (uint8_t i = 0; i < BLE_PAIRING_PASSKEY_LEN; i++) {
                Serial.printf("%c", event.payload.passkey[i]);
            }
            Serial.println("");
        } else if (event.type == BlePairingEventType::PASSKEY_INPUT) {
            Serial.print("Please enter 6-digits passkey displayed on the other side: ");
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

    if (request) {
        assertEqual(BLE.startPairing(peer), (int)SYSTEM_ERROR_NONE);
    } else {
        assertTrue(waitFor([&]{ return pairingRequested; }, 5000));
    }
    assertTrue(BLE.isPairing(peer));
    assertTrue(waitFor([&]{ return !BLE.isPairing(peer); }, 20000));
    assertTrue(BLE.isPaired(peer));
    assertEqual(pairingStatus, (int)SYSTEM_ERROR_NONE);

    delay(500);
    BLE.disconnect(peer);
    delay(500);
}

test(BLE_23_Central_Initiate_Pairing_Central_Io_Caps_None_Peripheral_Io_Caps_None) {
    BLE.setPairingIoCaps(BlePairingIoCaps::NONE);
    pairingTestRoutine(true);
}

test(BLE_24_Central_Pairing_Receiption_Central_Io_Caps_None_Peripheral_Io_Caps_Dispaly_Only) {
    BLE.setPairingIoCaps(BlePairingIoCaps::NONE);
    pairingTestRoutine(false);
}

test(BLE_25_Central_Initiate_Pairing_Central_Io_Caps_None_Peripheral_Io_Caps_Dispaly_Yesno) {
    BLE.setPairingIoCaps(BlePairingIoCaps::NONE);
    pairingTestRoutine(true);
}

test(BLE_26_Central_Pairing_Receiption_Central_Io_Caps_None_Peripheral_Io_Caps_Keyboard_Only) {
    BLE.setPairingIoCaps(BlePairingIoCaps::NONE);
    pairingTestRoutine(false);
}

test(BLE_27_Central_Initiate_Pairing_Central_Io_Caps_None_Peripheral_Io_Caps_Keyboard_Dispaly) {
    BLE.setPairingIoCaps(BlePairingIoCaps::NONE);
    pairingTestRoutine(true);
}

test(BLE_28_Central_Pairing_Receiption_Central_Io_Caps_Dispaly_Only_Peripheral_Io_Caps_None) {
    BLE.setPairingIoCaps(BlePairingIoCaps::DISPLAY_ONLY);
    pairingTestRoutine(false);
}

test(BLE_29_Central_Initiate_Pairing_Central_Io_Caps_Dispaly_Only_Peripheral_Io_Caps_Display_Only) {
    BLE.setPairingIoCaps(BlePairingIoCaps::DISPLAY_ONLY);
    pairingTestRoutine(true);
}

test(BLE_30_Central_Pairing_Receiption_Central_Io_Caps_Dispaly_Only_Peripheral_Io_Caps_Display_Yesno) {
    BLE.setPairingIoCaps(BlePairingIoCaps::DISPLAY_ONLY);
    pairingTestRoutine(false);
}

test(BLE_31_Central_Initiate_Pairing_Central_Io_Caps_Dispaly_Only_Peripheral_Io_Caps_Keyboard_Only) {
    BLE.setPairingIoCaps(BlePairingIoCaps::DISPLAY_ONLY);
    pairingTestRoutine(true);
}

test(BLE_32_Central_Pairing_Receiption_Central_Io_Caps_Dispaly_Only_Peripheral_Io_Caps_Keyboard_Display) {
    BLE.setPairingIoCaps(BlePairingIoCaps::DISPLAY_ONLY);
    pairingTestRoutine(false);
}

test(BLE_33_Central_Initiate_Pairing_Central_Io_Caps_Dispaly_Yesno_Peripheral_Io_Caps_None) {
    BLE.setPairingIoCaps(BlePairingIoCaps::DISPLAY_YESNO);
    pairingTestRoutine(true);
}

test(BLE_34_Central_Pairing_Receiption_Central_Io_Caps_Dispaly_Yesno_Peripheral_Io_Caps_Display_Only) {
    BLE.setPairingIoCaps(BlePairingIoCaps::DISPLAY_YESNO);
    pairingTestRoutine(false);
}

test(BLE_35_Central_Initiate_Pairing_Central_Io_Caps_Dispaly_Yesno_Peripheral_Io_Caps_Display_Yesno) {
    BLE.setPairingIoCaps(BlePairingIoCaps::DISPLAY_YESNO);
    pairingTestRoutine(true);
}

test(BLE_36_Central_Pairing_Receiption_Central_Io_Caps_Dispaly_Yesno_Peripheral_Io_Caps_Keyboard_Only) {
    BLE.setPairingIoCaps(BlePairingIoCaps::DISPLAY_YESNO);
    pairingTestRoutine(false);
}

test(BLE_37_Central_Initiate_Pairing_Central_Io_Caps_Dispaly_Yesno_Peripheral_Io_Caps_Keyboard_Display) {
    BLE.setPairingIoCaps(BlePairingIoCaps::DISPLAY_YESNO);
    pairingTestRoutine(true);
}

test(BLE_38_Central_Pairing_Receiption_Central_Io_Caps_Keyboard_Only_Peripheral_Io_Caps_None) {
    BLE.setPairingIoCaps(BlePairingIoCaps::KEYBOARD_ONLY);
    pairingTestRoutine(false);
}

test(BLE_39_Central_Initiate_Pairing_Central_Io_Caps_Keyboard_Only_Peripheral_Io_Caps_Display_Only) {
    BLE.setPairingIoCaps(BlePairingIoCaps::KEYBOARD_ONLY);
    pairingTestRoutine(true);
}

test(BLE_40_Central_Pairing_Receiption_Central_Io_Caps_Keyboard_Only_Peripheral_Io_Caps_Display_Yesno) {
    BLE.setPairingIoCaps(BlePairingIoCaps::KEYBOARD_ONLY);
    pairingTestRoutine(false);
}

test(BLE_41_Central_Initiate_Pairing_Central_Io_Caps_Keyboard_Only_Peripheral_Io_Caps_Keyboard_Only) {
    BLE.setPairingIoCaps(BlePairingIoCaps::KEYBOARD_ONLY);
    Serial.println("Please enter the same 6-digits key on each side");
    pairingTestRoutine(true);
}

test(BLE_42_Central_Pairing_Receiption_Central_Io_Caps_Keyboard_Only_Peripheral_Io_Caps_Keyboard_Display) {
    BLE.setPairingIoCaps(BlePairingIoCaps::KEYBOARD_ONLY);
    pairingTestRoutine(false);
}

test(BLE_43_Central_Initiate_Pairing_Central_Io_Caps_Keyboard_Display_Peripheral_Io_Caps_None) {
    BLE.setPairingIoCaps(BlePairingIoCaps::KEYBOARD_DISPLAY);
    pairingTestRoutine(true);
}

test(BLE_44_Central_Pairing_Receiption_Central_Io_Caps_Keyboard_Display_Peripheral_Io_Caps_Display_Only) {
    BLE.setPairingIoCaps(BlePairingIoCaps::KEYBOARD_DISPLAY);
    pairingTestRoutine(false);
}

test(BLE_45_Central_Initiate_Pairing_Central_Io_Caps_Keyboard_Display_Peripheral_Io_Caps_Display_Yesno) {
    BLE.setPairingIoCaps(BlePairingIoCaps::KEYBOARD_DISPLAY);
    pairingTestRoutine(true);
}

test(BLE_46_Central_Pairing_Receiption_Central_Io_Caps_Keyboard_Display_Peripheral_Io_Caps_Keyboard_Only) {
    BLE.setPairingIoCaps(BlePairingIoCaps::KEYBOARD_DISPLAY);
    pairingTestRoutine(false);
}

test(BLE_47_Central_Initiate_Pairing_Central_Io_Caps_Keyboard_Display_Peripheral_Io_Caps_Keyboard_Display) {
    BLE.setPairingIoCaps(BlePairingIoCaps::KEYBOARD_DISPLAY);
    pairingTestRoutine(true);
}

test(BLE_48_Central_Initiate_Pairing_Peripheral_Being_Reject) {
    peer = BLE.connect(peerAddr, false);
    assertTrue(peer.connected());

    assertEqual(BLE.startPairing(peer), (int)SYSTEM_ERROR_NONE);
    assertTrue(BLE.isPairing(peer));
    assertTrue(waitFor([&]{ return !BLE.isPairing(peer); }, 20000));
    assertFalse(BLE.isPaired(peer));

    delay(500);
    BLE.disconnect(peer);
    delay(500);
}

test(BLE_49_Central_Pairing_Receiption_Reject) {
    pairingStatus = 0;
    BLE.onPairingEvent([&](const BlePairingEvent& event) {
        if (event.type == BlePairingEventType::REQUEST_RECEIVED) {
            Serial.println("Reject pairing request.");
            BLE.rejectPairing(event.peer);
        } else if (event.type == BlePairingEventType::STATUS_UPDATED) {
            pairingStatus = event.payload.status;
        }
    });

    peer = BLE.connect(peerAddr, false);
    assertTrue(peer.connected());

    assertTrue(waitFor([]{ return pairingStatus != 0; }, 5000));

    delay(500);
    BLE.disconnect(peer);
    delay(500);
}

#endif // #if Wiring_BLE == 1

