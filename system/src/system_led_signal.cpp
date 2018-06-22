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

#include "hal_platform.h"
#include "debug.h"

#if HAL_PLATFORM_DCT
#include "dct.h"
#endif

#include <algorithm>

/*
    In serialized form, parameters of each signal are stored as a 3 byte sequence with the
    following layout:

    0              |1              |2
    7 6 5 4|3 2 1 0|7 6 5 4|3 2 1 0|7 6 5 4 3 2 1 0
    r      |g      |b      |pattern|period

    r, g, b - RGB color components (4 bits per component);
    pattern - pattern type, as defined by LEDPattern enum;
    period - pattern period in 50ms units.

    NOTE: Part of the LED theme data is also processed in bootloader. Ensure implementation
    there is updated accordingly.
*/

// Helper macro for DEFAULT_THEME_DATA initialization
#define DEFAULT_SIGNAL_DATA(_color, _pattern, _speed) \
        ((RGB_COLOR_##_color >> 16) & 0xf0) | (((RGB_COLOR_##_color >> 8) & 0xf0) >> 4), /* R, G */ \
        (RGB_COLOR_##_color & 0xf0) | (LED_PATTERN_##_pattern & 0x0f), /* B, Pattern type */ \
        (uint16_t)PatternPeriod::_pattern##_##_speed / 50 /* Pattern period */

namespace {

// Predefined pattern periods
enum class PatternPeriod {
    SOLID_NORMAL = 0,
    BLINK_SLOW = 500,
    BLINK_NORMAL = 200,
    BLINK_FAST = 100,
    FADE_SLOW = 8000,
    FADE_NORMAL = 4000,
    FADE_FAST = 1000
};

// Serialized factory default theme
const uint8_t DEFAULT_THEME_DATA[] = {
    DEFAULT_SIGNAL_DATA(WHITE, FADE, NORMAL), // LED_SIGNAL_NETWORK_OFF
    DEFAULT_SIGNAL_DATA(BLUE, FADE, NORMAL), // LED_SIGNAL_NETWORK_ON
    DEFAULT_SIGNAL_DATA(GREEN, BLINK, NORMAL), // LED_SIGNAL_NETWORK_CONNECTING
    DEFAULT_SIGNAL_DATA(GREEN, BLINK, FAST), // LED_SIGNAL_NETWORK_DHCP
    DEFAULT_SIGNAL_DATA(GREEN, FADE, NORMAL), // LED_SIGNAL_NETWORK_CONNECTED
    DEFAULT_SIGNAL_DATA(CYAN, BLINK, NORMAL), // LED_SIGNAL_CLOUD_CONNECTING
    DEFAULT_SIGNAL_DATA(CYAN, BLINK, FAST), // LED_SIGNAL_CLOUD_HANDSHAKE
    DEFAULT_SIGNAL_DATA(CYAN, FADE, NORMAL), // LED_SIGNAL_CLOUD_CONNECTED
    DEFAULT_SIGNAL_DATA(MAGENTA, FADE, NORMAL), // LED_SIGNAL_SAFE_MODE
    DEFAULT_SIGNAL_DATA(BLUE, BLINK, SLOW), // LED_SIGNAL_LISTENING_MODE
    DEFAULT_SIGNAL_DATA(YELLOW, BLINK, NORMAL), // LED_SIGNAL_DFU_MODE
    DEFAULT_SIGNAL_DATA(MAGENTA, BLINK, FAST), // LED_SIGNAL_FIRMWARE_UPDATE
    DEFAULT_SIGNAL_DATA(GRAY, SOLID, NORMAL) // LED_SIGNAL_POWER_OFF
};

// Size of a serialized theme in bytes
const size_t THEME_DATA_SIZE = 39; // 13 signals, 3 bytes per signal

static_assert(THEME_DATA_SIZE == LED_SIGNAL_COUNT * 3,
        "Invalid THEME_DATA_SIZE");

static_assert(sizeof(DEFAULT_THEME_DATA) == THEME_DATA_SIZE,
        "Size of DEFAULT_THEME_DATA has changed");

static_assert(LED_SIGNAL_THEME_VERSION == 1,
        "LED_SIGNAL_THEME_VERSION has changed");

#if HAL_PLATFORM_DCT
static_assert(THEME_DATA_SIZE <= (DCT_LED_THEME_SIZE - 1), // 1 byte in DCT is reserved for version number
        "THEME_DATA_SIZE is greater than size of LED theme section in DCT");
#endif

// Returns pattern period in milliseconds for a predefined speed value
uint16_t patternPeriod(int pattern, int speed) {
    switch (pattern) {
    case LED_PATTERN_BLINK:
        // Blinking LED
        if (speed == LED_SPEED_NORMAL) {
            return (uint16_t)PatternPeriod::BLINK_NORMAL;
        } else if (speed > LED_SPEED_NORMAL) {
            return (uint16_t)PatternPeriod::BLINK_FAST;
        } else {
            return (uint16_t)PatternPeriod::BLINK_SLOW;
        }
    case LED_PATTERN_FADE:
        // Breathing LED
        if (speed == LED_SPEED_NORMAL) {
            return (uint16_t)PatternPeriod::FADE_NORMAL;
        } else if (speed > LED_SPEED_NORMAL) {
            return (uint16_t)PatternPeriod::FADE_FAST;
        } else {
            return (uint16_t)PatternPeriod::FADE_SLOW;
        }
    default:
        return 0; // Not applicable
    }
}

class LEDSignalManager {
public:
    LEDSignalManager() {
        // Initialize status data
        initStatusData();
        // Set current theme
        LEDSignalThemeData t = { LED_SIGNAL_THEME_VERSION };
#if HAL_PLATFORM_DCT
        char theme[DCT_LED_THEME_SIZE];
#else
        char theme[THEME_DATA_SIZE];
#endif /* HAL_PLATFORM_DCT */

        deserializeTheme(t, currentThemeData(theme, sizeof(theme)), THEME_DATA_SIZE);
        setTheme(t);
    }

    bool start(int signal, uint8_t priority, int flags) {
        return startSignal(signal, priority);
    }

    void stop(int signal, int flags) {
        if (flags & LED_SIGNAL_FLAG_ALL_SIGNALS) {
            // Ignore `signal` argument and stop all signals
            for (int i = 0; i < LED_SIGNAL_COUNT; ++i) {
                stopSignal(i);
            }
        } else {
            stopSignal(signal);
        }
    }

    bool isStarted(int signal) {
        LEDStatusData* const s = signalStatus(signal);
        if (s) {
            return s->flags & LED_STATUS_FLAG_ACTIVE;
        }
        return false;
    }

    bool setTheme(const LEDSignalThemeData* theme, int flags) {
        if (flags & LED_SIGNAL_FLAG_DEFAULT_THEME) {
            // Ignore `theme` argument and set factory default theme
            LEDSignalThemeData t = { LED_SIGNAL_THEME_VERSION };
            deserializeTheme(t, (const char*)DEFAULT_THEME_DATA, THEME_DATA_SIZE);
            setTheme(t);
#if HAL_PLATFORM_DCT
            if (flags & LED_SIGNAL_FLAG_SAVE_THEME) {
                // Reset theme version in DCT
                const uint8_t version = 0xff;
                dct_write_app_data(&version, DCT_LED_THEME_OFFSET, 1);
            }
#endif
            return true;
        } else if (theme->version == LED_SIGNAL_THEME_VERSION) {
            setTheme(*theme);
#if HAL_PLATFORM_DCT
            if (flags & LED_SIGNAL_FLAG_SAVE_THEME) {
                // Serialize theme data and write it to DCT
                char data[DCT_LED_THEME_SIZE];
                serializeTheme(*theme, data + 1, sizeof(data) - 1);
                data[0] = LED_SIGNAL_THEME_VERSION;
                dct_write_app_data(data, DCT_LED_THEME_OFFSET, sizeof(data)); // Write theme data
            }
#endif
            return true;
        }
        return false;
    }

    bool getTheme(LEDSignalThemeData* theme, int flags) const {
        if (theme->version == LED_SIGNAL_THEME_VERSION) {
            if (flags & LED_SIGNAL_FLAG_DEFAULT_THEME) {
                // Return factory default theme
                deserializeTheme(*theme, (const char*)DEFAULT_THEME_DATA, THEME_DATA_SIZE);
                return true;
            } else {
                // Return current theme
                getTheme(*theme);
                return true;
            }
        }
        return false;
    }

    LEDStatusData* signalStatus(int signal) {
        if (signal >= 0 && signal < LED_SIGNAL_COUNT) {
            return &statusData_[signal];
        }
        return nullptr;
    }

private:
    LEDStatusData statusData_[LED_SIGNAL_COUNT];

    void initStatusData() {
        for (size_t i = 0; i < LED_SIGNAL_COUNT; ++i) {
            LEDStatusData& s = statusData_[i];
            s.size = sizeof(LEDStatusData);
            s.next = nullptr;
            s.prev = nullptr;
            s.priority = 0;
            s.pattern = LED_PATTERN_INVALID;
            s.flags = 0;
            s.color = 0;
            s.period = 0;
        }
    }

    bool startSignal(int signal, uint8_t priority) {
        LEDStatusData* const s = signalStatus(signal);
        if (s) {
            s->priority = priority;
            led_set_status_active(s, 1, nullptr);
            return true;
        }
        return false;
    }

    bool stopSignal(int signal) {
        LEDStatusData* const s = signalStatus(signal);
        if (s) {
            led_set_status_active(s, 0, nullptr);
            return true;
        }
        return false;
    }

    void setTheme(const LEDSignalThemeData& theme) {
        led_set_update_enabled(0, nullptr); // Disable background LED updates
        for (size_t i = 0; i < LED_SIGNAL_COUNT; ++i) {
            const auto& signal = theme.signals[i];
            LEDStatusData& status = statusData_[i];
            status.color = signal.color;
            status.pattern = signal.pattern;
            status.period = signal.period;
        }
        led_set_update_enabled(1, nullptr); // Enable background LED updates
    }

    void getTheme(LEDSignalThemeData& theme) const {
        for (size_t i = 0; i < LED_SIGNAL_COUNT; ++i) {
            const LEDStatusData& status = statusData_[i];
            auto& signal = theme.signals[i];
            signal.color = status.color;
            signal.pattern = status.pattern;
            signal.period = status.period;
        }
    }

    static char* currentThemeData(char* buffer, size_t size) {
#if HAL_PLATFORM_DCT
        int r = dct_read_app_data_copy(DCT_LED_THEME_OFFSET, buffer, size);
        if (r || buffer[0] != LED_SIGNAL_THEME_VERSION) { // Check if theme data is initialized in DCT
            memcpy(buffer, DEFAULT_THEME_DATA, THEME_DATA_SIZE);
        } else {
            // First byte is reserved for version number
            ++buffer;
        }
#else
        memcpy(buffer, DEFAULT_THEME_DATA, size);
#endif
        return buffer;
    }

    static void serializeTheme(const LEDSignalThemeData& theme, char* data, size_t size) {
        SPARK_ASSERT(size >= LED_SIGNAL_COUNT * 3); // 3 bytes per signal
        for (size_t i = 0; i < LED_SIGNAL_COUNT; ++i) {
            const auto& s = theme.signals[i];
            *(data++) = (packColorComponent((s.color >> 16) & 0xff) << 4) | packColorComponent((s.color >> 8) & 0xff); // R, G
            *(data++) = (packColorComponent(s.color & 0xff) << 4) | (s.pattern & 0x0f); // B, Pattern type
            *(data++) = s.period / 50; // Pattern period
        }
    }

    static void deserializeTheme(LEDSignalThemeData& theme, const char* data, size_t size) {
        SPARK_ASSERT(size >= LED_SIGNAL_COUNT * 3); // 3 bytes per signal
        for (size_t i = 0; i < LED_SIGNAL_COUNT; ++i) {
            auto& s = theme.signals[i];
            uint8_t d = *(data++);
            s.color = ((uint32_t)unpackColorComponent((d & 0xf0) >> 4) << 16) | ((uint32_t)unpackColorComponent(d & 0x0f) << 8); // R, G
            d = *(data++);
            s.color |= (uint32_t)unpackColorComponent((d & 0xf0) >> 4); // B
            s.pattern = d & 0x0f; // Pattern type
            d = *(data++);
            s.period = (uint16_t)d * 50; // Pattern period
        }
    }

    static uint8_t packColorComponent(uint8_t value) {
        return value >> 4;
    }

    static uint8_t unpackColorComponent(uint8_t value) {
        return (value << 4) | (value & 0x0f); // 0 -> 0, 1 -> 17, 2 -> 34, ..., 15 -> 255
    }
};

LEDSignalManager ledSignalManager;

} // namespace

int led_start_signal(int signal, uint8_t priority, int flags, void* reserved) {
    return (ledSignalManager.start(signal, priority, flags) ? 0 : 1);
}

void led_stop_signal(int signal, int flags, void* reserved) {
    ledSignalManager.stop(signal, flags);
}

int led_signal_started(int signal, void* reserved) {
    return (ledSignalManager.isStarted(signal) ? 1 : 0);
}

int led_set_signal_theme(const LEDSignalThemeData* theme, int flags, void* reserved) {
    return (ledSignalManager.setTheme(theme, flags) ? 0 : 1);
}

int led_get_signal_theme(LEDSignalThemeData* theme, int flags, void* reserved) {
    return (ledSignalManager.getTheme(theme, flags) ? 0 : 1);
}

const LEDStatusData* led_signal_status(int signal, void* reserved) {
    return ledSignalManager.signalStatus(signal);
}

uint16_t led_pattern_period(int pattern, int speed, void* reserved) {
    return patternPeriod(pattern, speed);
}
