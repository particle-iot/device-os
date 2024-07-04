/*
 * Copyright (c) 2024 Particle Industries, Inc.  All rights reserved.
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

// Flash the Lora firmware through SWD with Daplink:
// openocd -f interface/cmsis-dap.cfg -c "transport select swd" -f target/stm32wlx.cfg  -c init -c "reset halt; wait_halt; flash write_image erase KG200Z_AT.bin 0x08000000" -c reset -c shutdown
// Flash the Lora firmware through SWD with Jlink:
// openocd -f interface/jlink.cfg -c "transport select swd" -f target/stm32wlx.cfg  -c init -c "reset halt; wait_halt; flash write_image erase KG200Z_AT.bin 0x08000000" -c reset -c shutdown

#include "application.h"
#include "stusb4500_api.h"
#include "am18x5.h"
#include "kg200z.h"
#include "tmp112a.h"
#include "power_module.h"

SYSTEM_MODE(MANUAL);

STARTUP (
    System.setPowerConfiguration(SystemPowerConfiguration().auxPowerControlPin(PowerModule::auxEnPin()).feature(SystemPowerFeature::PMIC_DETECTION));

    System.enableFeature(FEATURE_ETHERNET_DETECTION);

    if_wiznet_pin_remap remap = {};
    remap.base.type = IF_WIZNET_DRIVER_SPECIFIC_PIN_REMAP;
    remap.cs_pin = A3;
    remap.reset_pin = PIN_INVALID;
    remap.int_pin = A4;
    if_request(nullptr, IF_REQ_DRIVER_SPECIFIC, &remap, sizeof(remap), nullptr);
);

Serial1LogHandler l1(115200, LOG_LEVEL_ALL);
SerialLogHandler l2(115200, LOG_LEVEL_ALL);

void setup() {
    Log.info("<< Test program for Muon v0.5>>\r\n");

    Wire.begin();

    PowerModule::getInstance().begin();
    PowerModule::getInstance().setAlertThreshold(25);
    Log.info("Fuel Gauge version: 0x%02X", PowerModule::getInstance().getFuelGaugeVersion());
    Log.info("PMIC version: 0x%02X, expected: 0x23", PowerModule::getInstance().getPmicVersion());

    stusb4500Init();
    stusb4500SetDefaultParams();
    stusb4500LogParams();
    stusb4500LogStatus();

    delay(250);

    Kg200z::getInstance().begin();
    Kg200z::getInstance().getVersion();

    Tmp112a::getInstance().begin();

    uint16_t id;
    Am18x5::getInstance().getPartNumber(&id);
    Log.info("Am18x5 Part Number: 0x%04x", id);

    struct timeval tv = {};
    tv.tv_sec = 1716819790;
    Log.info("Set time: %ld", tv.tv_sec);
    Am18x5::getInstance().setTime(&tv);

    // Am18x5::getInstance().setPsw(1);
}

void loop() {
    static system_tick_t startTime = 0;
    if (millis() - startTime > 3000) {
        startTime = millis();

        float temp;
        Tmp112a::getInstance().getTemperature(&temp);
        Log.info("Temperature: %.2f", temp);

        struct timeval tv = {};
        Am18x5::getInstance().getTime(&tv);
        struct tm calendar = {};
        gmtime_r(&tv.tv_sec, &calendar);
        Log.info("Time: %ld-%ld-%ld, %ld:%ld:%ld",
            calendar.tm_year + 1900, calendar.tm_mon + 1, calendar.tm_mday, calendar.tm_hour, calendar.tm_min, calendar.tm_sec);

        FuelGauge fuelGauge;
        Log.info("Current percentage: %.2f%%", PowerModule::getInstance().getSoc());
        if (PowerModule::getInstance().isBatLevelLow()) {
            Log.info("low battery alert, low than 25%% !!!");
            PowerModule::getInstance().clearFuelGaugeAlert();
        }
    }
}
