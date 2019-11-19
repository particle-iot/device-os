/*
 * Copyright (c) 2019 Particle Industries, Inc.  All rights reserved.
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
#pragma once

#include "protocol_defs.h"

namespace particle { namespace protocol {

/**
 * On the majority of platforms, only one type of protocol is instantiated. A smart compiler may choose to remove
 * the virtual methods and optimize them, but that seems unlikely.  So for functionality that would ordinarily be
 * virtual to allow for variance in implementation is brought in here and templated.
 */

template <typename Subclass, typename Superclass>
class ProtocolMixin : public Superclass
{
protected:

    ProtocolMixin(MessageChannel& channel) : Protocol(channel) {}

    inline Subclass* subclass() {
        return reinterpret_cast<Subclass*>(this);
    }

public:


    virtual int command(ProtocolCommands::Enum command, uint32_t data) override
    {
        int result = UNKNOWN;
        switch (command)
        {
            case ProtocolCommands::SLEEP:
                result = wait_confirmable();
                break;
            case ProtocolCommands::DISCONNECT:
                result = wait_confirmable();
                this->ack_handlers.clear();
                break;
            case ProtocolCommands::WAKE:
                subclass()->wake();
                result = NO_ERROR;
                break;
            case ProtocolCommands::TERMINATE:
                this->ack_handlers.clear();
                result = NO_ERROR;
                break;
            case ProtocolCommands::FORCE_PING: {
                if (!this->pinger.is_expecting_ping_ack()) {
                    LOG(INFO, "Forcing a cloud ping");
                    this->pinger.process(std::numeric_limits<system_tick_t>::max(), [this] {
                        return this->ping(true);
                    });
                }
                break;
            }
            case ProtocolCommands::CANCEL_MESSAGE: {
                subclass()->cancel_message(message_handle_t(data));
            }
        }
        return result;
    }

    int wait_confirmable(uint32_t timeout=60000, uint32_t minimum_delay=0)
    {
        system_tick_t start = this->millis();
        LOG(INFO, "Waiting for Confirmed messages to be sent.");
        ProtocolError err = NO_ERROR;
        // FIXME: Additionally wait for 1 second before going into sleep to give
        // a chance for some requests to arrive (e.g. application describe request)
        while ((subclass()->has_unacknowledged_requests() && (this->millis()-start)<timeout) ||
                (this->millis() - start) <= minimum_delay)
        {
            CoAPMessageType::Enum message;
            err = this->event_loop(message);
            if (err)
            {
                LOG(WARN, "error receiving acknowledgements: %d", err);
                break;
            }
        }

        if (err == ProtocolError::NO_ERROR && subclass()->has_unacknowledged_requests())
        {
            err = ProtocolError::MESSAGE_TIMEOUT;
            LOG(WARN, "Timeout while waiting for confirmable messages to be processed");
        }

        subclass()->log_unacknowledged_requests();
        subclass()->clear_unacknowledged_requests();
        return (int)err;
    }

    bool has_unacknoweldged_requests();
    void log_unacknowledged_requests();
    void clear_unacknowledged_requests();

};

}} // namespace particle::protocol
