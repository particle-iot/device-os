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

#pragma once

#include "protocol_selector.h"

#if HAL_PLATFORM_CLOUD_UDP && PARTICLE_PROTOCOL

#include <string.h>
#include "protocol_defs.h"
#include "message_channel.h"
#include "messages.h"
#include "spark_descriptor.h"
#include "protocol.h"
#include "dtls_message_channel.h"
#include "coap_channel.h"
#include "eckeygen.h"
#include <limits>
#include "logging.h"
#include "protocol_mixin.h"

namespace particle {
namespace protocol {


class DTLSProtocol : public ProtocolMixin<DTLSProtocol, Protocol>
{
    friend class ProtocolMixin<DTLSProtocol, Protocol>;

    CoAPChannel<CoAPReliableChannel<DTLSMessageChannel, decltype(SparkCallbacks::millis)>> channel;


    static void handle_seed(const uint8_t* data, size_t len)
    {

    }

    uint8_t device_id[12];

public:
    // todo - this a duplicate of LightSSLProtocol - factor out

    DTLSProtocol() : ProtocolMixin(channel) {}

    void init(const char *id,
              const SparkKeys &keys,
              const SparkCallbacks &callbacks,
              const SparkDescriptor &descriptor) override;

    size_t build_hello(Message& message, uint8_t flags) override
    {
        product_details_t deets;
        deets.size = sizeof(deets);
        get_product_details(deets);
        size_t len = Messages::hello(message.buf(), 0,
                flags, PLATFORM_ID, deets.product_id,
                deets.product_version, true,
                device_id, sizeof(device_id));
        return len;
    }

    void wake()
    {
        ping();
    }

    int get_status(protocol_status* status) const override {
        SPARK_ASSERT(status);
        status->flags = 0;
        if (channel.has_unacknowledged_client_requests()) {
            status->flags |= PROTOCOL_STATUS_HAS_PENDING_CLIENT_MESSAGES;
        }
        return NO_ERROR;
    }

    bool has_unacknowledged_requests() {
        return channel.has_unacknowledged_requests();
    }

    void log_unacknowledged_requests() {
        LOG(INFO, "All Confirmed messages sent: client(%s) server(%s)",
            channel.client_messages().has_messages() ? "no" : "yes",
            channel.server_messages().has_unacknowledged_requests() ? "no" : "yes");
    }

    void clear_unacknowledged_requests() {
        // this is empty because the original DTLSProtocol::wait_confirmable method did not clear unacknowledged requests after a timeout
        // this should be reviewed and reconsidered so we are sure this is the correct behavior.
        // ack_handlers.clear();
    }

    bool cancel_message(message_handle_t msg) {
        return channel.cancel_message(msg);
    }
};



}}

#endif // HAL_PLATFORM_CLOUD_UDP && PARTICLE_PROTOCOL
