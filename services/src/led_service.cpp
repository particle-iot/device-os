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

class StateQueue {
public:
    StateQueue() :
            front_(nullptr) {
    }

    void add(LEDState* state) {
        state->prev = nullptr;
        state->next = front_;
        front_ = state;
    }

    void remove(LEDState* state) {
        for (LEDState* s = front_; s != nullptr; ++s) {
            if (s == state) {
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

    LEDState* front() const {
        return front_;
    }

private:
    LEDState* front_;
};

// TODO: Move to header file for testing
class LEDService {
public:
    LEDService() :
            lastPatternType_(0) { // Invalid pattern type
    }

    void enterState(LEDState* state, int source, int priority) {
        const int oldPriority = state->priority;
        state->priority = priority;
        state->source = source;
        LED_SERVICE_WITH_LOCK(lock_) {
            stateQueue(source, oldPriority).remove(state);
            stateQueue(source, priority).add(state);
        }
    }

    void leaveState(LEDState* state) {
        LED_SERVICE_WITH_LOCK(lock_) {
            stateQueue(state->source, state->priority).remove(state);
        }
    }

    void startSignal(int signal, int priority) {
    }

    void stopSignal(int signal) {
    }

    void setSignalTheme(const LEDSignalTheme* theme, int flags) {
    }

    void getSignalTheme(LEDSignalTheme* theme) {
    }

    void enableUpdates() {
    }

    void disableUpdates() {
    }

    void update(system_tick_t time) {
        LEDPattern p;
        LED_SERVICE_WITH_LOCK(lock_) {
            if (!getActivePattern(&p)) {
                return;
            }
        }
        if (p.type != lastPatternType_) {
            // Reset pattern state
            lastUpdateTime_ = 0;
        }
    }

private:
    static const int PRIORITY_COUNT = 4;

    StateQueue sysStates_[PRIORITY_COUNT];
    StateQueue appStates_[PRIORITY_COUNT];

    uint32_t lastUpdateTime_;
    int lastPatternType_;

    LED_SERVICE_DECLARE_LOCK(lock_);

    bool getActivePattern(LEDPattern* pattern) {
        LEDState* const sysState = activeState(sysStates_); // System state
        LEDState* const appState = activeState(appStates_); // Application state
        LEDState* state = nullptr; // Active state
        if (sysState) {
            if (appState) {
                // System state overrides application state with same priority
                state = (sysState->priority >= appState->priority) ? sysState : appState;
            } else {
                state = sysState;
            }
        } else if (appState) {
            state = appState;
        } else {
            return false;
        }
        // Do not use assignment operator here
        pattern->type = state->pattern.type;
        pattern->speed = state->pattern.speed;
        pattern->color = state->pattern.color;
        return true;
    }

    StateQueue& stateQueue(int source, int priority) {
        SPARK_ASSERT((source == LED_SOURCE_SYSTEM || source == LED_SOURCE_APPLICATION) &&
                (priority == LED_PRIORITY_BACKGROUND || priority == LED_PRIORITY_NORMAL ||
                priority == LED_PRIORITY_IMPORTANT || priority == LED_PRIORITY_CRITICAL));
        return (source == LED_SOURCE_SYSTEM) ? sysStates_[priority] : appStates_[priority];
    }

    static LEDState* activeState(const StateQueue states[]) {
        for (int i = PRIORITY_COUNT - 1; i >= 0; --i) { // Starting from highest priority
            LEDState* s = states[i].front();
            if (s) {
                return s;
            }
        }
        return nullptr;
    }
};

LEDService ledService;

} // namespace

void led_enter_state(LEDState* state, int source, int priority, void* reserved) {
    ledService.enterState(state, source, priority);
}

void led_leave_state(LEDState* state, void* reserved) {
    ledService.leaveState(state);
}

void led_start_signal(int signal, int priority, void* reserved) {
    ledService.startSignal(signal, priority);
}

void led_stop_signal(int signal, void* reserved) {
    ledService.stopSignal(signal);
}

void led_set_signal_theme(const LEDSignalTheme* theme, int flags, void* reserved) {
    ledService.setSignalTheme(theme, flags);
}

void led_get_signal_theme(LEDSignalTheme* theme, void* reserved) {
    ledService.getSignalTheme(theme);
}

void led_enable_updates(void* reserved) {
    ledService.enableUpdates();
}

void led_disable_updates(void* reserved) {
    ledService.disableUpdates();
}

void led_update(system_tick_t time, void* reserved) {
    ledService.update(time);
}
