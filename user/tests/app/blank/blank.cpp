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

#include "application.h"

/* executes once at startup */
void setup() {
    Time.now();
    Time.getTimeSource();

    InternalTime.now();
    InternalTime.getTimeSource();
    ExternalTime.now();
    ExternalTime.getTimeSource();

    ExternalTime.enable(RtcConfiguration().i2c(Wire, 0x11).interrupt(D3).pin(1, D4).type(RtcType::AM18X5).defaultTimeSource(true));
    ExternalTime.enable(Am18x5Configuration().i2c(Wire, 0x11).interrupt(D3).watchdogPin(D4).type(RtcType::AM18X5).defaultTimeSource(true));
    ExternalTime.enable(Am18x5Configuration().i2c(Wire).interrupt(D3).watchdogPin(D4).type(RtcType::AM18X5).defaultTimeSource(true));
    ExternalTime.enable(Am18x5Configuration().i2c(Wire).interrupt(D3).watchdogPin(D4).defaultTimeSource(false).xtalCalibration(-12));

    Am18x5.enable(RtcConfiguration().i2c(Wire, 0x11).interrupt(D3).pin(1, D4).type(RtcType::AM18X5).defaultTimeSource(true));
    Am18x5.enable(Am18x5Configuration().i2c(Wire, 0x11).interrupt(D3).watchdogPin(D4).type(RtcType::AM18X5).defaultTimeSource(true));
    Am18x5.enable(Am18x5Configuration().i2c(Wire).interrupt(D3).watchdogPin(D4).type(RtcType::AM18X5).defaultTimeSource(true));
    Am18x5.enable(Am18x5Configuration().i2c(Wire).interrupt(D3).watchdogPin(D4).defaultTimeSource(false).xtalCalibration(-12).clockSource(RtcClockSource::EXTERNAL).capabilities(RtcCap::EXTI_LEVEL_TRIGGER));


    System.sleep(SystemSleepConfiguration().mode(SystemSleepMode::POWER_OFF).duration(5s).gpio(PinType::RTC, FALLING));


    RtcConfiguration extCfgOut;
    int extGet0 = ExternalTime.getConfig(extCfgOut);
    auto extCfgRet = ExternalTime.getConfig();
    int extSet0 = ExternalTime.setConfig(extCfgOut);
    int extSet1 = ExternalTime.setConfig(extCfgRet);
    int extSet2 = ExternalTime.setConfig(ExternalTime.getConfig().defaultTimeSource(false).capabilities(RtcCaps{}));

    Am18x5Configuration amCfgOut;
    int amGet0 = Am18x5.getConfig(amCfgOut);
    auto amCfgRet = Am18x5.getConfig();
    int amSet0 = Am18x5.setConfig(amCfgOut);
    int amSet1 = Am18x5.setConfig(amCfgRet);
    int amSet2 = Am18x5.setConfig(Am18x5.getConfig().defaultTimeSource(true).xtalCalibration(4));

    int extDisable = ExternalTime.disable();
    int amDisable = Am18x5.disable();
}

/* executes continuously after setup() runs */
void loop() {

}
