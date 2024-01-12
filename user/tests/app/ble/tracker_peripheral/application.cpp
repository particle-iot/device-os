/*
 * Copyright (c) 2020 Particle Industries, Inc.  All rights reserved.
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
#include "deviceid_hal.h"

SYSTEM_MODE(MANUAL);

Serial1LogHandler log(115200, LOG_LEVEL_ALL);

// UUID of present BLE services
constexpr ble_sig_uuid_t disUuid = BLE_SIG_UUID_DEVICE_INFORMATION_SVC;
constexpr ble_sig_uuid_t basUuid = BLE_SIG_UUID_BATTERY_SVC;
constexpr ble_sig_uuid_t lnsUuid = BLE_SIG_UUID_LOCATION_NAVIGATION_SVC;

// UUID of characteristics under the DIS
constexpr uint16_t systemIdCharUuid = 0x2A23; //The OUI is basically the first three octets of a MAC address
constexpr uint16_t modelNumStrCharUuid = 0x2A24;
constexpr uint16_t serialNumStrCharUuid = 0x2A25;
constexpr uint16_t fwRevStrCharUuid = 0x2A26;
constexpr uint16_t hwRevStrCharUuid = 0x2A27;
constexpr uint16_t swRevStrCharUuid = 0x2A28;
constexpr uint16_t mfgNameStrCharUuid = 0x2A29;
constexpr uint16_t ieeeRcdlCharUuid = 0x2A2A;
constexpr uint16_t pnpCharUuid = 0x2A50;

// UUID of characteristic under the BAS
constexpr uint16_t batLevelCharUuid = 0x2A19;

// UUID of characteristics under the LNS
constexpr uint16_t lnFeatureCharUuid = 0x2A6A;
constexpr uint16_t locAndSpeedCharUuid = 0x2A67;
// constexpr uint16_t posQualityCharUuid = 0x2A69;
// constexpr uint16_t lnCtrlPointCharUuid = 0x2A6B;
// constexpr uint16_t navigationCharUuid = 0x2A68;

void onLnCtrlPointReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

// Characteristics definition
BleCharacteristic mfgNameStrChar(nullptr, BleCharacteristicProperty::READ, mfgNameStrCharUuid, disUuid);
BleCharacteristic modelNumStrChar(nullptr, BleCharacteristicProperty::READ, modelNumStrCharUuid, disUuid);
BleCharacteristic serialNumStrChar(nullptr, BleCharacteristicProperty::READ, serialNumStrCharUuid, disUuid);
BleCharacteristic hwRevStrChar(nullptr, BleCharacteristicProperty::READ, hwRevStrCharUuid, disUuid);
BleCharacteristic fwRevStrChar(nullptr, BleCharacteristicProperty::READ, fwRevStrCharUuid, disUuid);
BleCharacteristic swRevStrChar(nullptr, BleCharacteristicProperty::READ, swRevStrCharUuid, disUuid);
BleCharacteristic systemIdChar(nullptr, BleCharacteristicProperty::READ, systemIdCharUuid, disUuid);
BleCharacteristic ieeeRcdlChar(nullptr, BleCharacteristicProperty::READ, ieeeRcdlCharUuid, disUuid);
BleCharacteristic pnpChar(nullptr, BleCharacteristicProperty::READ, pnpCharUuid, disUuid);

BleCharacteristic batLevelChar(nullptr, BleCharacteristicProperty::READ | BleCharacteristicProperty::NOTIFY, batLevelCharUuid, basUuid);

BleCharacteristic lnFeatureChar(nullptr, BleCharacteristicProperty::READ, lnFeatureCharUuid, lnsUuid);
BleCharacteristic locAndSpeedChar(nullptr, BleCharacteristicProperty::NOTIFY, locAndSpeedCharUuid, lnsUuid);
// BleCharacteristic posQualityChar(nullptr, BleCharacteristicProperty::READ, posQualityCharUuid, lnsUuid);
// BleCharacteristic lnCtrlPointChar(nullptr, BleCharacteristicProperty::WRITE | BleCharacteristicProperty::INDICATE, lnCtrlPointCharUuid, lnsUuid, onLnCtrlPointReceived, &lnCtrlPointChar);
// BleCharacteristic navigationChar(nullptr, BleCharacteristicProperty::NOTIFY, navigationCharUuid, lnsUuid);


void onLnCtrlPointReceived(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
    BleAddress address = peer.address();
    LOG(TRACE, "Received data from: %s", address.toString().c_str());

    BleCharacteristic* characteristic = static_cast<BleCharacteristic*>(context);
    Serial1.printf("Characteristic UUID: %s\r\n", characteristic->UUID().toString().c_str());

    for (uint8_t i = 0; i < len; i++) {
        Serial1.write(data[i]);
    }
}

void setup() {
    LOG(TRACE, "Application started.");

    BLE.addCharacteristic(mfgNameStrChar);
    BLE.addCharacteristic(modelNumStrChar);
    BLE.addCharacteristic(serialNumStrChar);
    BLE.addCharacteristic(hwRevStrChar);
    BLE.addCharacteristic(fwRevStrChar);
    BLE.addCharacteristic(swRevStrChar);
    BLE.addCharacteristic(systemIdChar);
    BLE.addCharacteristic(ieeeRcdlChar);
    BLE.addCharacteristic(pnpChar);

    BLE.addCharacteristic(batLevelChar);

    BLE.addCharacteristic(lnFeatureChar);
    BLE.addCharacteristic(locAndSpeedChar);
    // BLE.addCharacteristic(posQualityChar);
    // BLE.addCharacteristic(lnCtrlPointChar);
    // BLE.addCharacteristic(navigationChar);

    // Initial value of characteristics under the DIS
    {
        char str[10] = "";

        mfgNameStrChar.setValue("Particle");

        uint32_t model, variant;
        if (hal_get_device_hw_model(&model, &variant, nullptr) != SYSTEM_ERROR_NONE) {
            modelNumStrChar.setValue("Unknown");
        } else {
            utoa(model, str, 16);
            modelNumStrChar.setValue(str);
        }

        serialNumStrChar.setValue("Unknown"); // The HAL API to get serial number is not exported.

        uint32_t hw;
        if (hal_get_device_hw_version(&hw, nullptr) != SYSTEM_ERROR_NONE) {
            hwRevStrChar.setValue("Unknown");
        } else {
            memset(str, 0, sizeof(str));
            utoa(hw, str, 16);
            hwRevStrChar.setValue(str);
        }

        SystemVersionInfo fwInfo = {};
        system_version_info(&fwInfo, nullptr);
        fwRevStrChar.setValue(fwInfo.versionString);

        swRevStrChar.setValue("0.0.1-rc.1");
        systemIdChar.setValue(0x1234);
        ieeeRcdlChar.setValue(0x5678);
        pnpChar.setValue(0x0662); // Particle company ID
    }

    {
        /* LN Feature bit flags (bit0 - bit20):
         * LNS_FEATURE_INSTANT_SPEED
         * LNS_FEATURE_TOTAL_DISTANCE
         * LNS_FEATURE_LOCATION - 1
         * LNS_FEATURE_ELEVATION
         * LNS_FEATURE_HEADING
         * LNS_FEATURE_ROLLING_TIME
         * LNS_FEATURE_UTC_TIME - 1
         * LNS_FEATURE_REMAINING_DISTANCE
         * LNS_FEATURE_REMAINING_VERT_DISTANCE
         * LNS_FEATURE_EST_TIME_OF_ARRIVAL
         * LNS_FEATURE_NUM_SATS_IN_SOLUTION
         * LNS_FEATURE_NUM_SATS_IN_VIEW
         * LNS_FEATURE_TIME_TO_FIRST_FIX
         * LNS_FEATURE_EST_HORZ_POS_ERROR
         * LNS_FEATURE_EST_VERT_POS_ERROR
         * LNS_FEATURE_HORZ_DILUTION_OF_PRECISION
         * LNS_FEATURE_VERT_DILUTION_OF_PRECISION
         * LNS_FEATURE_LOC_AND_SPEED_CONTENT_MASKING
         * LNS_FEATURE_FIX_RATE_SETTING
         * LNS_FEATURE_ELEVATION_SETTING
         * LNS_FEATURE_POSITION_STATUS
         */
        uint32_t lnFeature = 0x00000044;
        lnFeatureChar.setValue(lnFeature);
    }

    BLE.advertise();
}

void loop() {
    static system_tick_t now = millis();

    static uint8_t batLevel = 100;
    static bool charging = false;

    static uint32_t latitude = 0, longtitude = 0;

    // Simulate battery charging and discharging
    // Notify battery level every 5 seconds
    if (millis() - now >= 1000) {
        now = millis();

        {
            if (charging) {
                batLevel++;
                if (batLevel == 100) {
                    charging = false;
                }
            } else {
                batLevel--;
                if (batLevel == 0) {
                    charging = true;
                }
            }
            batLevelChar.setValue(batLevel);
        }

        {
            uint16_t flags = 0x0044; // Bit6 for UTC time and bit2 for location
            latitude++;
            longtitude++;
            struct tm calendar;
            time_t t = System.uptime();
            gmtime_r(&t, &calendar);

            uint8_t encodedBuff[20];
            uint8_t len = 0;

            // Flags field
            encodedBuff[len++] = (uint8_t) ((flags & 0x00FF) >> 0);
            encodedBuff[len++] = (uint8_t) ((flags & 0xFF00) >> 8);
            // Location field
            encodedBuff[len++] = (uint8_t) ((latitude & 0x000000FF) >> 0);
            encodedBuff[len++] = (uint8_t) ((latitude & 0x0000FF00) >> 8);
            encodedBuff[len++] = (uint8_t) ((latitude & 0x00FF0000) >> 16);
            encodedBuff[len++] = (uint8_t) ((latitude & 0xFF000000) >> 24);
            encodedBuff[len++] = (uint8_t) ((longtitude & 0x000000FF) >> 0);
            encodedBuff[len++] = (uint8_t) ((longtitude & 0x0000FF00) >> 8);
            encodedBuff[len++] = (uint8_t) ((longtitude & 0x00FF0000) >> 16);
            encodedBuff[len++] = (uint8_t) ((longtitude & 0xFF000000) >> 24);
            // UTC time field
            calendar.tm_year += 1900;
            encodedBuff[len++] = (uint8_t) ((calendar.tm_year & 0x00FF) >> 0);
            encodedBuff[len++] = (uint8_t) ((calendar.tm_year & 0xFF00) >> 8);

            calendar.tm_mon += 1;
            encodedBuff[len++] = calendar.tm_mon;
            encodedBuff[len++] = calendar.tm_mday;
            encodedBuff[len++] = calendar.tm_hour;
            encodedBuff[len++] = calendar.tm_min;
            encodedBuff[len++] = calendar.tm_sec;

            Log.info("%d-%d-%d %d:%d:%d", calendar.tm_year, calendar.tm_mon, calendar.tm_mday, calendar.tm_hour, calendar.tm_min, calendar.tm_sec);

            locAndSpeedChar.setValue(encodedBuff, len);
        }
    }
}
