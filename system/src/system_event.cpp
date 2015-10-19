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
#include <stdint.h>
#include <vector>


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

/**
 * Notifes all subscribers about an event.
 * @param event
 * @param data
 * @param pointer
 */
void system_notify_event(system_event_t event, uint32_t data, void* pointer, void (*fn)())
{
    APPLICATION_THREAD_CONTEXT_ASYNC(system_notify_event(event, data, pointer, fn));
    // run event notifications on the application thread

    for (const SystemEventSubscription& subscription : subscriptions)
    {
        subscription.notify(event, data, pointer);
    }
    if (fn)
        fn();
}



