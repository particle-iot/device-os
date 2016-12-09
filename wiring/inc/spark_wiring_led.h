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

#ifndef SPARK_WIRING_LED_H
#define SPARK_WIRING_LED_H

#include "system_led_signal.h"

namespace particle {

class LEDStatus {
public:
    explicit LEDStatus(uint32_t color, LEDPattern pattern = LED_PATTERN_SOLID, LEDSpeed speed = LED_SPEED_NORMAL);
    LEDStatus(uint32_t color, LEDPriority priority, LEDPattern pattern = LED_PATTERN_SOLID, LEDSpeed speed = LED_SPEED_NORMAL);
    LEDStatus(uint32_t color, LEDSource source, LEDPattern pattern = LED_PATTERN_SOLID, LEDSpeed speed = LED_SPEED_NORMAL);
    LEDStatus(uint32_t color, LEDSource source, LEDPriority priority, LEDPattern pattern = LED_PATTERN_SOLID, LEDSpeed speed = LED_SPEED_NORMAL);
    ~LEDStatus();

    void setColor(uint32_t color);
    uint32_t color() const;

    void setPattern(LEDPattern pattern);
    LEDPattern pattern() const;

    void setSpeed(LEDSpeed speed);
    LEDSpeed speed() const;

    void on();
    void off();
    void toggle();
    bool isOn();

    void setActive(LEDPriority priority); // Activate and override priority
    void setActive(bool active = true);
    bool isActive() const;

    LEDPriority priority() const;

private:
    LEDStatusData status_;
};

class LEDTheme {
public:
    LEDTheme();

    void setColor(LEDSignal signal, int colorIndex);
    int color(LEDSignal signal) const;

    void setPattern(LEDSignal signal, LEDPattern pattern);
    LEDPattern pattern(LEDSignal signal) const;

    void setSpeed(LEDSignal signal, LEDSpeed speed);
    LEDSpeed speed(LEDSignal signal) const;

    void setPaletteColor(int index, uint32_t color);
    uint32_t paletteColor(int index) const;

    void set();

    static void setDefault();

private:
    LEDThemeData theme_;
};

} // namespace particle

// particle::LEDStatus
inline particle::LEDStatus::LEDStatus(uint32_t color, LEDPattern pattern, LEDSpeed speed) :
        LEDStatus(color, LED_SOURCE_DEFAULT, LED_PRIORITY_NORMAL, pattern, speed) {
}

inline particle::LEDStatus::LEDStatus(uint32_t color, LEDPriority priority, LEDPattern pattern, LEDSpeed speed) :
        LEDStatus(color, LED_SOURCE_DEFAULT, priority, pattern, speed) {
}

inline particle::LEDStatus::LEDStatus(uint32_t color, LEDSource source, LEDPattern pattern, LEDSpeed speed) :
        LEDStatus(color, source, LED_PRIORITY_NORMAL, pattern, speed) {
}

inline particle::LEDStatus::LEDStatus(uint32_t color, LEDSource source, LEDPriority priority, LEDPattern pattern, LEDSpeed speed) :
        status_{ sizeof(LEDStatusData), nullptr /* Internal */, nullptr /* Internal */, color, pattern, speed,
            0 /* Flags */, LED_PRIORITY_VALUE(priority, source) } {
}

inline particle::LEDStatus::~LEDStatus() {
    setActive(false);
}

inline void particle::LEDStatus::setColor(uint32_t color) {
    status_.color = color;
}

inline uint32_t particle::LEDStatus::color() const {
    return status_.color;
}

inline void particle::LEDStatus::setPattern(LEDPattern pattern) {
    status_.pattern = pattern;
}

inline LEDPattern particle::LEDStatus::pattern() const {
    return (LEDPattern)status_.pattern;
}

inline void particle::LEDStatus::setSpeed(LEDSpeed speed) {
    status_.speed = speed;
}

inline LEDSpeed particle::LEDStatus::speed() const {
    return (LEDSpeed)status_.speed;
}

inline void particle::LEDStatus::on() {
    status_.flags &= ~LED_STATUS_FLAG_OFF;
}

inline void particle::LEDStatus::off() {
    status_.flags |= LED_STATUS_FLAG_OFF;
}

inline void particle::LEDStatus::toggle() {
    status_.flags ^= LED_STATUS_FLAG_OFF;
}

inline bool particle::LEDStatus::isOn() {
    return !(status_.flags & LED_STATUS_FLAG_OFF);
}

inline void particle::LEDStatus::setActive(LEDPriority priority) {
    status_.priority = (status_.priority & 0x03) | ((uint8_t)priority << 2);
    setActive(true);
}

inline void particle::LEDStatus::setActive(bool active) {
    led_set_status_active(&status_, active, nullptr);
}

inline bool particle::LEDStatus::isActive() const {
    return status_.flags & LED_STATUS_FLAG_ACTIVE;
}

inline LEDPriority particle::LEDStatus::priority() const {
    return (LEDPriority)(status_.priority >> 2);
}

// particle::LEDTheme
inline particle::LEDTheme::LEDTheme() :
        theme_{ LED_THEME_DATA_VERSION } {
    led_get_signal_theme(&theme_, 0, nullptr); // Get current theme
}

inline void particle::LEDTheme::setColor(LEDSignal signal, int colorIndex) {
    theme_.signals[signal].color = colorIndex;
}

inline int particle::LEDTheme::color(LEDSignal signal) const {
    return theme_.signals[signal].color;
}

inline void particle::LEDTheme::setPattern(LEDSignal signal, LEDPattern pattern) {
    theme_.signals[signal].pattern = pattern;
}

inline LEDPattern particle::LEDTheme::pattern(LEDSignal signal) const {
    return (LEDPattern)theme_.signals[signal].pattern;
}

inline void particle::LEDTheme::setSpeed(LEDSignal signal, LEDSpeed speed) {
    theme_.signals[signal].speed = speed;
}

inline LEDSpeed particle::LEDTheme::speed(LEDSignal signal) const {
    return (LEDSpeed)theme_.signals[signal].speed;
}

inline void particle::LEDTheme::setPaletteColor(int index, uint32_t color) {
    if (index >= 0 && index < LED_PALETTE_COLOR_COUNT) {
        theme_.palette[index] = color;
    }
}

inline uint32_t particle::LEDTheme::paletteColor(int index) const {
    if (index >= 0 && index < LED_PALETTE_COLOR_COUNT) {
        return theme_.palette[index];
    }
    return 0;
}

inline void particle::LEDTheme::set() {
    led_set_signal_theme(&theme_, 0, nullptr);
}

inline void particle::LEDTheme::setDefault() {
    led_set_signal_theme(nullptr, LED_THEME_FLAG_DEFAULT, nullptr);
}

#endif // SPARK_WIRING_LED_H
