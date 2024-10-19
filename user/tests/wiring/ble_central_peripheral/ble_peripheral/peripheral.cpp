#include "Particle.h"
#include "unit-test/unit-test.h"
#include "util.h"
#include "random.h"

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

using namespace particle::test;

constexpr uint16_t LOCAL_DESIRED_ATT_MTU = 123;
constexpr uint16_t PEER_DESIRED_ATT_MTU = 100;

Thread* scanThread = nullptr;
volatile unsigned scanResults = 0;

test(BLE_0000_Check_Feature_Disable_Listening_Mode) {
    // System.enableFeature(FEATURE_DISABLE_LISTENING_MODE);
    if (System.featureEnabled(FEATURE_DISABLE_LISTENING_MODE)) {
        System.disableFeature(FEATURE_DISABLE_LISTENING_MODE);
        assertEqual(0, pushMailbox(MailboxEntry().type(MailboxEntry::Type::RESET_PENDING), 5000));
        System.reset();
    }
}

test(BLE_000_Peripheral_Cloud_Connect) {
    assertFalse(System.featureEnabled(FEATURE_DISABLE_LISTENING_MODE));
    subscribeEvents(BLE_ROLE_PERIPHERAL);
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertTrue(publishBlePeerInfo());
}

test(BLE_00_Prepare) {
#ifndef PARTICLE_TEST_RUNNER
    for (int i = 0; i < 60; i++) {
        assertTrue(publishBlePeerInfo());
        if (getBleTestPeer().isValid()) {
            break;
        }
        delay(1000);
    }
    assertTrue(getBleTestPeer().isValid());
#else
    assertTrue(waitFor(getBleTestPeer().isValid, 60 * 1000));
#endif // PARTICLE_TEST_RUNNER
    assertEqual(BLE.setDesiredAttMtu(LOCAL_DESIRED_ATT_MTU), (int)SYSTEM_ERROR_NONE);
}

test(BLE_01_Advertising_Scan_Connect) {
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

    BLE.onConnected([](const BlePeerDevice& peer) {
        Serial.println("Connected.");
    });
    BLE.onDisconnected([](const BlePeerDevice& peer) {
        Serial.println("Disconnected.");
    });

    Serial.println("BLE starts advertising...");

    assertTrue(BLE.advertising());
}

test(BLE_02_Connected) {
    assertTrue(waitFor(BLE.connected, 20000));
}

test(BLE_03_Read_Peer_characteristic_With_Read_Property) {
}

// For the first data transmission, we need to wait longer to make sure
// the Central device has discovered the services and characteristics.
test(BLE_04_Write_Characteristic_With_Write_Property_Auto) {
    assertTrue(waitFor([]{ return str1Rec; }, 10000));
}

test(BLE_05_Write_Characteristic_With_Write_Property_Ack) {
    assertTrue(waitFor([]{ return str2Rec; }, 2000));
}

test(BLE_06_Write_Characteristic_With_Write_Property_Nack) {
}

test(BLE_07_Write_Characteristic_With_Write_Wo_Rsp_Property_Auto) {
    assertTrue(waitFor([]{ return str3Rec; }, 2000));
}

test(BLE_08_Write_Characteristic_With_Write_Wo_Rsp_Property_Ack) {
}

test(BLE_09_Write_Characteristic_With_Write_Wo_Rsp_Property_Nack) {
    assertTrue(waitFor([]{ return str4Rec; }, 2000));
}

test(BLE_10_Write_Characteristic_With_Write_Write_Wo_Rsp_Property_Auto) {
    assertTrue(waitFor([]{ return str5Rec; }, 2000));
}

test(BLE_11_Write_Characteristic_With_Write_Write_Wo_Rsp_Property_Ack) {
    assertTrue(waitFor([]{ return str6Rec; }, 2000));
}

test(BLE_12_Write_Characteristic_With_Write_Write_Wo_Rsp_Property_Nack) {
    assertTrue(waitFor([]{ return str7Rec; }, 2000));
}

test(BLE_13_Received_Characteristic_With_Notify_Property_Auto) {
    const String str("05a9ae9588dd");
    int ret = charNotify.setValue(str);
    assertTrue(ret == (int)str.length());
}

test(BLE_14_Received_Characteristic_With_Notify_Property_Nack) {
    const String str("4f62bd40046a");
    int ret = charNotify.setValue(str, BleTxRxType::NACK);
    assertTrue(ret == (int)str.length());
}

test(BLE_15_Received_Characteristic_With_Indicate_Property_Auto) {
    const String str("6619918032a2");
    int ret = charIndicate.setValue(str);
    assertTrue(ret == (int)str.length());
}

test(BLE_16_Received_Characteristic_With_Indicate_Property_Ack) {
    const String str("359b4deb3f37");
    int ret = charIndicate.setValue(str, BleTxRxType::ACK);
    assertTrue(ret == (int)str.length());
}

test(BLE_17_Received_Characteristic_With_Notify_Indicate_Property_Auto) {
    const String str("a2ef18ec6eaa");
    int ret = charNotifyAndIndicate.setValue(str);
    assertTrue(ret == (int)str.length());
}

test(BLE_18_Received_Characteristic_With_Notify_Indicate_Property_Ack) {
    const String str("7223b5dd3342");
    int ret = charNotifyAndIndicate.setValue(str, BleTxRxType::ACK);
    assertTrue(ret == (int)str.length());
}

test(BLE_19_Received_Characteristic_With_Notify_Indicate_Property_Nack) {
    const String str("d4b4249bbbe3");
    int ret = charNotifyAndIndicate.setValue(str, BleTxRxType::NACK);
    assertTrue(ret == (int)str.length());
}

test(BLE_20_Peripheral_Notify_Characteristic_With_Notify_Property_Ack) {
    const String str("b551f6ca1329");
    int ret = charNotify.setValue(str, BleTxRxType::ACK);
    // Fail
    assertTrue(ret < 0);
}

test(BLE_21_Peripheral_Notify_Characteristic_With_Indicate_Property_Nack) {
    const String str("06d0be9e185b");
    int ret = charIndicate.setValue(str, BleTxRxType::NACK);
    // Fail
    assertTrue(ret < 0);
}

test(BLE_22_Discover_All_Services) {
}

test(BLE_23_Discover_All_Characteristics) {
}

test(BLE_24_Discover_Characteristics_Of_Service) {
}

test(BLE_25_Pairing_Sync) {
    // The central will perform some service and characteristic discovery tests
    // before starting the pairing tests
    assertTrue(waitFor([]{ return pairingStarted; }, 20000));
    assertTrue(waitFor([]{ return !BLE.connected(); }, 5000));
}

static bool pairingRequested = false;
static int pairingStatus = -1;
static bool lesc;
static BlePeerDevice peer;
static size_t effectiveAttMtu = 0;

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

static void pairingTestRoutine(bool request, BlePairingAlgorithm algorithm,
        BlePairingIoCaps ours = BlePairingIoCaps::NONE, BlePairingIoCaps theirs = BlePairingIoCaps::NONE) {
    pairingRequested = false;
    pairingStatus = -1;

    BLE.onPairingEvent([&](const BlePairingEvent& event) {
        if (event.type == BlePairingEventType::REQUEST_RECEIVED) {
            pairingRequested = true;
            peer = event.peer;
            Serial.println("Request received");
        } else if (event.type == BlePairingEventType::STATUS_UPDATED) {
            pairingStatus = event.payload.status.status;
            lesc = event.payload.status.lesc;
            Serial.println("status updated");
        } else if (event.type == BlePairingEventType::PASSKEY_DISPLAY || event.type == BlePairingEventType::NUMERIC_COMPARISON) {
            Serial.print("Passkey display: ");
            for (uint8_t i = 0; i < BLE_PAIRING_PASSKEY_LEN; i++) {
                Serial.printf("%c", event.payload.passkey[i]);
            }
            Serial.println("");
#if !defined(PARTICLE_TEST_RUNNER)
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
#else
            Serial.printlnf("publish passkey");
            assertTrue(publishPassKey((const char*)event.payload.passkey, event.payloadLen));
            if (event.type == BlePairingEventType::NUMERIC_COMPARISON) {
                String peerPassKey;
                for (auto t = millis(); millis() - t < 60000;) {
                    if (getBleTestPasskey(peerPassKey)) {
                        break;
                    }
                    delay(50);
                }
                assertNotEqual(String(), peerPassKey);
                BLE.setPairingNumericComparison(event.peer, peerPassKey == String((const char*)event.payload.passkey, event.payloadLen));
            }
#endif // PARTICLE_TEST_RUNNER
        } else if (event.type == BlePairingEventType::PASSKEY_INPUT) {
#if !defined(PARTICLE_TEST_RUNNER)
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
#else
            Serial.printlnf("passkey input");
            String peerPassKey;
            // Special case where both ends have KEYBOARD_ONLY capabilities:
            // Generate random passkey, the side that is the requester will publish it
            if (ours == theirs && ours == BlePairingIoCaps::KEYBOARD_ONLY && request) {
                Random rand;
                char passkey[BLE_PAIRING_PASSKEY_LEN + 1] = {};
                const char dict[] = "0123456789";
                rand.genAlpha(passkey, BLE_PAIRING_PASSKEY_LEN, dict, sizeof(dict) - 1);
                assertTrue(publishPassKey(passkey, BLE_PAIRING_PASSKEY_LEN));
                peerPassKey = String(passkey);
            } else {
                for (auto t = millis(); millis() - t < 60000;) {
                    if (getBleTestPasskey(peerPassKey)) {
                        break;
                    }
                    delay(50);
                }
            }
            assertNotEqual(String(), peerPassKey);
            BLE.setPairingPasskey(event.peer, (const uint8_t*)peerPassKey.c_str());
#endif // PARTICLE_TEST_RUNNER
        }
    });
    assertTrue(waitFor(BLE.connected, 20000));
    {
        SCOPE_GUARD ({
            Particle.publish("BLE disconnect", nullptr, WITH_ACK);
            assertTrue(waitFor([]{ return !BLE.connected(); }, 60000));
            assertFalse(BLE.connected());
        });

        if (request) {
            peer = BLE.peerCentral();
            Serial.println("Start pairing...");
            assertEqual(BLE.startPairing(peer), (int)SYSTEM_ERROR_NONE);
        } else {
            assertTrue(waitFor([&]{ return pairingRequested; }, 20000));
        }
        // It may be paired in real quick if pairing uses JUST_WORK
        if (BLE.isPairing(peer)) {
            assertTrue(waitFor([&]{ return !BLE.isPairing(peer); }, 60000));
        }
        assertTrue(BLE.isPaired(peer));
        assertEqual(pairingStatus, (int)SYSTEM_ERROR_NONE);
        if (algorithm != BlePairingAlgorithm::LEGACY_ONLY) {
            assertTrue(lesc);
        } else {
            assertFalse(lesc);
        }
    }
}

test(BLE_26_Pairing_Algorithm_Auto_Io_Caps) {
    for (uint8_t p = 0; p < 5; p++) { // Peer I/O capabilities
        for (uint8_t l = 0; l < 5; l++) {
            assertEqual(BLE.setPairingIoCaps(pairingIoCaps[l]), (int)SYSTEM_ERROR_NONE); // Local I/O capabilities
            Serial.printlnf("Local I/O Caps: %s, request = %d", ioCapsStr[l], !(p % 2));
            pairingTestRoutine(!(p % 2), BlePairingAlgorithm::AUTO, pairingIoCaps[l], pairingIoCaps[p]);
        }
    }
}

test(BLE_27_Pairing_Algorithm_Legacy_Only) {
    assertEqual(BLE.setPairingIoCaps(BlePairingIoCaps::NONE), (int)SYSTEM_ERROR_NONE);
    assertEqual(BLE.setPairingAlgorithm(BlePairingAlgorithm::LEGACY_ONLY), (int)SYSTEM_ERROR_NONE);
    pairingTestRoutine(false, BlePairingAlgorithm::LEGACY_ONLY);
}

test(BLE_28_Pairing_Algorithm_Lesc_Only_Reject_Legacy_Prepare) {
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

}

test(BLE_29_Pairing_Algorithm_Lesc_Only_Reject_Legacy) {
    assertTrue(waitFor(BLE.connected, 20000));
    {
        SCOPE_GUARD ({
            assertTrue(waitFor([]{ return !BLE.connected(); }, 5000));
            assertFalse(BLE.connected());
        });

        assertTrue(waitFor([&]{ return pairingRequested; }, 20000));
        assertTrue(BLE.isPairing(peer));
        assertTrue(waitFor([&]{ return !BLE.isPairing(peer); }, 20000));
        assertFalse(BLE.isPaired(peer));
    }
}

test(BLE_30_Initiate_Pairing_Being_Rejected) {
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
    {
        SCOPE_GUARD ({
            assertTrue(waitFor([]{ return !BLE.connected(); }, 5000));
            assertFalse(BLE.connected());
        });

        assertTrue(waitFor([]{ return pairingStatus != 0; }, 5000));
    }
}

test(BLE_31_Pairing_Receiption_Reject) {
    assertTrue(waitFor(BLE.connected, 20000));
    {
        SCOPE_GUARD ({
            assertTrue(waitFor([]{ return !BLE.connected(); }, 5000));
            assertFalse(BLE.connected());
        });

        peer = BLE.peerCentral();
        assertEqual(BLE.startPairing(peer), (int)SYSTEM_ERROR_NONE);

        assertTrue(BLE.isPairing(peer));
        assertTrue(waitFor([&]{ return !BLE.isPairing(peer); }, 20000));
        assertFalse(BLE.isPaired(peer));
    }
}

test(BLE_32_Att_Mtu_Exchange) {
    BLE.onAttMtuExchanged([&](const BlePeerDevice& p, size_t attMtu) {
        effectiveAttMtu = attMtu;
        Serial.printlnf("ATT MTU: %d", attMtu);
    });

    SCOPE_GUARD ({
        BLE.onAttMtuExchanged(nullptr, nullptr);
    });

    assertTrue(waitFor(BLE.connected, 20000));
    {
        SCOPE_GUARD ({
            assertTrue(waitFor([]{ return !BLE.connected(); }, 5000));
            assertFalse(BLE.connected());
        });
        assertTrue(waitFor([]{ return effectiveAttMtu == PEER_DESIRED_ATT_MTU; }, 5000));
    }
}

test(BLE_33_Central_Can_Connect_While_Scanning) {
    assertTrue(waitFor(BLE.connected, 20000));
    SCOPE_GUARD ({
        assertTrue(waitFor([]{ return !BLE.connected(); }, 10000));
        assertFalse(BLE.connected());
    });
    BleAdvertisingData advData;
    advData.appendServiceUUID(serviceUuid);
    BleAdvertisingData srData;
    srData.appendLocalName("ble-test");
    int ret = BLE.advertise(&advData, &srData);
    assertEqual(ret, 0);
    assertTrue(BLE.advertising());
}

test(BLE_34_Central_Is_Still_Scanning_After_Disconnect) {
}

test(BLE_35_Central_Can_Connect_While_Peripheral_Is_Scanning_Prepare) {
    BleScanParams params = {};
    params.size = sizeof(BleScanParams);
    params.timeout = 0;
    params.interval = 800; // *0.625ms = 500ms
    params.window = 800; // *0.625 = 500ms
    params.active = true; // Send scan request
    params.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
    assertEqual(0, BLE.setScanParameters(&params));

    scanThread = new Thread("test", [](void* param) -> os_thread_return_t {
        BLE.scanWithFilter(BleScanFilter().allowDuplicates(true), +[](const BleScanResult *result, void *context) -> void {
            scanResults++;
        }, nullptr);
    }, nullptr);
    assertTrue((bool)scanThread);
    assertTrue(waitFor(BLE.scanning, 500));
}

test(BLE_36_Central_Can_Connect_While_Peripheral_Is_Scanning) {
    SCOPE_GUARD({
        if (BLE.scanning() && scanThread) {
            BLE.stopScanning();
            scanThread->join();
            delete scanThread;
            scanThread = nullptr;
        }
    });
    assertTrue(BLE.scanning());
    assertTrue(waitFor(BLE.connected, 60000));
    scanResults = 0;
    delay(2000);
    assertMoreOrEqual((unsigned)scanResults, 1);
    SCOPE_GUARD ({
        assertTrue(waitFor([]{ return !BLE.connected(); }, 10000));
        assertFalse(BLE.connected());
        
        scanResults = 0;
        delay(2000);
        assertMoreOrEqual((unsigned)scanResults, 1);

        assertEqual(0, BLE.stopScanning());
        assertFalse(BLE.scanning());
        scanThread->join();
        delete scanThread;
        scanThread = nullptr;
    });
}

test(BLE_37_Central_Can_Connect_While_Peripheral_Is_Scanning_And_Stops_Scanning_Prepare) {
    BleScanParams params = {};
    params.size = sizeof(BleScanParams);
    params.timeout = 0;
    params.interval = 800; // *0.625ms = 500ms
    params.window = 800; // *0.625 = 500ms
    params.active = true; // Send scan request
    params.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
    assertEqual(0, BLE.setScanParameters(&params));

    scanThread = new Thread("test", [](void* param) -> os_thread_return_t {
        BLE.scanWithFilter(BleScanFilter().allowDuplicates(true), +[](const BleScanResult *result, void *context) -> void {
            scanResults++;
        }, nullptr);
    }, nullptr);
    assertTrue((bool)scanThread);
    waitFor([]{ return BLE.scanning(); }, 5000);
}

test(BLE_38_Central_Can_Connect_While_Peripheral_Is_Scanning_And_Stops_Scanning) {
    SCOPE_GUARD({
        if (BLE.scanning() && scanThread) {
            BLE.stopScanning();
            scanThread->join();
            delete scanThread;
            scanThread = nullptr;
        }
    });
    assertTrue(BLE.scanning());
    assertTrue(waitFor(BLE.connected, 60000));
    scanResults = 0;
    delay(2000);
    assertMoreOrEqual((unsigned)scanResults, 1);

    assertEqual(0, BLE.stopScanning());
    assertFalse(BLE.scanning());
    scanThread->join();
    delete scanThread;
    scanThread = nullptr;

    assertTrue(BLE.connected());

    SCOPE_GUARD ({
        assertTrue(waitFor([]{ return !BLE.connected(); }, 10000));
        assertFalse(BLE.connected());
    });
}

test(BLE_39_Advertising_Scheme_Auto_Adv_After_Disconnect) {
    BleAdvertisingScheme defaultScheme;
    BLE.getAdvertisingScheme(&defaultScheme);
    assertEqual((int)defaultScheme, (int)BleAdvertisingScheme::AUTO_ADV_ALWAYS);

    SCOPE_GUARD ({
        assertTrue(waitFor([]{ return !BLE.connected(); }, 10000));
        assertFalse(BLE.connected());
        // Stop advertising, otherwise, test BLE_40 will enter the connected-advertising state
        BLE.stopAdvertising();
        assertFalse(BLE.advertising());
    });

    BLE.setAdvertisingScheme(BleAdvertisingScheme::AUTO_ADV_ALWAYS);
    BLE.advertise();
    assertTrue(waitFor(BLE.connected, 60000));
    assertTrue(waitFor([]{ return !BLE.connected(); }, 10000));
    assertFalse(BLE.connected());
    delay(1000);
    assertTrue(BLE.advertising());
}

test(BLE_40_Advertising_Scheme_Stop_Adv_After_Current_Connection_Disconnect) {
    SCOPE_GUARD ({
        assertTrue(waitFor([]{ return !BLE.connected(); }, 10000));
        assertFalse(BLE.connected());
        // Stop advertising, otherwise, test BLE_41 will enter the connected-advertising state
        BLE.stopAdvertising();
        assertFalse(BLE.advertising());
    });

    for (uint8_t i = 0; i < 3; i++) { // try 3 times
        BLE.advertise();
        assertTrue(waitFor(BLE.connected, 60000));
        BLE.setAdvertisingScheme(BleAdvertisingScheme::AUTO_ADV_SINCE_NEXT_CONN);
        BleAdvertisingScheme scheme;
        BLE.getAdvertisingScheme(&scheme);
        assertEqual((int)scheme, (int)BleAdvertisingScheme::AUTO_ADV_SINCE_NEXT_CONN);
        assertTrue(waitFor([]{ return !BLE.connected(); }, 10000));
        assertFalse(BLE.connected());
        delay(1000);
        assertFalse(BLE.advertising());
    }

    // Without calling BLE.setAdvertisingScheme(BleAdvertisingScheme::AUTO_ADV_SINCE_NEXT_CONN),
    // it should be AUTO_ADV_ALWAYS
    BLE.advertise();
    assertTrue(waitFor(BLE.connected, 60000));
    BleAdvertisingScheme scheme;
    BLE.getAdvertisingScheme(&scheme);
    assertEqual((int)scheme, (int)BleAdvertisingScheme::AUTO_ADV_ALWAYS);
    assertTrue(waitFor([]{ return !BLE.connected(); }, 10000));
    assertFalse(BLE.connected());
    delay(1000);
    assertTrue(BLE.advertising());
}

test(BLE_41_Advertising_Scheme_Stop_Adv_After_Disconnect) {
    SCOPE_GUARD ({
        assertTrue(waitFor([]{ return !BLE.connected(); }, 10000));
        assertFalse(BLE.connected());
    });

    BLE.setAdvertisingScheme(BleAdvertisingScheme::AUTO_ADV_FORBIDDEN);
    for (uint8_t i = 0; i < 4; i++) {
        BLE.advertise();
        BleAdvertisingScheme scheme;
        BLE.getAdvertisingScheme(&scheme);
        assertEqual((int)scheme, (int)BleAdvertisingScheme::AUTO_ADV_FORBIDDEN);
        assertTrue(waitFor(BLE.connected, 60000));
        assertTrue(waitFor([]{ return !BLE.connected(); }, 10000));
        assertFalse(BLE.connected());
        delay(1000);
        assertFalse(BLE.advertising());
    }
}

test(BLE_42_Advertising_Scheme_Ignored_After_Disconnect_If_Advertising_While_Connected) {
    SCOPE_GUARD ({
        assertTrue(waitFor([]{ return !BLE.connected(); }, 10000));
        assertFalse(BLE.connected());
    });

    BLE.setAdvertisingScheme(BleAdvertisingScheme::AUTO_ADV_FORBIDDEN);
    BleAdvertisingScheme scheme;
    BLE.getAdvertisingScheme(&scheme);
    assertEqual((int)scheme, (int)BleAdvertisingScheme::AUTO_ADV_FORBIDDEN);

    BLE.advertise();
    assertTrue(waitFor(BLE.connected, 60000));
    // Enter the connected-advertising state
    BLE.advertise();

    assertTrue(waitFor([]{ return !BLE.connected(); }, 10000));
    assertFalse(BLE.connected());
    assertTrue(BLE.advertising());
}


#endif // #if Wiring_BLE == 1

