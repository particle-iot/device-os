#include "Particle.h"
#include "unit-test/unit-test.h"
#include "util.h"
#include "random.h"

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

using namespace particle::test;

constexpr uint16_t LOCAL_DESIRED_ATT_MTU = 100;
constexpr uint16_t PEER_DESIRED_ATT_MTU = 123;

test(BLE_000_Central_Cloud_Connect) {
    subscribeEvents(BLE_ROLE_PERIPHERAL);
    Particle.connect();
    assertTrue(waitFor(Particle.connected, HAL_PLATFORM_MAX_CLOUD_CONNECT_TIME));
    assertTrue(publishBlePeerInfo());
    assertEqual(BLE.setDesiredAttMtu(LOCAL_DESIRED_ATT_MTU), (int)SYSTEM_ERROR_NONE);
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
}

test(BLE_01_Advertising_Scan_Connect) {
    peerCharNotify.onDataReceived(onDataReceived, &peerCharNotify);
    peerCharIndicate.onDataReceived(onDataReceived, &peerCharIndicate);
    peerCharNotifyAndIndicate.onDataReceived(onDataReceived, &peerCharNotifyAndIndicate);

    int ret = BLE.setScanTimeout(100); // Scan timeout: 1s
    assertEqual(ret, 0);

    Serial.println("BLE starts scanning...");

    size_t wait = 20; // Scanning for 20s.
    while (!BLE.connected() && wait > 0) {
        int count = BLE.scan(results, SCAN_RESULT_COUNT);
        if (count > 0) {
            for (int i = 0; i < count; i++) {
                BleUuid foundServiceUUID;
                size_t svcCount = results[i].advertisingData().serviceUUID(&foundServiceUUID, 1);
#if defined(PARTICLE_TEST_RUNNER)
                if (results[i].address() != BleAddress(getBleTestPeer().address, results[i].address().type())) {
                    continue;
                }
#endif // PARTICLE_TEST_RUNNER
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

test(BLE_02_Connected) {
}

test(BLE_03_Read_Peer_characteristic_With_Read_Property) {
    const char* str = "6dd902629e1d";
    String getStr;
    int ret = peerCharRead.getValue(getStr);
    assertTrue(ret > 0);
    assertTrue(getStr == str);
}

test(BLE_04_Write_Characteristic_With_Write_Property_Auto) {
    const String str("6b4bf92a37f3");
    int ret = peerCharWrite.setValue(str);
    assertTrue(ret == (int)str.length());
}

test(BLE_05_Write_Characteristic_With_Write_Property_Ack) {
    const String str("df3b41caedac");
    int ret = peerCharWrite.setValue(str, BleTxRxType::ACK);
    assertTrue(ret == (int)str.length());
}

test(BLE_06_Write_Characteristic_With_Write_Property_Nack) {
    const String str("febe08cc1f96");
    int ret = peerCharWrite.setValue(str, BleTxRxType::NACK);
    // Fails
    assertTrue(ret < 0);
}

test(BLE_07_Write_Characteristic_With_Write_Wo_Rsp_Property_Auto) {
    const String str("2ad4bffbb8c7");
    int ret = peerCharWriteWoRsp.setValue(str);
    assertTrue(ret == (int)str.length());
}

test(BLE_08_Write_Characteristic_With_Write_Wo_Rsp_Property_Ack) {
    const String str("ad2cb5697c37");
    int ret = peerCharWriteWoRsp.setValue(str, BleTxRxType::ACK);
    // Fails
    assertTrue(ret < 0);
}

test(BLE_09_Write_Characteristic_With_Write_Wo_Rsp_Property_Nack) {
    const String str("203a02992be0");
    int ret = peerCharWriteWoRsp.setValue(str, BleTxRxType::NACK);
    assertTrue(ret == (int)str.length());
}

test(BLE_10_Write_Characteristic_With_Write_Write_Wo_Rsp_Property_Auto) {
    const String str("86d7a840079f");
    int ret = peerCharWriteAndWriteWoRsp.setValue(str);
    assertTrue(ret == (int)str.length());
}

test(BLE_11_Write_Characteristic_With_Write_Write_Wo_Rsp_Property_Ack) {
    const String str("77982c283c65");
    int ret = peerCharWriteAndWriteWoRsp.setValue(str, BleTxRxType::ACK);
    assertTrue(ret == (int)str.length());
}

test(BLE_12_Write_Characteristic_With_Write_Write_Wo_Rsp_Property_Nack) {
    const String str("21ec57d28a0c");
    int ret = peerCharWriteAndWriteWoRsp.setValue(str, BleTxRxType::NACK);
    assertTrue(ret == (int)str.length());
}

test(BLE_13_Received_Characteristic_With_Notify_Property_Auto) {
    assertTrue(waitFor([]{ return str1Rec; }, 2000));
}

test(BLE_14_Received_Characteristic_With_Notify_Property_Nack) {
    assertTrue(waitFor([]{ return str2Rec; }, 2000));
}

test(BLE_15_Received_Characteristic_With_Indicate_Property_Auto) {
    assertTrue(waitFor([]{ return str3Rec; }, 2000));
}

test(BLE_16_Received_Characteristic_With_Indicate_Property_Ack) {
    assertTrue(waitFor([]{ return str4Rec; }, 2000));
}

test(BLE_17_Received_Characteristic_With_Notify_Indicate_Property_Auto) {
    assertTrue(waitFor([]{ return str5Rec; }, 2000));
}

test(BLE_18_Received_Characteristic_With_Notify_Indicate_Property_Ack) {
    assertTrue(waitFor([]{ return str6Rec; }, 2000));
}

test(BLE_19_Received_Characteristic_With_Notify_Indicate_Property_Nack) {
    assertTrue(waitFor([]{ return str7Rec; }, 2000));

    BLE.disconnect(peer);
    int ret = BLE.setScanTimeout(500); // Scan timeout: 5s
    assertEqual(ret, 0);
}


test(BLE_20_Peripheral_Notify_Characteristic_With_Notify_Property_Ack) {
}

test(BLE_21_Peripheral_Notify_Characteristic_With_Indicate_Property_Nack) {
}

test(BLE_22_Discover_All_Services) {
    peer = BLE.connect(peerAddr, false);
    assertTrue(peer.connected());

    Vector<BleService> services = peer.discoverAllServices();
    BleService ctrlService;
    assertTrue(peer.getServiceByUUID(ctrlService, "6FA90001-5C4E-48A8-94F4-8030546F36FC"));
    BleService customService;
    assertTrue(peer.getServiceByUUID(customService, "6E400000-B5A3-F393-E0A9-E50E24DCCA9E"));

    BLE.disconnect(peer);
}

test(BLE_23_Discover_All_Characteristics) {
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

test(BLE_24_Discover_Characteristics_Of_Service) {
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

test(BLE_25_Pairing_Sync) {
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
static bool lesc;
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
    pairingStatus = -1;
    pairingRequested = false;
    BLE.onPairingEvent([&](const BlePairingEvent& event) {
        if (event.type == BlePairingEventType::REQUEST_RECEIVED) {
            Serial.println("Request received");
            pairingRequested = true;
        } else if (event.type == BlePairingEventType::STATUS_UPDATED) {
            pairingStatus = event.payload.status.status;
            lesc = event.payload.status.lesc;
            Serial.printlnf("status updated, %d", event.payload.status.status);
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

    // Some platforms may have to restart BT stack to set IO caps, which may fail the connection attempt
    delay(1s);

    peer = BLE.connect(peerAddr, false);
    assertTrue(peer.connected());
    {
        SCOPE_GUARD ({
            delay(500);
            assertEqual(BLE.disconnect(peer), (int)SYSTEM_ERROR_NONE);
            assertFalse(BLE.connected());
            assertFalse(peer.connected());
            delay(500);
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
        if (algorithm != BlePairingAlgorithm::LEGACY_ONLY) {
            assertTrue(lesc);
        } else {
            assertFalse(lesc);
        }
    }
}

test(BLE_26_Pairing_Algorithm_Auto_Io_Caps) {
    for (uint8_t l = 0; l < 5; l++) { // Local I/O capabilities
        assertEqual(BLE.setPairingIoCaps(pairingIoCaps[l]), (int)SYSTEM_ERROR_NONE);
        for (uint8_t p = 0; p < 5; p++) { // Peer I/O capabilities
            Serial.printlnf("Local I/O Caps: %s, request = %d", ioCapsStr[l], l % 2);
            pairingTestRoutine(l % 2, BlePairingAlgorithm::AUTO, pairingIoCaps[l], pairingIoCaps[p]);
        }
    }
}

test(BLE_27_Pairing_Algorithm_Legacy_Only) {
    assertEqual(BLE.setPairingIoCaps(BlePairingIoCaps::NONE), (int)SYSTEM_ERROR_NONE);
    assertEqual(BLE.setPairingAlgorithm(BlePairingAlgorithm::LEGACY_ONLY), (int)SYSTEM_ERROR_NONE);
    pairingTestRoutine(true, BlePairingAlgorithm::LEGACY_ONLY);
}

test(BLE_28_Pairing_Algorithm_Lesc_Only_Reject_Legacy_Prepare) {
}

test(BLE_29_Pairing_Algorithm_Lesc_Only_Reject_Legacy) {
    assertEqual(BLE.setPairingIoCaps(BlePairingIoCaps::NONE), (int)SYSTEM_ERROR_NONE);
    assertEqual(BLE.setPairingAlgorithm(BlePairingAlgorithm::LESC_ONLY), (int)SYSTEM_ERROR_NONE);

    // Some platforms may have to restart BT stack to set IO caps, which may fail the connection attempt
    delay(1s);

    peer = BLE.connect(peerAddr, false);
    assertTrue(peer.connected());
    {
        SCOPE_GUARD ({
            delay(500);
            assertEqual(BLE.disconnect(peer), (int)SYSTEM_ERROR_NONE);
            assertFalse(BLE.connected());
            delay(500);
        });

        pairingStatus = -1;
        BLE.onPairingEvent([&](const BlePairingEvent& event) {
            if (event.type == BlePairingEventType::STATUS_UPDATED) {
                pairingStatus = event.payload.status.status;
                // Serial.println("status updated");
            }
        });

        assertEqual(BLE.startPairing(peer), (int)SYSTEM_ERROR_NONE);
        assertTrue(BLE.isPairing(peer));
        assertTrue(waitFor([&]{ return !BLE.isPairing(peer); }, 20000));
        assertFalse(BLE.isPaired(peer));
        assertNotEqual(pairingStatus, (int)SYSTEM_ERROR_NONE);
    }
}

test(BLE_30_Initiate_Pairing_Being_Rejected) {
    peer = BLE.connect(peerAddr, false);
    assertTrue(peer.connected());
    {
        SCOPE_GUARD ({
            delay(500);
            assertEqual(BLE.disconnect(peer), (int)SYSTEM_ERROR_NONE);
            assertFalse(BLE.connected());
            delay(500);
        });

        assertEqual(BLE.startPairing(peer), (int)SYSTEM_ERROR_NONE);
        assertTrue(BLE.isPairing(peer));
        assertTrue(waitFor([&]{ return !BLE.isPairing(peer); }, 20000));
        assertFalse(BLE.isPaired(peer));
    }
}

test(BLE_31_Pairing_Receiption_Reject) {
    pairingStatus = 0;
    BLE.onPairingEvent([&](const BlePairingEvent& event) {
        if (event.type == BlePairingEventType::REQUEST_RECEIVED) {
            Serial.println("Reject pairing request.");
            BLE.rejectPairing(event.peer);
        } else if (event.type == BlePairingEventType::STATUS_UPDATED) {
            pairingStatus = event.payload.status.status;
        }
    });

#if HAL_PLATFORM_RTL872X
    // We set it to LESC_ONLY before, it will reject the pairing request automatically without generating any event.
    assertEqual(BLE.setPairingAlgorithm(BlePairingAlgorithm::AUTO), (int)SYSTEM_ERROR_NONE);
#endif

    peer = BLE.connect(peerAddr, false);
    assertTrue(peer.connected());
    {
        SCOPE_GUARD ({
            delay(500);
            assertEqual(BLE.disconnect(peer), (int)SYSTEM_ERROR_NONE);
            assertFalse(BLE.connected());
            delay(500);
        });

        assertTrue(waitFor([]{ return pairingStatus != 0; }, 5000));
    }
}

test(BLE_32_Att_Mtu_Exchange) {
    BLE.onAttMtuExchanged([&](const BlePeerDevice& p, size_t attMtu) {
        effectiveAttMtu = attMtu;
    });

    SCOPE_GUARD ({
        BLE.onAttMtuExchanged(nullptr, nullptr);
    });

    peer = BLE.connect(peerAddr, false);
    assertTrue(peer.connected());
    {
        SCOPE_GUARD ({
            delay(500);
            assertEqual(BLE.disconnect(peer), (int)SYSTEM_ERROR_NONE);
            assertFalse(BLE.connected());
            delay(500);
        });

        assertTrue(waitFor([&]{ return effectiveAttMtu == LOCAL_DESIRED_ATT_MTU; }, 5000));
    }
}

#endif // #if Wiring_BLE == 1

