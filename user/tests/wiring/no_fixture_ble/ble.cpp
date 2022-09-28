/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "Particle.h"
#include "unit-test/unit-test.h"

#if Wiring_BLE == 1

test(BLE_01_Set_BLE_Device_Address) {
    int ret;
    BleAddress defaultAddr = BLE.address();
    BleAddress getAddr;

#if !HAL_PLATFORM_RTL872X // P2 doesn't support setting public device address
    // The most two significant bits should be  0b10.
    uint8_t mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x9b};
    ret = BLE.setAddress(BleAddress(mac));
    assertEqual(ret, 0);
    getAddr = BLE.address();
    assertTrue(!strcmp(getAddr.toString().c_str(), "9B:44:33:22:11:00"));

    // The address in string is big-endian
    ret = BLE.setAddress("9b:44:33:22:11:00");
    assertEqual(ret, 0);
    getAddr = BLE.address();
    assertTrue(!strcmp(getAddr.toString().c_str(), "9B:44:33:22:11:00"));

    ret = BLE.setAddress("9b44:33:22/11:00");
    assertEqual(ret, 0);
    getAddr = BLE.address();
    assertTrue(!strcmp(getAddr.toString().c_str(), "9B:44:33:22:11:00"));

    ret = BLE.setAddress("9b44:33:22/x;11");
    assertEqual(ret, 0);
    getAddr = BLE.address();
    assertTrue(!strcmp(getAddr.toString().c_str(), "9B:44:33:22:11:00"));

    ret = BLE.setAddress("9b44:33:22/11:00:43gd34");
    assertEqual(ret, 0);
    getAddr = BLE.address();
    assertTrue(!strcmp(getAddr.toString().c_str(), "9B:44:33:22:11:00"));

    ret = BLE.setAddress("9b4k:33:2,/11:00:43gd34");
    assertEqual(ret, 0);
    getAddr = BLE.address();
    assertTrue(!strcmp(getAddr.toString().c_str(), "9B:40:33:20:11:00"));
#endif // !HAL_PLATFORM_RTL872X

    // The most two significant bits of static address must be 0b11
    ret = BLE.setAddress("9b:44:33:22:11:00", BleAddressType::RANDOM_STATIC);
    assertNotEqual(ret, 0);

    ret = BLE.setAddress("c5:44:33:22:11:00", BleAddressType::RANDOM_STATIC);
    assertEqual(ret, 0);
    getAddr = BLE.address();
    assertTrue(!strcmp(getAddr.toString().c_str(), "C5:44:33:22:11:00"));

    // Restore default device address
    ret = BLE.setAddress(nullptr);
    assertEqual(ret, 0);
    getAddr = BLE.address();
    assertTrue(getAddr == defaultAddr);
}

test(BLE_02_Set_BLE_Device_Name) {
    int ret;
    String defaultName = BLE.getDeviceName();
    String getName;

    // Fetched device name should be null-terminated.
    char buf[BLE_MAX_DEV_NAME_LEN + 1];
    size_t len = BLE.getDeviceName(buf, sizeof(buf));
    assertTrue(buf[len] == '\0');

    ret = BLE.setDeviceName("Argon-test1");
    assertEqual(ret, 0);
    getName = BLE.getDeviceName();
    assertTrue(getName == "Argon-test1");

    ret = BLE.setDeviceName("Argon-test01234567890123456789");
    assertEqual(ret, 0);
    getName = BLE.getDeviceName();
    assertTrue(getName == "Argon-test0123456789");

    ret = BLE.setDeviceName("Argon-test0123456789012", 5);
    assertEqual(ret, 0);
    getName = BLE.getDeviceName();
    assertTrue(getName == "Argon");

    ret = BLE.setDeviceName("Argon-test2", BLE_MAX_DEV_NAME_LEN);
    assertEqual(ret, 0);
    getName = BLE.getDeviceName();
    assertTrue(getName == "Argon-test2");

    ret = BLE.setDeviceName(String("Argon-test3"));
    assertEqual(ret, 0);
    getName = BLE.getDeviceName();
    assertTrue(getName == "Argon-test3");

    // Restore default device name
    ret = BLE.setDeviceName(nullptr);
    assertEqual(ret, 0);
    getName = BLE.getDeviceName();
    assertTrue(getName == defaultName);
}

#if !HAL_PLATFORM_RTL872X // P2 doesn't support setting TX power
test(BLE_03_Set_BLE_Tx_Power) {
    int ret;
    int8_t getTxPower;

    // Valid TX power: -20, -16, -12, -8, -4, 0, 4, 8
    // Other values are right-rounded, except the value larger than 8.
    ret = BLE.setTxPower(-25);
    assertEqual(ret, 0);
    ret = BLE.txPower(&getTxPower);
    assertEqual(ret, 0);
    assertEqual(getTxPower, -20);

    ret = BLE.setTxPower(-18);
    assertEqual(ret, 0);
    ret = BLE.txPower(&getTxPower);
    assertEqual(ret, 0);
    assertEqual(getTxPower, -16);

    ret = BLE.setTxPower(-14);
    assertEqual(ret, 0);
    ret = BLE.txPower(&getTxPower);
    assertEqual(ret, 0);
    assertEqual(getTxPower, -12);

    ret = BLE.setTxPower(-10);
    assertEqual(ret, 0);
    ret = BLE.txPower(&getTxPower);
    assertEqual(ret, 0);
    assertEqual(getTxPower, -8);

    ret = BLE.setTxPower(-6);
    assertEqual(ret, 0);
    ret = BLE.txPower(&getTxPower);
    assertEqual(ret, 0);
    assertEqual(getTxPower, -4);

    ret = BLE.setTxPower(-2);
    assertEqual(ret, 0);
    ret = BLE.txPower(&getTxPower);
    assertEqual(ret, 0);
    assertEqual(getTxPower, 0);

    ret = BLE.setTxPower(2);
    assertEqual(ret, 0);
    ret = BLE.txPower(&getTxPower);
    assertEqual(ret, 0);
    assertEqual(getTxPower, 4);

    ret = BLE.setTxPower(6);
    assertEqual(ret, 0);
    ret = BLE.txPower(&getTxPower);
    assertEqual(ret, 0);
    assertEqual(getTxPower, 8);

    ret = BLE.setTxPower(9);
    assertEqual(ret, 0);
    ret = BLE.txPower(&getTxPower);
    assertEqual(ret, 0);
    assertEqual(getTxPower, 8);
}
#endif // !HAL_PLATFORM_RTL872X

test(BLE_04_Select_BLE_Antenna) {
#if HAL_PLATFORM_RADIO_ANTENNA_EXTERNAL
    int ret = BLE.selectAntenna(BleAntennaType::EXTERNAL);
    assertEqual(ret, 0);
#endif // HAL_PLATFORM_RADIO_ANTENNA_EXTERNAL
#if HAL_PLATFORM_RADIO_ANTENNA_INTERNAL
    ret = BLE.selectAntenna(BleAntennaType::INTERNAL);
    assertEqual(ret, 0);
#endif // HAL_PLATFORM_RADIO_ANTENNA_INTERNAL
}

test(BLE_05_Set_BLE_Advertising_Parameters) {
    int ret;
    BleAdvertisingParams defaultAdvParams = {};
    BleAdvertisingParams getAdvParams = {};

    defaultAdvParams.size = sizeof(BleAdvertisingParams);
    ret = BLE.getAdvertisingParameters(&defaultAdvParams);
    assertEqual(ret, 0);

    ret = BLE.setAdvertisingInterval(50); // In units of 0.625ms
    assertEqual(ret, 0);
    ret = BLE.setAdvertisingTimeout(1000); // In units of 10ms
    assertEqual(ret, 0);
    ret = BLE.setAdvertisingType(BleAdvertisingEventType::SCANABLE_UNDIRECTED);
    assertEqual(ret, 0);
    getAdvParams.size = sizeof(BleAdvertisingParams);
    ret = BLE.getAdvertisingParameters(&getAdvParams);
    assertEqual(ret, 0);
    assertEqual(getAdvParams.interval, 50);
    assertEqual(getAdvParams.timeout, 1000);
    assertTrue(getAdvParams.type == (uint8_t)BleAdvertisingEventType::SCANABLE_UNDIRECTED);

    ret = BLE.setAdvertisingParameters(150, 2000, BleAdvertisingEventType::CONNECTABLE_UNDIRECTED);
    assertEqual(ret, 0);
    ret = BLE.getAdvertisingParameters(&getAdvParams);
    assertEqual(ret, 0);
    assertEqual(getAdvParams.interval, 150);
    assertEqual(getAdvParams.timeout, 2000);
    assertTrue(getAdvParams.type == (uint8_t)BleAdvertisingEventType::CONNECTABLE_UNDIRECTED);

    BleAdvertisingParams setAdvParams = {};
    setAdvParams.size = sizeof(BleAdvertisingParams);
    setAdvParams.interval = 200;
    setAdvParams.timeout = 3000;
    setAdvParams.type = (hal_ble_adv_evt_type_t)BleAdvertisingEventType::NON_CONNECTABLE_NON_SCANABLE_UNDIRECTED;
    ret = BLE.setAdvertisingParameters(&setAdvParams);
    assertEqual(ret, 0);
    ret = BLE.getAdvertisingParameters(&getAdvParams);
    assertEqual(ret, 0);
    assertEqual(getAdvParams.interval, 200);
    assertEqual(getAdvParams.timeout, 3000);
    assertTrue(getAdvParams.type == (uint8_t)BleAdvertisingEventType::NON_CONNECTABLE_NON_SCANABLE_UNDIRECTED);

    // Restore default advertising data
    ret = BLE.setAdvertisingParameters(nullptr);
    assertEqual(ret, 0);
    ret = BLE.getAdvertisingParameters(&getAdvParams);
    assertEqual(ret, 0);
    assertEqual(getAdvParams.interval, defaultAdvParams.interval);
    assertEqual(getAdvParams.timeout, defaultAdvParams.timeout);
    assertTrue(getAdvParams.type == defaultAdvParams.type);
}

test(BLE_06_Set_BLE_Advertising_Data) {
    int ret;
    BleAdvertisingData getAdvData = {};
    BleAdvertisingData setAdvData = {};

    setAdvData.appendServiceUUID("1234");
    ret = BLE.setAdvertisingData(&setAdvData);
    assertEqual(ret, 0);
    ret = BLE.getAdvertisingData(&getAdvData);
    assertEqual(ret, 7);
    assertEqual(getAdvData.length(), 7);

    BleUuid uuid;
    ret = getAdvData.serviceUUID(&uuid, 1);
    assertEqual(ret, 1);
    assertTrue(uuid == "1234");

    // Clear advertising data.
    ret = BLE.setAdvertisingData(nullptr);
    assertEqual(ret, 0);
    ret = BLE.getAdvertisingData(&getAdvData);
    assertEqual(ret, 0);
    assertEqual(getAdvData.length(), 0);
}

test(BLE_07_Set_BLE_Scan_Response_Data) {
    int ret;
    BleAdvertisingData getSrData = {};
    BleAdvertisingData setSrData = {};

    setSrData.appendServiceUUID("1234");
    ret = BLE.setScanResponseData(&setSrData);
    assertEqual(ret, 0);
    ret = BLE.getScanResponseData(&getSrData);
    assertEqual(ret, 4);
    assertEqual(getSrData.length(), 4);

    BleUuid uuid;
    ret = getSrData.serviceUUID(&uuid, 1);
    assertEqual(ret, 1);
    assertTrue(uuid == "1234");

    // Clear Scan response data.
    ret = BLE.setScanResponseData(nullptr);
    assertEqual(ret, 0);
    ret = BLE.getScanResponseData(&getSrData);
    assertEqual(ret, 0);
    assertEqual(getSrData.length(), 0);
}

test(BLE_08_BLE_Advertising_Control) {
    INFO("  > Testing BLE advertisement...\r\n");

    int ret;
    BleAdvertisingData setAdvData = {};
    BleAdvertisingData setSrData = {};
    BleAdvertisingData getAdvData = {};
    BleAdvertisingData getSrData = {};

    setAdvData.appendLocalName("Argon");
    setSrData.appendServiceUUID("1234");
    ret = BLE.advertise(&setAdvData, &setSrData);
    assertEqual(ret, 0);
    assertTrue(BLE.advertising());

    ret = BLE.getAdvertisingData(&getAdvData);
    assertEqual(ret, 10);
    assertEqual(getAdvData.length(), 10);
    ret = BLE.getScanResponseData(&getSrData);
    assertEqual(ret, 4);
    assertEqual(getSrData.length(), 4);

    String name = getAdvData.deviceName();
    assertTrue(name == "Argon");
    BleUuid uuid;
    ret = getSrData.serviceUUID(&uuid, 1);
    assertEqual(ret, 1);
    assertTrue(uuid == "1234");

    ret = BLE.setAdvertisingInterval(50); // In units of 0.625ms
    assertEqual(ret, 0);
    ret = BLE.setAdvertisingTimeout(200); // In units of 10ms, 2 seconds
    assertEqual(ret, 0);

    delay(2500);
    assertFalse(BLE.advertising());

    ret = BLE.advertise(); // Timeout: inherit from the value that is set at the last time.
    assertEqual(ret, 0);
    delay(2500);
    assertFalse(BLE.advertising());

    iBeacon beacon(1, 2, "9c1b8bdc-5548-4e32-8a78-b9f524131206", -55);
    ret = BLE.advertise(beacon); // Timeout: inherit from the value that is set at the last time.
    assertEqual(ret, 0);
    delay(1000);
    assertTrue(BLE.advertising());
    ret = BLE.stopAdvertising();
    assertEqual(ret, 0);
    assertFalse(BLE.advertising());
}

test(BLE_09_Set_BLE_Scanning_Parameters) {
    int ret;
    BleScanParams setScanParams = {};
    BleScanParams getScanParams = {};

    ret = BLE.setScanParameters(nullptr);
    assertNotEqual(ret, 0);

    setScanParams.size = sizeof(BleScanParams);
    setScanParams.interval = 50; // In units of 0.625ms
    setScanParams.window = 25; // In units of 0.625ms
    setScanParams.timeout = 100; // In units of 10ms
    setScanParams.active = false; // Do not send scan request
    setScanParams.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
    ret = BLE.setScanParameters(&setScanParams);
    assertEqual(ret, 0);

    getScanParams.size = sizeof(BleScanParams);
    ret = BLE.getScanParameters(&getScanParams);
    assertEqual(ret, 0);
    assertTrue(setScanParams.interval == getScanParams.interval);
    assertTrue(setScanParams.timeout == getScanParams.timeout);
    assertTrue(setScanParams.window == getScanParams.window);
    assertTrue(setScanParams.active == getScanParams.active);
    assertTrue(setScanParams.filter_policy == getScanParams.filter_policy);

    setScanParams.interval = 25;
    setScanParams.window = 50;
    ret = BLE.setScanParameters(&setScanParams);
    assertNotEqual(ret, 0);

    setScanParams.interval = 100;
    setScanParams.window = 50;
    ret = BLE.setScanParameters(&setScanParams);
    assertEqual(ret, 0);
    ret = BLE.setScanTimeout(1000); // In units of 10ms
    assertEqual(ret, 0);
    ret = BLE.getScanParameters(&getScanParams);
    assertEqual(ret, 0);
    assertEqual(getScanParams.timeout, 1000);
}

test(BLE_10_Add_BLE_Local_Characteristics) {
    BleCharacteristic characteristic(BleCharacteristicProperty::NOTIFY, "char1");
    BleCharacteristic char1 = BLE.addCharacteristic(characteristic);
    assertTrue(char1.UUID() == "F5720001-13A9-49DD-AC15-F87B7427E37B"); // Default particle assigned UUID
    assertTrue(char1.properties() == BleCharacteristicProperty::NOTIFY);
    assertTrue(char1.description() == "char1");

    BleCharacteristic char2 = BLE.addCharacteristic(BleCharacteristicProperty::READ, "char2");
    assertTrue(char2.UUID() == "F5720002-13A9-49DD-AC15-F87B7427E37B");
    assertTrue(char2.properties() == BleCharacteristicProperty::READ);
    assertTrue(char2.description() == "char2");

    BleCharacteristic char3 = BLE.addCharacteristic("char3", BleCharacteristicProperty::WRITE, 0x1234, 0x5678);
    assertTrue(char3.UUID() == 0x1234);
    assertTrue(char3.properties() == BleCharacteristicProperty::WRITE);
    assertTrue(char3.description() == "char3");

    BleCharacteristic char4 = BLE.addCharacteristic("char4", BleCharacteristicProperty::WRITE_WO_RSP, "8a37dbf2-c931-4102-9068-4bed3643e726", "58850a2a-38f6-4188-9f19-d867f02a028c ");
    assertTrue(char4.UUID() == "8A37DBF2-C931-4102-9068-4BED3643E726");
    assertTrue(char4.properties() == BleCharacteristicProperty::WRITE_WO_RSP);
    assertTrue(char4.description() == "char4");
}

test(BLE_11_BLE_UUID_Conversion) {
    const char* longUuidStr = "6E401234-B5A3-F393-E0A9-E50E24DCCA9E";
    const uint8_t longUuidArray[] = {0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x34, 0x12, 0x40, 0x6E};
    const uint8_t extendedShortUUID[] = {0xFB,0x34,0x9B,0x5F,0x80,0x00, 0x00,0x80, 0x00,0x10, 0x00,0x00, 0x78,0x56,0x00,0x00};
    const char* shortUuidStr = "5678";
    uint16_t shortUuid = 0x5678;
    uint8_t buffer[16];
    const uint8_t* pUuid;

    BleUuid longUuidFromArray(longUuidArray);
    assertTrue(longUuidFromArray.shorted() == 0x1234);
    assertTrue(longUuidFromArray.toString() == longUuidStr);
    
    BleUuid longUuidFromString(longUuidStr);
    size_t len = longUuidFromString.rawBytes(buffer);
    assertTrue(len == 16);
    assertTrue(!memcmp(buffer, longUuidArray, BLE_SIG_UUID_128BIT_LEN));
    pUuid = longUuidFromString.rawBytes();
    assertTrue(!memcmp(pUuid, longUuidArray, BLE_SIG_UUID_128BIT_LEN));

    BleUuid shortUuidFromUint16(shortUuid);
    assertTrue(shortUuidFromUint16.shorted() == 0x5678);
    assertTrue(shortUuidFromUint16.toString() == shortUuidStr);

    BleUuid shortUuidFromString(shortUuidStr);
    len = shortUuidFromString.rawBytes(buffer);
    assertTrue(len == 16);
    assertTrue(!memcmp(buffer, extendedShortUUID, BLE_SIG_UUID_128BIT_LEN));
    pUuid = shortUuidFromString.rawBytes();
    assertTrue(!memcmp(pUuid, extendedShortUUID, BLE_SIG_UUID_128BIT_LEN));
}

test(BLE_12_BLE_Address_Validation) {
    BleAddress address;
    assertFalse(address.isValid());

    // Public address
    address = "44:14:0A:9F:F2:96";
    assertTrue(address.isValid());
    address = "00:00:00:00:00:00"; // At least 1 bit is set
    assertFalse(address.isValid());
    address = "FF:FF:FF:FF:FF:FF"; // At least 1 bit is cleared
    assertFalse(address.isValid());

    // Static address. Two most significant bits should be 0b11 and
    // the reset of bits should at least have 1 bit set and cleared.
    address.type(BleAddressType::RANDOM_STATIC);
    address = "30:12:34:56:78:9a"; // 0b00
    assertFalse(address.isValid());
    address = "40:12:34:56:78:9a"; // 0b01
    assertFalse(address.isValid());
    address = "80:12:34:56:78:9a:"; // 0b10
    assertFalse(address.isValid());
    address = "C0:00:00:00:00:00"; // At least 1 bit is set
    assertFalse(address.isValid());
    address = "FF:FF:FF:FF:FF:FF"; // At least 1 bit is clear
    assertFalse(address.isValid());
    address = "C0:12:34:56:78:9a";
    assertTrue(address.isValid()); 

    // Non-resolvable address.  Two most significant bits should be 0b00 and
    // the reset of bits should at least have 1 bit set and cleared.
    address.type(BleAddressType::RANDOM_PRIVATE_NON_RESOLVABLE);
    address = "40:12:34:56:78:9a"; // 0b01
    assertFalse(address.isValid());
    address = "80:12:34:56:78:9a"; // 0b10
    assertFalse(address.isValid());
    address = "C0:12:34:56:78:9a"; // 0b11
    assertFalse(address.isValid());
    address = "00:00:00:00:00:00"; // At least 1 bit is set
    assertFalse(address.isValid());
    address = "3F:FF:FF:FF:FF:FF"; // At least 1 bit is clear
    assertFalse(address.isValid());
    address = "3F:12:34:56:78:9a";
    assertTrue(address.isValid()); 

    // Resolvable address. Two most significant bits should be 0b01 and
    // bit24 ~ bit45 should at least have 1 bit set and cleared.
    address.type(BleAddressType::RANDOM_PRIVATE_RESOLVABLE);
    address = "30:12:34:56:78:9a"; // 0b00
    assertFalse(address.isValid());
    address = "80:12:34:56:78:9a"; // 0b10
    assertFalse(address.isValid());
    address = "C0:12:34:56:78:9a"; // 0b11
    assertFalse(address.isValid());
    address = "40:00:00:12:34:56"; // At least 1 bit is set
    assertFalse(address.isValid());
    address = "7F:FF:FF:12:34:56:"; // At least 1 bit is cleared
    assertFalse(address.isValid());
    address = "4F:12:34:56:78:9a";
    assertTrue(address.isValid()); 

    address.clear();
    assertFalse(address.isValid());
}

#endif // #if Wiring_BLE == 1

