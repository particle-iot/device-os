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

#include "system_led_signal.h"

#include "dct.h"
#include "debug.h"

#include <algorithm>

// Helper macros for DEFAULT_THEME_DATA initialization
#define DEFAULT_COLOR(_r, _g, _b) \
        (uint8_t)_r, (uint8_t)_g, (uint8_t)_b

#define DEFAULT_PATTERN(_color, _pattern, _speed) \
        ((uint8_t)((((uint8_t)PatternIndex::_pattern & 0x03) << 6) | \
            (((uint8_t)SpeedIndex::_speed & 0x03) << 4) | \
            ((uint8_t)ColorIndex::_color & 0x0f)))

namespace {

enum class ColorIndex {
    GRAY = 0,
    BLUE = 1,
    GREEN = 2,
    CYAN = 3,
    RED = 4,
    MAGENTA = 5,
    YELLOW = 6,
    WHITE = 7,
    RESERVED = 15
};

enum class PatternIndex {
    SOLID = 0,
    BLINK = 1,
    FADE = 2,
    RESERVED = 3
};

enum class SpeedIndex {
    SLOW = 0,
    NORMAL = 1,
    FAST = 2,
    RESERVED = 3
};

// Serialized factory default theme
const uint8_t DEFAULT_THEME_DATA[] = {
    // Palette colors (in order of ColorIndex elements)
    DEFAULT_COLOR(31, 31, 31), // GRAY
    DEFAULT_COLOR(0, 0, 255), // BLUE
    DEFAULT_COLOR(0, 255, 0), // GREEN
    DEFAULT_COLOR(0, 255, 255), // CYAN
    DEFAULT_COLOR(255, 0, 0), // RED
    DEFAULT_COLOR(255, 0, 255), // MAGENTA
    DEFAULT_COLOR(255, 255, 0), // YELLOW
    DEFAULT_COLOR(255, 255, 255), // WHITE
    // Signal patterns (in order of LEDSignal elements)
    DEFAULT_PATTERN(WHITE, FADE, NORMAL), // LED_SIGNAL_NETWORK_OFF
    DEFAULT_PATTERN(BLUE, FADE, NORMAL), // LED_SIGNAL_NETWORK_ON
    DEFAULT_PATTERN(GREEN, BLINK, NORMAL), // LED_SIGNAL_NETWORK_CONNECTING
    DEFAULT_PATTERN(GREEN, BLINK, FAST), // LED_SIGNAL_NETWORK_DHCP
    DEFAULT_PATTERN(GREEN, FADE, NORMAL), // LED_SIGNAL_NETWORK_CONNECTED
    DEFAULT_PATTERN(CYAN, BLINK, NORMAL), // LED_SIGNAL_CLOUD_CONNECTING
    DEFAULT_PATTERN(CYAN, BLINK, FAST), // LED_SIGNAL_CLOUD_HANDSHAKE
    DEFAULT_PATTERN(CYAN, FADE, NORMAL), // LED_SIGNAL_CLOUD_CONNECTED
    DEFAULT_PATTERN(BLUE, BLINK, SLOW), // LED_SIGNAL_LISTENING_MODE
    DEFAULT_PATTERN(MAGENTA, FADE, NORMAL), // LED_SIGNAL_SAFE_MODE
    DEFAULT_PATTERN(YELLOW, BLINK, NORMAL), // LED_SIGNAL_DFU_MODE
    DEFAULT_PATTERN(MAGENTA, BLINK, FAST), // LED_SIGNAL_FIRMWARE_UPDATE
    DEFAULT_PATTERN(GRAY, SOLID, NORMAL) // LED_SIGNAL_POWER_OFF
};

// Size of a serialized theme in bytes
const size_t THEME_DATA_SIZE = 37; // 8 palette colors (3 bytes each) + 13 signal patterns (1 byte each)

static_assert(THEME_DATA_SIZE == LED_PALETTE_COLOR_COUNT * 3 + LED_SIGNAL_COUNT * 1,
        "Invalid THEME_DATA_SIZE");

static_assert(THEME_DATA_SIZE <= (DCT_LED_THEME_SIZE - 1), // 1 byte in DCT is reserved for theme version
        "THEME_DATA_SIZE is greater than size of LED theme section in DCT");

static_assert(sizeof(DEFAULT_THEME_DATA) == THEME_DATA_SIZE,
        "Size of DEFAULT_THEME_DATA has changed");

static_assert(LED_THEME_DATA_VERSION == 1,
        "LED_THEME_DATA_VERSION has changed");

class LEDSignalManager {
public:
    LEDSignalManager() {
        // Initialize status data
        initStatusData();
        // Set current theme
        LEDThemeData t = { LED_THEME_DATA_VERSION };
        deserializeTheme(t, currentThemeData(), THEME_DATA_SIZE);
        setTheme(t);
    }

    bool start(int signal, uint8_t priority) {
        LEDStatusData* const s = statusDataForSignal(signal);
        if (s) {
            s->priority = priority;
            led_set_status_active(s, true, nullptr);
            return true;
        }
        return false;
    }

    void stop(int signal) {
        LEDStatusData* const s = statusDataForSignal(signal);
        if (s) {
            led_set_status_active(s, false, nullptr);
        }
    }

    bool isStarted(int signal) {
        LEDStatusData* const s = statusDataForSignal(signal);
        if (s) {
            return s->flags & LED_STATUS_FLAG_ACTIVE;
        }
        return false;
    }

    bool setTheme(const LEDThemeData* theme, int flags) {
        if (flags & LED_THEME_FLAG_DEFAULT) {
            // Ignore 'theme' argument and set default theme
            LEDThemeData t = { LED_THEME_DATA_VERSION };
            deserializeTheme(t, (const char*)DEFAULT_THEME_DATA, THEME_DATA_SIZE);
            setTheme(t);
            // Reset theme version in DCT
            const uint8_t version = 0xff;
            dct_write_app_data(&version, DCT_LED_THEME_OFFSET, 1);
            return true;
        } else if (theme->version == LED_THEME_DATA_VERSION) {
            setTheme(*theme);
            // Write theme data to DCT
            char data[THEME_DATA_SIZE];
            serializeTheme(*theme, data, sizeof(data));
            const uint8_t version = LED_THEME_DATA_VERSION;
            dct_write_app_data(&version, DCT_LED_THEME_OFFSET, 1); // Write version
            dct_write_app_data(data, DCT_LED_THEME_OFFSET + 1, sizeof(data)); // Write theme data
            return true;
        }
        return false;
    }

    bool getTheme(LEDThemeData* theme, int flags) {
        if (theme->version == LED_THEME_DATA_VERSION) {
            if (flags & LED_THEME_FLAG_DEFAULT) {
                // Return default theme
                deserializeTheme(*theme, (const char*)DEFAULT_THEME_DATA, THEME_DATA_SIZE);
                return true;
            } else {
                // Return current theme
                deserializeTheme(*theme, currentThemeData(), THEME_DATA_SIZE);
                return true;
            }
        }
        return false;
    }

private:
    LEDStatusData statusData_[LED_SIGNAL_COUNT];

    void setTheme(const LEDThemeData& theme) {
        led_set_updates_enabled(0, nullptr); // Disable LED updates
        for (size_t i = 0; i < LED_SIGNAL_COUNT; ++i) {
            const LEDThemeSignalData& signal = theme.signals[i];
            LEDStatusData& status = statusData_[i];
            SPARK_ASSERT(signal.color < LED_PALETTE_COLOR_COUNT);
            status.color = theme.palette[signal.color];
            status.pattern = signal.pattern;
            status.speed = signal.speed;
        }
        led_set_updates_enabled(1, nullptr); // Enable LED updates
    }

    void initStatusData() {
        for (size_t i = 0; i < LED_SIGNAL_COUNT; ++i) {
            LEDStatusData& s = statusData_[i];
            s.size = sizeof(LEDStatusData);
            s.next = nullptr;
            s.prev = nullptr;
            s.color = 0;
            s.pattern = LED_PATTERN_INVALID;
            s.speed = LED_SPEED_INVALID;
            s.flags = 0;
            s.priority = 0;
        }
    }

    LEDStatusData* statusDataForSignal(int signal) {
        if (signal >= 0 && signal < LED_SIGNAL_COUNT) {
            return &statusData_[signal];
        }
        return nullptr;
    }

    static const char* currentThemeData() {
        const char* d = (const char*)dct_read_app_data(DCT_LED_THEME_OFFSET);
        if (!d || *d == 0xff) { // Check if theme data is initialized in DCT
            return (const char*)DEFAULT_THEME_DATA;
        }
        return d + 1; // First byte is reserved for theme version
    }

    static void serializeTheme(const LEDThemeData& theme, char* data, size_t size) {
        serializePalette(theme.palette, data, size);
        for (size_t i = 0; i < LED_SIGNAL_COUNT; ++i) {
            serializeThemeSignal(theme.signals[i], data, size);
        }
    }

    static void deserializeTheme(LEDThemeData& theme, const char* data, size_t size) {
        deserializePalette(theme.palette, data, size);
        for (size_t i = 0; i < LED_SIGNAL_COUNT; ++i) {
            deserializeThemeSignal(theme.signals[i], data, size);
        }
    }

    static void serializePalette(const uint32_t* palette, char*& data, size_t& size) {
        const size_t n = LED_PALETTE_COLOR_COUNT * 3; // 3 bytes per color
        if (size >= n) {
            for (size_t i = 0; i < LED_PALETTE_COLOR_COUNT; ++i) {
                const uint32_t c = palette[i];
                *(data++) = (c & 0x00ff0000) >> 16; // R
                *(data++) = (c & 0x0000ff00) >> 8; // G
                *(data++) = c & 0x000000ff; // B
            }
            size -= n;
        }
    }

    static void deserializePalette(uint32_t* palette, const char*& data, size_t& size) {
        const size_t n = LED_PALETTE_COLOR_COUNT * 3;
        if (size >= n) {
            for (size_t i = 0; i < LED_PALETTE_COLOR_COUNT; ++i) {
                const uint8_t r = *(data++);
                const uint8_t g = *(data++);
                const uint8_t b = *(data++);
                palette[i] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
            }
            size -= n;
        }
    }

    static void serializeThemeSignal(const LEDThemeSignalData& signal, char*& data, size_t& size) {
        if (size >= 1) {
            // Signal parameters are stored as a byte with the following layout: PPSSCCCCb,
            // where P, S and C are pattern, speed and color bits respectively
            const uint8_t pattern = patternToIndex(signal.pattern);
            const uint8_t speed = speedToIndex(signal.speed);
            *(data++) = ((pattern & 0x03) << 6) | ((speed & 0x03) << 4) | (signal.color & 0x0f);
            --size;
        }
    }

    static void deserializeThemeSignal(LEDThemeSignalData& signal, const char*& data, size_t& size) {
        if (size >= 1) {
            const uint8_t s = *(data++);
            signal.pattern = patternFromIndex((s >> 6) & 0x03);
            signal.speed = speedFromIndex((s >> 4) & 0x03);
            signal.color = s & 0x0f;
            --size;
        }
    }

    static uint8_t patternToIndex(uint8_t type) {
        switch (type) {
        case LED_PATTERN_SOLID:
            return (uint8_t)PatternIndex::SOLID;
        case LED_PATTERN_BLINK:
            return (uint8_t)PatternIndex::BLINK;
        case LED_PATTERN_FADE:
            return (uint8_t)PatternIndex::FADE;
        default:
            return (uint8_t)PatternIndex::RESERVED;
        }
    }

    static uint8_t patternFromIndex(uint8_t index) {
        switch (index) {
        case (uint8_t)PatternIndex::SOLID:
            return LED_PATTERN_SOLID;
        case (uint8_t)PatternIndex::BLINK:
            return LED_PATTERN_BLINK;
        case (uint8_t)PatternIndex::FADE:
            return LED_PATTERN_FADE;
        default:
            return LED_PATTERN_INVALID;
        }
    }

    static uint8_t speedToIndex(uint8_t speed) {
        if (speed == LED_SPEED_NORMAL) {
            return (uint8_t)SpeedIndex::NORMAL;
        } else if (speed < LED_SPEED_NORMAL) {
            return (uint8_t)SpeedIndex::SLOW;
        } else {
            return (uint8_t)SpeedIndex::FAST;
        }
    }

    static uint8_t speedFromIndex(uint8_t index) {
        switch (index) {
        case (uint8_t)SpeedIndex::SLOW:
            return LED_SPEED_SLOW;
        case (uint8_t)SpeedIndex::NORMAL:
            return LED_SPEED_NORMAL;
        case (uint8_t)SpeedIndex::FAST:
            return LED_SPEED_FAST;
        default:
            return LED_SPEED_INVALID;
        }
    }
};

LEDSignalManager ledSignalManager;

} // namespace

int led_start_signal(int signal, uint8_t priority, void* reserved) {
    return (ledSignalManager.start(signal, priority) ? 0 : 1);
}

void led_stop_signal(int signal, void* reserved) {
    ledSignalManager.stop(signal);
}

int led_is_signal_started(int signal, void* reserved) {
    return ledSignalManager.isStarted(signal);
}

int led_set_signal_theme(const LEDThemeData* theme, int flags, void* reserved) {
    return (ledSignalManager.setTheme(theme, flags) ? 0 : 1);
}

int led_get_signal_theme(LEDThemeData* theme, int flags, void* reserved) {
    return (ledSignalManager.getTheme(theme, flags) ? 0 : 1);
}
