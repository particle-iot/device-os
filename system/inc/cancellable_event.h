#pragma once

#include "protocol_defs.h"

using particle::protocol::message_handle_t;
using particle::protocol::MESSAGE_HANDLE_INVALID;


/**
 * Sends an event, and cancels the previous event sent if there is one pending. This is particularly useful with
 * transports that have a long timeout for sent messages where the content of the event may become out of date.
 * (This is a simple implementation of a TTL, where publishing the same event discards a previously sent one.)
 */
class SendEventCancelPrevious {

    message_handle_t previous_event;

public:

    SendEventCancelPrevious() : previous_event(MESSAGE_HANDLE_INVALID) {};

    int send_event(const char* event_name, const char* data, int ttl, int flags) {
        // make it asynchronous so that we do not block the application thread.

        clear_previous_event();
        spark_send_event_data extra = { .size = sizeof(data) };
        extra.handler_callback = event_completion_handler;
        extra.handler_data = this;
        int error = spark_send_event(event_name, data, ttl, flags, &extra);
        if (!error) {
            previous_event = extra.message_sent;
        }
        return error;
    }

protected:
    virtual void on_event_complete(int error) { };

private:

    void event_complete(int error) {
        previous_event = MESSAGE_HANDLE_INVALID;
        on_event_complete(error);
    }

    static void event_completion_handler(int error, const void* data, void* callback_data, void* reserved) {
        const auto target = reinterpret_cast<SendEventCancelPrevious*>(callback_data);
        if (target) {
            target->event_complete(error);
        }
    }

    void clear_previous_event() {
        if (previous_event != MESSAGE_HANDLE_INVALID) {
            spark_protocol_command(sp, ProtocolCommands::CANCEL_MESSAGE, previous_event, nullptr);
            previous_event = MESSAGE_HANDLE_INVALID;
        }
    }
};
