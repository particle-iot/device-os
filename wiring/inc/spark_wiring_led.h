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

// This class allows to implement a custom RGB LED indication that can be used at the same time
// with the system indication, according to the predefined LED priorities
class LEDStatus {
public:
    explicit LEDStatus(LEDPriority priority = LED_PRIORITY_NORMAL, LEDSource source = LED_SOURCE_DEFAULT);
    explicit LEDStatus(LEDPattern pattern, LEDPriority priority = LED_PRIORITY_NORMAL, LEDSource source = LED_SOURCE_DEFAULT);
    LEDStatus(LEDPattern pattern, LEDSpeed speed, LEDPriority priority = LED_PRIORITY_NORMAL, LEDSource source = LED_SOURCE_DEFAULT);
    LEDStatus(LEDPattern pattern, uint16_t period, LEDPriority priority = LED_PRIORITY_NORMAL, LEDSource source = LED_SOURCE_DEFAULT);

    explicit LEDStatus(uint32_t color, LEDPriority priority = LED_PRIORITY_NORMAL, LEDSource source = LED_SOURCE_DEFAULT);
    LEDStatus(uint32_t color, LEDPattern pattern, LEDPriority priority = LED_PRIORITY_NORMAL, LEDSource source = LED_SOURCE_DEFAULT);
    LEDStatus(uint32_t color, LEDPattern pattern, LEDSpeed speed, LEDPriority priority = LED_PRIORITY_NORMAL, LEDSource source = LED_SOURCE_DEFAULT);
    LEDStatus(uint32_t color, LEDPattern pattern, uint16_t period, LEDPriority priority = LED_PRIORITY_NORMAL, LEDSource source = LED_SOURCE_DEFAULT);

    virtual ~LEDStatus();

    void setColor(uint32_t color);
    uint32_t color() const;

    void setPattern(LEDPattern pattern);
    LEDPattern pattern() const;

    void setSpeed(LEDSpeed speed);
    void setPeriod(uint16_t period);
    uint16_t period() const;

    void setPriority(LEDPriority priority);
    LEDPriority priority() const;

    void on();
    void off();
    void toggle();
    bool isOn();
    bool isOff();

    void setActive(LEDPriority priority); // Activate and override priority
    void setActive(bool active = true);
    bool isActive() const;

protected:
    // This method can be overridden to implement a custom signaling pattern.
    //
    // The system calls this method periodically for an active status instance constructed with
    // LED_PATTERN_CUSTOM pattern type. `ticks` argument contains a number of milliseconds passed
    // since previous update.
    //
    // NOTE: The system may call this method within an ISR. Ensure provided implementation doesn't
    // make any blocking calls, returns as quickly as possible, and, ideally, only updates internal
    // timing and makes calls to setColor(), setActive() and other methods of this class.
    virtual void update(system_tick_t ticks);

private:
    LEDStatusData d_;

    static void updateCallback(system_tick_t ticks, void* data);
};

// Class allowing to set a custom theme for system LED signaling
class LEDSystemTheme {
public:
    LEDSystemTheme();

    void setColor(LEDSignal signal, uint32_t color);
    uint32_t color(LEDSignal signal) const;

    // Note: System LED signaling doesn't support custom patterns
    void setPattern(LEDSignal signal, LEDPattern pattern);
    LEDPattern pattern(LEDSignal signal) const;

    void setSpeed(LEDSignal signal, LEDSpeed speed);
    void setPeriod(LEDSignal signal, uint16_t period);
    uint16_t period(LEDSignal signal) const;

    // Convenience methods setting several parameters at once
    void setSignal(LEDSignal signal, uint32_t color);
    void setSignal(LEDSignal signal, uint32_t color, LEDPattern pattern, LEDSpeed speed = LED_SPEED_NORMAL);
    void setSignal(LEDSignal signal, uint32_t color, LEDPattern pattern, uint16_t period);

    // Sets this theme as current theme used by the system. If `save` argument is set to `true`,
    // the theme will be saved to persistent storage and used as a default theme after reboot
    void apply(bool save = false);

    // Restores factory default theme
    static void restoreDefault();

private:
    LEDSignalThemeData d_;
};

} // namespace particle

// particle::LEDStatus
inline particle::LEDStatus::LEDStatus(LEDPriority priority, LEDSource source) :
        LEDStatus(LED_PATTERN_SOLID, priority, source) { // Use solid pattern by default
}

inline particle::LEDStatus::LEDStatus(LEDPattern pattern, LEDPriority priority, LEDSource source) :
        LEDStatus(pattern, LED_SPEED_NORMAL, priority, source) { // Use normal speed by default
}

inline particle::LEDStatus::LEDStatus(LEDPattern pattern, LEDSpeed speed, LEDPriority priority, LEDSource source) :
        LEDStatus(pattern, led_pattern_period(pattern, speed, nullptr), priority, source) { // Get pattern period for a predefined speed value
}

inline particle::LEDStatus::LEDStatus(LEDPattern pattern, uint16_t period, LEDPriority priority, LEDSource source) :
        LEDStatus(RGB_COLOR_WHITE, pattern, period, priority, source) { // Use white color by default
}

inline particle::LEDStatus::LEDStatus(uint32_t color, LEDPriority priority, LEDSource source) :
        LEDStatus(color, LED_PATTERN_SOLID, priority, source) { // Use solid pattern by default
}

inline particle::LEDStatus::LEDStatus(uint32_t color, LEDPattern pattern, LEDPriority priority, LEDSource source) :
        LEDStatus(color, pattern, LED_SPEED_NORMAL, priority, source) { // Use normal speed by default
}

inline particle::LEDStatus::LEDStatus(uint32_t color, LEDPattern pattern, LEDSpeed speed, LEDPriority priority, LEDSource source) :
        LEDStatus(color, pattern, led_pattern_period(pattern, speed, nullptr), priority, source) { // Get pattern period for a predefined speed value
}

inline particle::LEDStatus::~LEDStatus() {
    setActive(false);
}

inline void particle::LEDStatus::setColor(uint32_t color) {
    d_.color = color;
}

inline uint32_t particle::LEDStatus::color() const {
    return d_.color;
}

inline void particle::LEDStatus::setPattern(LEDPattern pattern) {
    // Custom pattern type can be set only at constuction time
    if (pattern != LED_PATTERN_CUSTOM && d_.pattern != LED_PATTERN_CUSTOM) {
        d_.pattern = pattern;
    }
}

inline LEDPattern particle::LEDStatus::pattern() const {
    return (LEDPattern)d_.pattern;
}

inline void particle::LEDStatus::setSpeed(LEDSpeed speed) {
    setPeriod(led_pattern_period(d_.pattern, speed, nullptr));
}

inline void particle::LEDStatus::setPeriod(uint16_t period) {
    // Pattern period cannot be set for custom pattern type
    if (d_.pattern != LED_PATTERN_CUSTOM) {
        d_.period = period;
    }
}

inline uint16_t particle::LEDStatus::period() const {
    return (d_.pattern != LED_PATTERN_CUSTOM ? d_.period : 0);
}

inline void particle::LEDStatus::setPriority(LEDPriority priority) {
    d_.priority = (d_.priority & 0x03) | ((uint8_t)priority << 2);
}

inline LEDPriority particle::LEDStatus::priority() const {
    return (LEDPriority)(d_.priority >> 2);
}

inline void particle::LEDStatus::on() {
    d_.flags &= ~LED_STATUS_FLAG_OFF;
}

inline void particle::LEDStatus::off() {
    d_.flags |= LED_STATUS_FLAG_OFF;
}

inline void particle::LEDStatus::toggle() {
    d_.flags ^= LED_STATUS_FLAG_OFF;
}

inline bool particle::LEDStatus::isOn() {
    return !(d_.flags & LED_STATUS_FLAG_OFF);
}

inline bool particle::LEDStatus::isOff() {
    return d_.flags & LED_STATUS_FLAG_OFF;
}

inline void particle::LEDStatus::setActive(LEDPriority priority) {
    setPriority(priority);
    setActive(true);
}

inline void particle::LEDStatus::setActive(bool active) {
    led_set_status_active(&d_, active, nullptr);
}

inline bool particle::LEDStatus::isActive() const {
    return d_.flags & LED_STATUS_FLAG_ACTIVE;
}

inline void particle::LEDStatus::update(system_tick_t ticks) {
    // Default implementation does nothing
}

// particle::LEDSystemTheme
inline particle::LEDSystemTheme::LEDSystemTheme() :
        d_{ LED_SIGNAL_THEME_VERSION } {
    led_get_signal_theme(&d_, 0, nullptr); // Get current theme
}

inline void particle::LEDSystemTheme::setColor(LEDSignal signal, uint32_t color) {
    d_.signals[signal].color = color;
}

inline uint32_t particle::LEDSystemTheme::color(LEDSignal signal) const {
    return d_.signals[signal].color;
}

inline void particle::LEDSystemTheme::setPattern(LEDSignal signal, LEDPattern pattern) {
    // System LED signaling doesn't support custom patterns
    d_.signals[signal].pattern = (pattern != LED_PATTERN_CUSTOM ? pattern : LED_PATTERN_SOLID);
}

inline LEDPattern particle::LEDSystemTheme::pattern(LEDSignal signal) const {
    return (LEDPattern)d_.signals[signal].pattern;
}

inline void particle::LEDSystemTheme::setSpeed(LEDSignal signal, LEDSpeed speed) {
    setPeriod(signal, led_pattern_period(d_.signals[signal].pattern, speed, nullptr));
}

inline void particle::LEDSystemTheme::setPeriod(LEDSignal signal, uint16_t period) {
    d_.signals[signal].period = period;
}

inline uint16_t particle::LEDSystemTheme::period(LEDSignal signal) const {
    return d_.signals[signal].period;
}

inline void particle::LEDSystemTheme::setSignal(LEDSignal signal, uint32_t color) {
    setColor(signal, color);
}

inline void particle::LEDSystemTheme::setSignal(LEDSignal signal, uint32_t color, LEDPattern pattern, LEDSpeed speed) {
    setSignal(signal, color, pattern, led_pattern_period(pattern, speed, nullptr));
}

inline void particle::LEDSystemTheme::apply(bool save) {
    led_set_signal_theme(&d_, (save ? LED_SIGNAL_FLAG_SAVE_THEME : 0), nullptr);
}

inline void particle::LEDSystemTheme::restoreDefault() {
    led_set_signal_theme(nullptr, LED_SIGNAL_FLAG_DEFAULT_THEME | LED_SIGNAL_FLAG_SAVE_THEME, nullptr);
}

#endif // SPARK_WIRING_LED_H
