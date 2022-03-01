/**
 ******************************************************************************
 Copyright (c) 2016 Particle Industries, Inc.  All rights reserved.

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

#include "lightssl_protocol.h"

#if HAL_PLATFORM_CLOUD_TCP

namespace particle { namespace protocol {

int LightSSLProtocol::command(ProtocolCommands::Enum command, uint32_t value, const void* data)
{
  switch (command) {
  case ProtocolCommands::SLEEP: // Deprecated
  case ProtocolCommands::DISCONNECT: {
    int r = ProtocolError::NO_ERROR;
    unsigned timeout = DEFAULT_DISCONNECT_COMMAND_TIMEOUT;
    if (data) {
      const auto d = (const spark_disconnect_command*)data;
      if (d->timeout != 0) {
        timeout = d->timeout;
      }
      if (d->cloud_reason != CLOUD_DISCONNECT_REASON_NONE) {
        r = send_goodbye((cloud_disconnect_reason)d->cloud_reason, (network_disconnect_reason)d->network_reason,
            (System_Reset_Reason)d->reset_reason, d->sleep_duration);
      }
    }
    if (r == ProtocolError::NO_ERROR) {
      r = wait_confirmable(timeout);
    }
    reset();
    return r;
  }
  case ProtocolCommands::TERMINATE: {
    reset();
    return ProtocolError::NO_ERROR;
  }
  default:
    return ProtocolError::UNKNOWN;
  }
}

int LightSSLProtocol::wait_confirmable(uint32_t timeout)
{
  ProtocolError err = NO_ERROR;

  if (ack_handlers.size() != 0) {
    system_tick_t start = millis();
    LOG(INFO, "Waiting for Confirmed messages to be ACKed.");

    while (((ack_handlers.size() != 0) && (millis()-start)<timeout))
    {
      CoAPMessageType::Enum message;
      err = event_loop(message);
      if (err)
      {
        LOG(WARN, "error receiving acknowledgements");
        break;
      }
    }
    LOG(INFO, "All Confirmed messages sent: %s",
        (ack_handlers.size() != 0) ? "no" : "yes");

    ack_handlers.clear();
  }

  return (int)err;
}



}}

#endif
