/*
 * Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.
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

#include "testapi.h"

namespace {

// Dummy status implementing a custom signaling pattern
class CustomStatus: public LEDStatus {
public:
    CustomStatus() :
            LEDStatus(LED_PATTERN_CUSTOM) {
    }

protected:
    virtual void update(system_tick_t) override {
    }
};

} // namespace

test(api_led_status) {
    // LEDStatus()
    API_COMPILE(LEDStatus());
    API_COMPILE(LEDStatus(LED_PATTERN_BLINK));
    API_COMPILE(LEDStatus(LED_PATTERN_BLINK, LED_SPEED_NORMAL));
    API_COMPILE(LEDStatus(LED_PATTERN_BLINK, 1000 /* period */));
    API_COMPILE(LEDStatus(RGB_COLOR_WHITE));
    API_COMPILE(LEDStatus(RGB_COLOR_WHITE, LED_PATTERN_BLINK));
    API_COMPILE(LEDStatus(RGB_COLOR_WHITE, LED_PATTERN_BLINK, LED_SPEED_NORMAL));
    API_COMPILE(LEDStatus(RGB_COLOR_WHITE, LED_PATTERN_BLINK, 1000 /* period */));
    // Same as above plus priority argument
    API_COMPILE(LEDStatus(LED_PRIORITY_NORMAL));
    API_COMPILE(LEDStatus(LED_PATTERN_BLINK, LED_PRIORITY_NORMAL));
    API_COMPILE(LEDStatus(LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_NORMAL));
    API_COMPILE(LEDStatus(LED_PATTERN_BLINK, 1000 /* period */, LED_PRIORITY_NORMAL));
    API_COMPILE(LEDStatus(RGB_COLOR_WHITE, LED_PRIORITY_NORMAL));
    API_COMPILE(LEDStatus(RGB_COLOR_WHITE, LED_PATTERN_BLINK, LED_PRIORITY_NORMAL));
    API_COMPILE(LEDStatus(RGB_COLOR_WHITE, LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_NORMAL));
    API_COMPILE(LEDStatus(RGB_COLOR_WHITE, LED_PATTERN_BLINK, 1000 /* period */, LED_PRIORITY_NORMAL));

    // setColor(), color()
    LEDStatus status;
    uint32_t color = RGB_COLOR_WHITE;
    API_COMPILE(status.setColor(color));
    API_COMPILE(color = status.color());

    // setPattern(), pattern()
    LEDPattern pattern = LED_PATTERN_BLINK;
    API_COMPILE(status.setPattern(pattern));
    API_COMPILE(pattern = status.pattern());

    // setSpeed(), setPeriod(), period()
    LEDSpeed speed = LED_SPEED_NORMAL;
    uint16_t period = 1000;
    API_COMPILE(status.setSpeed(speed));
    API_COMPILE(status.setPeriod(period));
    API_COMPILE(period = status.period());

    // setPriority(), priority()
    LEDPriority priority = LED_PRIORITY_NORMAL;
    API_COMPILE(status.setPriority(priority));
    API_COMPILE(priority = status.priority());

    // on(), off(), toggle(), isOn(), isOff()
    bool flag = false;
    API_COMPILE(status.on());
    API_COMPILE(status.off());
    API_COMPILE(status.toggle());
    API_COMPILE(flag = status.isOn());
    API_COMPILE(flag = status.isOff());

    // setActive(), isActive()
    API_COMPILE(status.setActive(priority));
    API_COMPILE(status.setActive(flag));
    API_COMPILE(status.setActive());
    API_COMPILE(flag = status.isActive());
}

test(api_led_system_theme) {
    // LEDSystemTheme()
    API_COMPILE(LEDSystemTheme());

    // setColor(), color()
    LEDSystemTheme theme;
    LEDSignal signal = LED_SIGNAL_NETWORK_ON;
    uint32_t color = RGB_COLOR_WHITE;
    API_COMPILE(theme.setColor(signal, color));
    API_COMPILE(color = theme.color(signal));

    // setPattern(), pattern()
    LEDPattern pattern = LED_PATTERN_BLINK;
    API_COMPILE(theme.setPattern(signal, pattern));
    API_COMPILE(pattern = theme.pattern(signal));

    // setSpeed(), setPeriod(), period()
    LEDSpeed speed = LED_SPEED_NORMAL;
    uint16_t period = 1000;
    API_COMPILE(theme.setSpeed(signal, speed));
    API_COMPILE(theme.setPeriod(signal, period));
    API_COMPILE(period = theme.period(signal));

    // setSignal()
    API_COMPILE(theme.setSignal(signal, color));
    API_COMPILE(theme.setSignal(signal, color, pattern));
    API_COMPILE(theme.setSignal(signal, color, pattern, speed));
    API_COMPILE(theme.setSignal(signal, color, pattern, period));

    // apply()
    bool flag = false;
    API_COMPILE(theme.apply(flag));
    API_COMPILE(theme.apply());

    // restoreDefault()
    API_COMPILE(LEDSystemTheme::restoreDefault());
}
