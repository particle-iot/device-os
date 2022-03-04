/**
 ******************************************************************************
  Copyright (c) 2013-2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#include "system_event.h"
#include "system_threading.h"
#include "interrupts_hal.h"
#include "system_task.h"
#include <stdint.h>
#include "spark_wiring_vector.h"
#include "spark_wiring_interrupts.h"
#include "spark_wiring_thread.h"
#include <algorithm>

namespace {

using namespace particle;

struct SystemEventSubscription {

    system_event_t events;
    system_event_handler_t* handler;
    SystemEventContext context;

    SystemEventSubscription()
            : SystemEventSubscription(0, nullptr) {
    }
    SystemEventSubscription(system_event_t e, system_event_handler_t* h, const SystemEventContext* c = nullptr)
            : events(e), handler(h), context{} {
        if (c) {
            auto size = std::min(c->size, (uint16_t)sizeof(SystemEventContext));
            memcpy(&context, c, size);
        }
    }

    inline bool matchesHandler(system_event_handler_t* matchHandler) const {
        return (matchHandler == nullptr) || (matchHandler == handler);
    }

    inline bool matchesEvent(system_event_t matchEvents) const {
        return (events&matchEvents) != 0;
    }

    bool matches(const SystemEventSubscription& subscription) const {
        return matchesHandler(subscription.handler) && matchesEvent(subscription.events);
    }

    bool matchesContext(const SystemEventContext* ctx) const {
        return (ctx != nullptr) && (ctx->callable == context.callable);
    }

    void notify(system_event_t event, uint32_t data, void* pointer) {
        if (matchesEvent(event)) {
            handler(event, data, pointer, &context);
        }
    }
};

// for now a simple implementation
spark::Vector<SystemEventSubscription> subscriptions;
#if PLATFORM_THREADING
RecursiveMutex sSubscriptionsMutex;
#endif // PLATFORM_THREADING

void system_notify_event_impl(system_event_t event, uint32_t data, void* pointer, void (*fn)(void* data), void* fndata) {
    for (SystemEventSubscription& subscription : subscriptions) {
        subscription.notify(event, data, pointer);
    }
    if (fn) {
        fn(fndata);
    }
}

void system_notify_event_async(system_event_t event, uint32_t data, void* pointer, void (*fn)(void* data), void* fndata) {
    // run event notifications on the application thread
    APPLICATION_THREAD_CONTEXT_ASYNC(system_notify_event_async(event, data, pointer, fn, fndata));
#if PLATFORM_THREADING
    std::lock_guard<RecursiveMutex> lk(sSubscriptionsMutex);
#endif // PLATFORM_THREADING
    system_notify_event_impl(event, data, pointer, fn, fndata);
}

class SystemEventTask : public ISRTaskQueue::Task {
    system_event_t event_;
    uint32_t data_;
    void* pointer_;
    void (*fn_)(void* data);
    void* fndata_;

    /**
     * @param task  The task to execute. It is an instance of SystemEventTask.
     */
    static void execute(Task* task) {
        auto that = reinterpret_cast<SystemEventTask*>(task);
        that->notify();
    }

    /**
     * Notify the system event encoded in this class.
     */
    void notify() {
        system_notify_event_async(event_, data_, pointer_, fn_, fndata_);
        system_pool_free(this, nullptr);
    }

public:

    SystemEventTask(system_event_t event, uint32_t data, void* pointer, void (*fn)(void* data), void* fndata) {
        event_ = event;
        data_ = data;
        pointer_ = pointer;
        fn_ = fn;
        fndata_ = fndata;
        func = execute;
    }
};

} // unnamed

/**
 * Subscribes to the system events given
 * @param events    One or more system events. Multiple system events are specified using the + operator.
 * @param handler   The system handler function to call.
 * @param context   Context along with the handler function.
 * @return {@code 0} if the system event handlers were registered successfully. Non-zero otherwise.
 */
int system_subscribe_event(system_event_t events, system_event_handler_t* handler, SystemEventContext* context) {
    // NOTE: using both mutex and ATOMIC_BLOCK, as some of the events may be generated directly out of an ISR.
    // Modification of subscriptions normally happens from thread context, so for events generated outside ISR
    // context, only mutex acquisition is sufficient to keep things thread safe (see system_notify_event_async())
#if PLATFORM_THREADING
    std::lock_guard<RecursiveMutex> lk(sSubscriptionsMutex);
#endif // PLATFORM_THREADING
    int r = 0;
    ATOMIC_BLOCK() {
        r = subscriptions.append(SystemEventSubscription(events, handler, context)) ? 0 : SYSTEM_ERROR_NO_MEMORY;
    }
    return r;
}

/**
 * Unsubscribes a handler from the given events.
 * @param handler   The handler that will be unsubscribed.
 * @param reserved  Set to nullptr.
 */
void system_unsubscribe_event(system_event_t events, system_event_handler_t* handler, const SystemEventContext* context) {
    // NOTE: using both mutex and ATOMIC_BLOCK, as some of the events may be generated directly out of an ISR.
    // Modification of subscriptions normally happens from thread context, so for events generated outside ISR
    // context, only mutex acquisition is sufficient to keep things thread safe (see system_notify_event_async())
#if PLATFORM_THREADING
    std::lock_guard<RecursiveMutex> lk(sSubscriptionsMutex);
#endif // PLATFORM_THREADING
    ATOMIC_BLOCK() {
        auto it = std::remove_if(subscriptions.begin(), subscriptions.end(), [events, handler, context](const SystemEventSubscription& sub) {
            if (!sub.matchesEvent(events)) {
                return false;
            }
            // Remove all subscriptions for this event
            if (!handler || sub.matchesHandler(handler)) {
                if (!context || sub.matchesContext(context)) {
                    return true;
                }
            }
            return false;
        });
        if (it != subscriptions.end()) {
            // Call destructors
            for (auto iter = it; iter != subscriptions.end(); iter++) {
                if (iter->context.callable && iter->context.destructor) {
                    iter->context.destructor(iter->context.callable);
                }
            }
            subscriptions.removeAt(it - subscriptions.begin(), subscriptions.end() - it);
        }
    }
}

void system_notify_event(system_event_t event, uint32_t data, void* pointer, void (*fn)(void* data), void* fndata,
        unsigned flags) {
    // TODO: Add an API that would allow user applications to control which event handlers can be
    // executed synchronously, possibly in the context of an ISR
    if (flags & NOTIFY_SYNCHRONOUSLY) {
        system_notify_event_impl(event, data, pointer, fn, fndata);
    } else if (hal_interrupt_is_isr()) {
        void* space = (system_pool_alloc(sizeof(SystemEventTask), nullptr));
        if (space) {
            auto task = new (space) SystemEventTask(event, data, pointer, fn, fndata);
            SystemISRTaskQueue.enqueue(task);
        };
    } else {
        system_notify_event_async(event, data, pointer, fn, fndata);
    }
}

void system_notify_time_changed(uint32_t data, void* reserved, void* reserved1) {
    system_notify_event(time_changed, data);
}
