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
#include <vector>

namespace {

struct SystemEventSubscription {

    system_event_t events;
    system_event_handler_t* handler;

    SystemEventSubscription() : SystemEventSubscription(0, nullptr) {}
    SystemEventSubscription(system_event_t e, system_event_handler_t* h) :
    events(e), handler(h) {}

    inline bool matchesHandler(system_event_handler_t* matchHandler) const
    {
        return (matchHandler==nullptr) || (matchHandler==handler);
    }

    inline bool matchesEvent(system_event_t matchEvents) const
    {
        return (events&matchEvents)!=0;
    }

    bool matches(const SystemEventSubscription& subscription) const
    {
        return matchesHandler(subscription.handler) && matchesEvent(subscription.events);
    }

    void notify(system_event_t event, uint32_t data, void* pointer) const
    {
        if (matchesEvent(event))
            handler(event, data, pointer);
    }
};

// for now a simple implementation
std::vector<SystemEventSubscription> subscriptions;

void system_notify_event_impl(system_event_t event, uint32_t data, void* pointer, void (*fn)(void* data), void* fndata) {
    for (const SystemEventSubscription& subscription : subscriptions) {
        subscription.notify(event, data, pointer);
    }
    if (fn) {
        fn(fndata);
    }
}

void system_notify_event_async(system_event_t event, uint32_t data, void* pointer, void (*fn)(void* data), void* fndata) {
    // run event notifications on the application thread
    APPLICATION_THREAD_CONTEXT_ASYNC(system_notify_event_async(event, data, pointer, fn, fndata));
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
 * @param reserved  Set to NULL.
 * @return {@code 0} if the system event handlers were registered successfully. Non-zero otherwise.
 */
int system_subscribe_event(system_event_t events, system_event_handler_t* handler, void* reserved)
{
    size_t count = subscriptions.size();
    subscriptions.push_back(SystemEventSubscription(events, handler));
    return subscriptions.size()==count+1 ? 0 : -1;
}

/**
 * Unsubscribes a handler from the given events.
 * @param handler   The handler that will be unsubscribed.
 * @param reserved  Set to NULL.
 */
void system_unsubscribe_event(system_event_t events, system_event_handler_t* handler, void* reserved)
{
}

void system_notify_event(system_event_t event, uint32_t data, void* pointer, void (*fn)(void* data), void* fndata,
        unsigned flags) {
    // TODO: Add an API that would allow user applications to control which event handlers can be
    // executed synchronously, possibly in the context of an ISR
    if (flags & NOTIFY_SYNCHRONOUSLY) {
        system_notify_event_impl(event, data, pointer, fn, fndata);
    } else if (HAL_IsISR()) {
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
