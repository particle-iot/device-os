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

#include "led_service.h"

#include "rgbled_hal.h"
#include "rgbled.h"
#include "debug.h"

// TODO: Move synchronization macros to some header file
#if PLATFORM_ID != 3

#define INTERRUPTS_HAL_EXCLUDE_PLATFORM_HEADERS
#include "spark_wiring_interrupts.h"

#define LED_SERVICE_DECLARE_LOCK(name)
#define LED_SERVICE_WITH_LOCK(name) ATOMIC_BLOCK()

#else // PLATFORM_ID == 3

#if PLATFORM_THREADING

#include "spark_wiring_thread.h"

#define LED_SERVICE_DECLARE_LOCK(name) mutable RecursiveMutex name
#define LED_SERVICE_WITH_LOCK(name) WITH_LOCK(name)

#else // PLATFORM_ID == 3 && !PLATFORM_THREADING

#define LED_SERVICE_DECLARE_LOCK(name)
#define LED_SERVICE_WITH_LOCK(name)

#endif

#endif

namespace {

class StatusQueue {
public:
    StatusQueue() :
            front_(nullptr) {
    }

    void add(LEDStatusData* status) {
        SPARK_ASSERT(status);
        LEDStatusData* prev = nullptr;
        LEDStatusData* next = front_;
        while (next) {
            // Elements are sorted by priority in descending order
            if (next->priority <= status->priority) {
                break;
            }
            prev = next;
            next = next->next;
        }
        if (next) {
            next->prev = status;
        }
        if (prev) {
            prev->next = status;
        } else {
            front_ = status;
        }
        status->prev = prev;
        status->next = next;
    }

    void remove(LEDStatusData* status) {
        SPARK_ASSERT(status);
        if (status->next) {
            status->next->prev = status->prev;
        }
        if (status->prev) {
            status->prev->next = status->next;
        } else {
            front_ = status->next;
        }
    }

    LEDStatusData* front() const {
        return front_;
    }

    bool isEmpty() const {
        return front_ == nullptr;
    }

private:
    LEDStatusData* front_;
};

class LEDService {
public:
    LEDService() :
            color_{ 0 },
            pattern_(LED_PATTERN_INVALID),
            period_(0),
            ticks_(0),
            disabled_(0),
            reset_(true) {
    }

    void setStatusActive(LEDStatusData* status, bool active) {
        SPARK_ASSERT(status);
        LED_SERVICE_WITH_LOCK(lock_) {
            if (status->flags & LED_STATUS_FLAG_ACTIVE) {
                queue_.remove(status);
                status->flags &= ~LED_STATUS_FLAG_ACTIVE;
            }
            if (active) {
                queue_.add(status);
                status->flags |= LED_STATUS_FLAG_ACTIVE;
            } else if (queue_.isEmpty()) {
                // LED service doesn't reset the LED to any default color when there's no active status
                // available, so cached LED color should be ignored for a next status activated later
                reset_ = true;
            }
        }
    }

    void setUpdateEnabled(bool enabled) {
        LED_SERVICE_WITH_LOCK(lock_) {
            if (enabled) {
                --disabled_;
                if (disabled_ == 0) {
                    reset_ = true; // Ignore cached LED color
                }
            } else {
                ++disabled_;
            }
        }
    }

    bool isUpdateEnabled() const {
        bool enabled = false;
        LED_SERVICE_WITH_LOCK(lock_) {
            enabled = (disabled_ == 0);
        }
        return enabled;
    }

    void update(system_tick_t ticks) {
        uint32_t color = 0;
        uint16_t period = 0;
        uint8_t pattern = LED_PATTERN_INVALID;
        uint8_t flags = 0;
        bool enabled = false;
        bool reset = false;
        // TODO: Add some flag to avoid locking if the queue has not changed since last update
        LED_SERVICE_WITH_LOCK(lock_) {
            LEDStatusData* s = queue_.front();
            if (s) {
                // Copy status parameters
                pattern = s->pattern;
                if (pattern == LED_PATTERN_CUSTOM) { // Custom pattern
                    s->callback(ticks, s->data);
                } else { // Predefined pattern
                    period = s->period;
                }
                color = s->color;
                flags = s->flags;
                enabled = !disabled_;
                reset = reset_;
                reset_ = false;
            }
        }
        if (pattern_ != pattern || period_ != period) {
            pattern_ = pattern;
            period_ = period;
            ticks_ = 0; // Restart pattern "animation"
        } else if (period_ > 0) {
            ticks_ += ticks;
            if (ticks_ >= period_) {
                ticks_ %= period_;
            }
        }
        if (enabled) {
            Color c = { 0 }; // Black
            if (!(flags & LED_STATUS_FLAG_OFF)) {
                scaleColor(color, led_rgb_brightness, &c); // Use global LED brightness
                if (period_ > 0) {
                    updatePatternColor(pattern_, ticks_, period_, &c);
                }
            }
            if (reset || color_.r != c.r || color_.g != c.g || color_.b != c.b) {
                setLedColor(c);
                color_ = c;
            }
        }
    }

private:
    struct Color {
        uint16_t r, g, b; // Scaled color components
    };

    StatusQueue queue_; // Status queue

    Color color_; // Current LED color
    uint8_t pattern_; // Current pattern type
    uint16_t period_; // Current pattern period in milliseconds
    uint16_t ticks_; // Number of milliseconds passed within pattern period

    volatile uint16_t disabled_; // The service is allowed to change LED color only if this counter is set to 0
    volatile bool reset_; // Flag signaling that cached LED color should be ignored

    LED_SERVICE_DECLARE_LOCK(lock_); // Platform-specific lock

    // Updates color according to specified pattern and timing
    static void updatePatternColor(uint8_t pattern, uint16_t ticks, uint16_t period, Color* color) {
        switch (pattern) {
        case LED_PATTERN_BLINK: {
            if (ticks >= period / 2) { // Turn LED off
                color->r = 0;
                color->g = 0;
                color->b = 0;
            }
            break;
        }
        case LED_PATTERN_FADE: {
            period /= 2;
            if (ticks < period) { // Fade out
                ticks = period - ticks;
            } else { // Fade in
                ticks = ticks - period;
            }
            color->r = (uint32_t)color->r * ticks / period;
            color->g = (uint32_t)color->g * ticks / period;
            color->b = (uint32_t)color->b * ticks / period;
            break;
        }
        default:
            break;
        }
    }

    // Splits 32-bit RGB value into 16-bit color components (as expected by HAL) and applies
    // brightness correction
    static void scaleColor(uint32_t color, uint8_t value, Color* scaled) {
        const uint32_t v = (uint32_t)value * LED_Callbacks.Led_Rgb_Get_Max_Value(nullptr);
        scaled->r = (((color >> 16) & 0xff) * v) >> 16;
        scaled->g = (((color >> 8) & 0xff) * v) >> 16;
        scaled->b = ((color & 0xff) * v) >> 16;
    }

    // Sets LED color and invokes user callback
    static void setLedColor(const Color& color) {
        LED_Callbacks.Led_Rgb_Set_Values(color.r, color.g, color.b, nullptr);
        if (led_update_handler) {
            // User callback expects RGB values to be in 0 - 255 range
            const uint32_t v = LED_Callbacks.Led_Rgb_Get_Max_Value(nullptr);
            const uint8_t r = ((uint32_t)color.r << 8) / v;
            const uint8_t g = ((uint32_t)color.g << 8) / v;
            const uint8_t b = ((uint32_t)color.b << 8) / v;
            led_update_handler(led_update_handler_data, r, g, b, nullptr /* reserved */);
        }
    }
};

LEDService ledService;

} // namespace

void led_set_status_active(LEDStatusData* status, int active, void* reserved) {
    ledService.setStatusActive(status, active);
}

void led_set_update_enabled(int enabled, void* reserved) {
    ledService.setUpdateEnabled(enabled);
}

int led_update_enabled(void* reserved) {
    return (ledService.isUpdateEnabled() ? 1 : 0);
}

void led_update(system_tick_t ticks, LEDStatusData* status, void* reserved) {
    ledService.update(ticks);
}
