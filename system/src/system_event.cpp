
#include "system_event.h"
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
void system_notify_event(system_event_t event, uint32_t data, void* pointer)
{
    for (const SystemEventSubscription& subscription : subscriptions)
    {
        subscription.notify(event, data, pointer);
    }
}



