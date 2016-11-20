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

#include "led_service.h"

namespace particle {

enum class LEDPattern {
    SOLID = LED_PATTERN_TYPE_SOLID,
    BLINK = LED_PATTERN_TYPE_BLINK,
    FADE = LED_PATTERN_TYPE_FADE
};

enum class LEDSpeed {
    SLOW = LED_PATTERN_SPEED_SLOW,
    NORMAL = LED_PATTERN_SPEED_NORMAL,
    FAST = LED_PATTERN_SPEED_FAST
};

enum class LEDPriority {
    BACKGROUND = 10,
    NORMAL = 20,
    IMPORTANT = 30,
    CRITICAL = 40
};

enum class LEDSource {
    APPLICATION = 1,
    SYSTEM = 2,
#ifdef PARTICLE_USER_MODULE
    DEFAULT = APPLICATION
#else
    DEFAULT = SYSTEM
#endif
};

class LEDStatus {
public:
    explicit LEDStatus(uint32_t color, LEDPattern pattern = LEDPattern::SOLID, LEDSpeed speed = LEDSpeed::NORMAL);
    LEDStatus(uint32_t color, LEDPriority priority, LEDPattern pattern = LEDPattern::SOLID, LEDSpeed speed = LEDSpeed::NORMAL);
    LEDStatus(uint32_t color, LEDSource source, LEDPattern pattern = LEDPattern::SOLID, LEDSpeed speed = LEDSpeed::NORMAL);
    LEDStatus(uint32_t color, LEDSource source, LEDPriority priority, LEDPattern pattern = LEDPattern::SOLID, LEDSpeed speed = LEDSpeed::NORMAL);
    ~LEDStatus();

    void setColor(uint32_t color);
    uint32_t color() const;

    void setBrightness(uint8_t value);
    uint8_t brightness() const;

    void setPattern(LEDPattern pattern);
    LEDPattern pattern() const;

    void setSpeed(LEDSpeed speed);
    LEDSpeed speed() const;

    void on();
    void off();
    void toggle();

    void setActive(LEDPriority priority); // Activate and override priority
    void setActive(bool active = true);
    bool isActive() const;

    LEDPriority priority() const;

private:
    LEDStatusData status_;
};

} // namespace particle

inline particle::LEDStatus::LEDStatus(uint32_t color, LEDPattern pattern, LEDSpeed speed) :
        LEDStatus(color, LEDSource::DEFAULT, LEDPriority::NORMAL, pattern, speed) {
}

inline particle::LEDStatus::LEDStatus(uint32_t color, LEDPriority priority, LEDPattern pattern, LEDSpeed speed) :
        LEDStatus(color, LEDSource::DEFAULT, priority, pattern, speed) {
}

inline particle::LEDStatus::LEDStatus(uint32_t color, LEDSource source, LEDPattern pattern, LEDSpeed speed) :
        LEDStatus(color, source, LEDPriority::NORMAL, pattern, speed) {
}

inline particle::LEDStatus::LEDStatus(uint32_t color, LEDSource source, LEDPriority priority, LEDPattern pattern, LEDSpeed speed) :
        status_{ sizeof(LEDStatusData), nullptr /* Internal */, nullptr /* Internal */, color, 0xff /* Brightness */,
            (uint8_t)pattern, (uint8_t)speed, 0 /* Flags */, (uint8_t)(((uint8_t)priority << 2) | (uint8_t)source) /* Priority */ } {
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

inline void particle::LEDStatus::setBrightness(uint8_t value) {
    status_.value = value;
}

inline uint8_t particle::LEDStatus::brightness() const {
    return status_.value;
}

inline void particle::LEDStatus::setPattern(LEDPattern pattern) {
    status_.pattern = (uint8_t)pattern;
}

inline particle::LEDPattern particle::LEDStatus::pattern() const {
    return (LEDPattern)status_.pattern;
}

inline void particle::LEDStatus::setSpeed(LEDSpeed speed) {
    status_.speed = (uint8_t)speed;
}

inline particle::LEDSpeed particle::LEDStatus::speed() const {
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

inline particle::LEDPriority particle::LEDStatus::priority() const {
    return (LEDPriority)(status_.priority >> 2);
}

#endif // SPARK_WIRING_LED_H
