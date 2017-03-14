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

namespace particle { namespace protocol {
int LightSSLProtocol::command(ProtocolCommands::Enum command, uint32_t data)
{
  if (command == ProtocolCommands::SLEEP || command == ProtocolCommands::DISCONNECT) {
    return this->wait_confirmable();
  }

  return 1;
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
