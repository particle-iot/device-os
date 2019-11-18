#pragma once

#include "protocol_defs.h"

using particle::protocol::message_handle_t;
using particle::protocol::MESSAGE_HANDLE_INVALID;


/**
 * Sends an event, and cancels the previous event sent. This is particularly useful with
 * transports that have a long timeout for sent messages
 */
class SendEventCancelPrevious {

    message_handle_t previous_event;

public:

    SendEventCancelPrevious() : previous_event(MESSAGE_HANDLE_INVALID) {};

    int send_event(const char* event_name, const char* data, int ttl, int flags) {
        // make it asynchronous so that we do not block the application thread.

        clear_previous_event();
        spark_send_event_data extra = { .size = sizeof(data) };
        int error = spark_send_event(event_name, data, ttl, flags, &extra);
        if (!error) {
            previous_event = extra.message_sent;
        }
        return error;
    }

private:

    void clear_previous_event() {
        if (previous_event != MESSAGE_HANDLE_INVALID) {
            spark_protocol_command(sp, ProtocolCommands::CANCEL_MESSAGE, previous_event, nullptr);
            previous_event = MESSAGE_HANDLE_INVALID;
        }
    }
};
