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

// Predefined pattern speed values
enum LEDSpeed {
    LED_SPEED_SLOW,
    LED_SPEED_NORMAL,
    LED_SPEED_FAST
};

class LEDStatus;
class LEDCustomStatus;

namespace detail {

// Returns pattern period for predefined speed value
uint16_t patternPeriod(LEDPattern pattern, LEDSpeed speed);

// Base class implementing common functionality of the LEDStatus and LEDCustomStatus classes
class LEDStatusBase {
public:
    virtual ~LEDStatusBase();

    void setColor(uint32_t color);
    uint32_t color() const;

    void on();
    void off();
    void toggle();
    bool isOn();
    bool isOff();

    void setActive(LEDPriority priority); // Activate and override priority
    void setActive(bool active = true);
    bool isActive() const;

    LEDPriority priority() const;

private:
    LEDStatusData d_;

    friend class particle::LEDStatus;
    friend class particle::LEDCustomStatus;
};

} // namespace particle::detail

// LED status using one of the predefined patterns for signaling
class LEDStatus: public detail::LEDStatusBase {
public:
    explicit LEDStatus(LEDPriority priority = LED_PRIORITY_NORMAL, LEDSource source = LED_SOURCE_DEFAULT);
    explicit LEDStatus(uint32_t color, LEDPriority priority = LED_PRIORITY_NORMAL, LEDSource source = LED_SOURCE_DEFAULT);
    LEDStatus(uint32_t color, LEDPattern pattern, LEDPriority priority = LED_PRIORITY_NORMAL, LEDSource source = LED_SOURCE_DEFAULT);
    LEDStatus(uint32_t color, LEDPattern pattern, LEDSpeed speed, LEDPriority priority = LED_PRIORITY_NORMAL, LEDSource source = LED_SOURCE_DEFAULT);
    LEDStatus(uint32_t color, LEDPattern pattern, uint16_t period, LEDPriority priority = LED_PRIORITY_NORMAL, LEDSource source = LED_SOURCE_DEFAULT);

    void setPattern(LEDPattern pattern);
    LEDPattern pattern() const;

    void setSpeed(LEDSpeed speed);
    void setPeriod(uint16_t period);
    uint16_t period() const;
};

// LED status implementing a custom pattern for signaling
class LEDCustomStatus: public detail::LEDStatusBase {
public:
    explicit LEDCustomStatus(LEDPriority priority = LED_PRIORITY_NORMAL, LEDSource source = LED_SOURCE_DEFAULT);
    explicit LEDCustomStatus(uint32_t color, LEDPriority priority = LED_PRIORITY_NORMAL, LEDSource source = LED_SOURCE_DEFAULT);

protected:
    // This method needs to be overridden to implement custom signaling pattern. `ticks` argument
    // contains number of milliseconds passed since previous update.
    //
    // NOTE: The system may call this method within an ISR. Ensure provided implementation doesn't
    // make any blocking calls, returns as quickly as possible, and, ideally, only updates internal
    // timing and makes calls to setColor(), setActive() and other methods of this class.
    virtual void update(system_tick_t ticks);

private:
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

    // Sets this theme as current theme used by the system. If `save` argument is set to `true`,
    // the theme will be saved to persistent storage and used as a default theme after reboot
    void apply(bool save = false);

    // Restores factory default theme
    static void restoreDefault();

private:
    LEDSignalThemeData d_;
};

} // namespace particle

// particle::detail::LEDStatusBase
inline particle::detail::LEDStatusBase::~LEDStatusBase() {
    setActive(false);
}

inline void particle::detail::LEDStatusBase::setColor(uint32_t color) {
    d_.color = color;
}

inline uint32_t particle::detail::LEDStatusBase::color() const {
    return d_.color;
}

inline void particle::detail::LEDStatusBase::on() {
    d_.flags &= ~LED_STATUS_FLAG_OFF;
}

inline void particle::detail::LEDStatusBase::off() {
    d_.flags |= LED_STATUS_FLAG_OFF;
}

inline void particle::detail::LEDStatusBase::toggle() {
    d_.flags ^= LED_STATUS_FLAG_OFF;
}

inline bool particle::detail::LEDStatusBase::isOn() {
    return !(d_.flags & LED_STATUS_FLAG_OFF);
}

inline bool particle::detail::LEDStatusBase::isOff() {
    return d_.flags & LED_STATUS_FLAG_OFF;
}

inline void particle::detail::LEDStatusBase::setActive(LEDPriority priority) {
    d_.priority = (d_.priority & 0x03) | ((uint8_t)priority << 2);
    setActive(true);
}

inline void particle::detail::LEDStatusBase::setActive(bool active) {
    led_set_status_active(&d_, active, nullptr);
}

inline bool particle::detail::LEDStatusBase::isActive() const {
    return d_.flags & LED_STATUS_FLAG_ACTIVE;
}

inline LEDPriority particle::detail::LEDStatusBase::priority() const {
    return (LEDPriority)(d_.priority >> 2);
}

// particle::LEDStatus
inline particle::LEDStatus::LEDStatus(LEDPriority priority, LEDSource source) :
        LEDStatus(RGB_COLOR_WHITE, priority, source) { // Use white color by default
}

inline particle::LEDStatus::LEDStatus(uint32_t color, LEDPriority priority, LEDSource source) :
        LEDStatus(color, LED_PATTERN_SOLID, priority, source) { // Use solid pattern by default
}

inline particle::LEDStatus::LEDStatus(uint32_t color, LEDPattern pattern, LEDPriority priority, LEDSource source) :
        LEDStatus(color, pattern, LED_SPEED_NORMAL, priority, source) { // Use normal speed by default
}

inline particle::LEDStatus::LEDStatus(uint32_t color, LEDPattern pattern, LEDSpeed speed, LEDPriority priority, LEDSource source) :
        LEDStatus(color, pattern, detail::patternPeriod(pattern, speed), priority, source) {
}

inline void particle::LEDStatus::setPattern(LEDPattern pattern) {
    d_.pattern = (pattern == LED_PATTERN_CUSTOM ? LED_PATTERN_SOLID : pattern); // Sanity check
}

inline LEDPattern particle::LEDStatus::pattern() const {
    return (LEDPattern)d_.pattern;
}

inline void particle::LEDStatus::setSpeed(LEDSpeed speed) {
    d_.period = detail::patternPeriod((LEDPattern)d_.pattern, speed);
}

inline void particle::LEDStatus::setPeriod(uint16_t period) {
    d_.period = period;
}

inline uint16_t particle::LEDStatus::period() const {
    return d_.period;
}

// particle::LEDCustomStatus
inline particle::LEDCustomStatus::LEDCustomStatus(LEDPriority priority, LEDSource source) :
        LEDCustomStatus(RGB_COLOR_WHITE, priority, source) { // Use white color by default
}

inline void particle::LEDCustomStatus::update(system_tick_t ticks) {
    // Default implementation does nothing
}

inline void particle::LEDCustomStatus::updateCallback(system_tick_t ticks, void* data) {
    LEDCustomStatus* s = static_cast<LEDCustomStatus*>(data);
    s->update(ticks);
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
    d_.signals[signal].pattern = (pattern == LED_PATTERN_CUSTOM ? LED_PATTERN_SOLID : pattern); // Sanity check
}

inline LEDPattern particle::LEDSystemTheme::pattern(LEDSignal signal) const {
    return (LEDPattern)d_.signals[signal].pattern;
}

inline void particle::LEDSystemTheme::setSpeed(LEDSignal signal, LEDSpeed speed) {
    auto& s = d_.signals[signal];
    s.period = detail::patternPeriod((LEDPattern)s.pattern, speed);
}

inline void particle::LEDSystemTheme::setPeriod(LEDSignal signal, uint16_t period) {
    d_.signals[signal].period = period;
}

inline uint16_t particle::LEDSystemTheme::period(LEDSignal signal) const {
    return d_.signals[signal].period;
}

inline void particle::LEDSystemTheme::apply(bool save) {
    led_set_signal_theme(&d_, (save ? LED_SIGNAL_THEME_FLAG_SAVE : 0), nullptr);
}

inline void particle::LEDSystemTheme::restoreDefault() {
    led_set_signal_theme(nullptr, LED_SIGNAL_THEME_FLAG_DEFAULT | LED_SIGNAL_THEME_FLAG_SAVE, nullptr);
}

#endif // SPARK_WIRING_LED_H
