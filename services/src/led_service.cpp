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

#include "debug.h"

#if PLATFORM_ID != 3

#include "spark_wiring_interrupts.h"

#define LED_SERVICE_DECLARE_LOCK(name)
#define LED_SERVICE_WITH_LOCK(name) ATOMIC_BLOCK()

#else // PLATFORM_ID == 3

#include <iostream> // FIXME

#if PLATFORM_THREADING

#include "spark_wiring_thread.h"

#define LED_SERVICE_DECLARE_LOCK(name) Mutex name
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

    void add(LEDStatus* status) {
        status->prev = nullptr;
        status->next = front_;
        front_ = status;
    }

    void remove(LEDStatus* status) {
        for (LEDStatus* s = front_; s != nullptr; ++s) {
            if (s == status) {
                if (s->prev) {
                    s->prev->next = s->next;
                } else {
                    front_ = s->next;
                }
                if (s->next) {
                    s->next->prev = s->prev;
                }
                break;
            }
        }
    }

    LEDStatus* front() const {
        return front_;
    }

private:
    LEDStatus* front_;
};

class LEDService {
public:
    LEDService() :
            ticks_(0),
            disabled_(0),
            reset_(true),
            currentColor_(0),
            currentPattern_(0),
            currentSpeed_(0) {
    }

    void addStatus(LEDStatus* status, int priority) {
        const int oldPriority = status->priority;
        status->priority = priority;
        LED_SERVICE_WITH_LOCK(lock_) {
            statusQueue(status->source, oldPriority).remove(status);
            statusQueue(status->source, priority).add(status);
        }
    }

    void removeStatus(LEDStatus* status) {
        LED_SERVICE_WITH_LOCK(lock_) {
            statusQueue(status->source, status->priority).remove(status);
        }
    }

    void startSignal(int signal, int priority) {
    }

    void stopSignal(int signal) {
    }

    void setTheme(const LEDTheme* theme, int flags) {
    }

    void getTheme(LEDTheme* theme) {
    }

    void enableUpdates() {
        LED_SERVICE_WITH_LOCK(lock_) {
            --disabled_;
            if (disabled_ == 0) {
                reset_ = true;
            }
        }
    }

    void disableUpdates() {
        LED_SERVICE_WITH_LOCK(lock_) {
            ++disabled_;
        }
    }

    void update(system_tick_t ticks) {
        uint32_t color = 0;
        uint8_t pattern = 0;
        // uint8_t speed = 0;
        bool enabled = false;
        bool reset = false;
        bool off = false;
        LED_SERVICE_WITH_LOCK(lock_) {
            LEDStatus* s = activeStatus();
            if (s) {
                color = s->pattern.color;
                pattern = s->pattern.type;
                // speed = s->pattern.speed;
                off = s->flags & LED_STATUS_FLAG_OFF;
            } else {
                off = true;
            }
            enabled = (disabled_ == 0);
            reset = reset_;
            reset_ = false;
        }
        if (pattern != currentPattern_) {
            currentPattern_ = pattern;
            reset = true;
        }
        if (off) {
            color = 0xff000000;
        }
        if (currentColor_ != color || reset) {
            if (enabled) {
                setColor(color);
            }
            currentColor_ = color;
        }
    }

    // FIXME
    void dump() {
        LEDStatus* s = activeStatus();
        if (s) {
            uint32_t c = s->pattern.color;
            int r = (c >> 16) & 0xff;
            int g = (c >> 8) & 0xff;
            int b = c & 0xff;
            std::cout << "r: " << r << ", g: " << g << ", b: " << b << std::endl;
            setColor(c);
        } else {
            std::cout << "no status" << std::endl;
        }
    }

    static LEDService* instance() {
        static LEDService service;
        return &service;
    }

private:
    static const int PRIORITY_COUNT = 4;

    StatusQueue sysStats_[PRIORITY_COUNT];
    StatusQueue appStats_[PRIORITY_COUNT];

    system_tick_t ticks_;
    int disabled_;
    bool reset_;

    uint32_t currentColor_;
    uint8_t currentPattern_;
    uint8_t currentSpeed_;

    LED_SERVICE_DECLARE_LOCK(lock_);

    LEDStatus* activeStatus() const {
        LEDStatus* const sysStat = activeStatus(sysStats_); // System status
        LEDStatus* const appStat = activeStatus(appStats_); // Application status
        LEDStatus* s = nullptr; // Active status
        if (sysStat) {
            if (appStat) {
                // System status always overrides application status with same priority
                s = (sysStat->priority >= appStat->priority) ? sysStat : appStat;
            } else {
                s = sysStat;
            }
        } else if (appStat) {
            s = appStat;
        }
        return s;
    }

    static LEDStatus* activeStatus(const StatusQueue stats[]) {
        for (int i = PRIORITY_COUNT - 1; i >= 0; --i) { // Starting from highest priority
            LEDStatus* s = stats[i].front();
            if (s) {
                return s;
            }
        }
        return nullptr;
    }

    // Returns status queue for specified source and priority
    StatusQueue& statusQueue(int source, int priority) {
        SPARK_ASSERT((source == LED_SOURCE_SYSTEM || source == LED_SOURCE_APPLICATION) &&
                (priority == LED_PRIORITY_BACKGROUND || priority == LED_PRIORITY_NORMAL ||
                priority == LED_PRIORITY_IMPORTANT || priority == LED_PRIORITY_CRITICAL));
        return (source == LED_SOURCE_SYSTEM) ? sysStats_[priority] : appStats_[priority];
    }

    static void setColor(uint32_t color) {
        const uint32_t k = (((color >> 24) & 0xff) + 1) /* alpha */ * Get_RGB_LED_Max_Value();
        const uint16_t r = (((color >> 16) & 0xff) * k) >> 16;
        const uint16_t g = (((color >> 8) & 0xff) * k) >> 16;
        const uint16_t b = ((color & 0xff) * k) >> 16;
        Set_RGB_LED_Values(r, g, b);
    }
};

} // namespace

void led_status_start(LEDStatus* status, int priority, void* reserved) {
    LEDService::instance()->addStatus(status, priority);
}

void led_status_stop(LEDStatus* status, void* reserved) {
    LEDService::instance()->removeStatus(status);
}

void led_signal_start(int signal, int priority, void* reserved) {
    LEDService::instance()->startSignal(signal, priority);
}

void led_signal_stop(int signal, void* reserved) {
    LEDService::instance()->stopSignal(signal);
}

void led_theme_set(const LEDTheme* theme, int flags, void* reserved) {
    LEDService::instance()->setTheme(theme, flags);
}

void led_theme_get(LEDTheme* theme, void* reserved) {
    LEDService::instance()->getTheme(theme);
}

void led_update_enable(void* reserved) {
    LEDService::instance()->enableUpdates();
}

void led_update_disable(void* reserved) {
    LEDService::instance()->disableUpdates();
}

void led_update(system_tick_t ticks, void* reserved) {
    LEDService::instance()->update(ticks);
}

void led_dump() {
    LEDService::instance()->dump();
}
