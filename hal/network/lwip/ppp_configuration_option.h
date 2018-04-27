/*
 * Copyright (c) 2018 Particle Industries, Inc.  All rights reserved.
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

#ifndef HAL_NETWORK_LWIP_PPP_CONFIGURATION_OPTION_H
#define HAL_NETWORK_LWIP_PPP_CONFIGURATION_OPTION_H

#ifdef __cplusplus

namespace particle { namespace net { namespace ppp {

enum ConfigurationOptionState {
  CONFIGURATION_OPTION_STATE_NONE = 0,
  CONFIGURATION_OPTION_STATE_REQ  = 1,
  CONFIGURATION_OPTION_STATE_ACK  = 2,
  CONFIGURATION_OPTION_STATE_NAK  = 3,
  CONFIGURATION_OPTION_STATE_REJ  = 4,
  CONFIGURATION_OPTION_STATE_ERR  = 5
};

enum ConfigurationOptionFlags {
  CONFIGURATION_OPTION_FLAG_REQUEST = 0x01
};

struct ConfigurationOption {
  ConfigurationOption() = default;
  ConfigurationOption(int _id, size_t _length)
      : id(_id),
        length(_length) {
  }

  virtual void reset() {
    stateLocal = CONFIGURATION_OPTION_STATE_NONE;
    statePeer = CONFIGURATION_OPTION_STATE_NONE;
  }

  virtual bool validate(uint8_t* buf, size_t len) = 0;

  /* Local -> Remote */
  virtual int sendConfigureReq(uint8_t* buf, size_t len) = 0;
  virtual int recvConfigureRej(uint8_t* buf, size_t len) = 0;
  virtual int recvConfigureAck(uint8_t* buf, size_t len) = 0;
  virtual int recvConfigureNak(uint8_t* buf, size_t len) = 0;

  /* Remote -> Local */
  virtual int recvConfigureReq(uint8_t* buf, size_t len) = 0;
  virtual int sendConfigureRej(uint8_t* buf, size_t len) = 0;
  virtual int sendConfigureAck(uint8_t* buf, size_t len) = 0;
  virtual int sendConfigureNak(uint8_t* buf, size_t len) = 0;

  int id = 0;
  size_t length = 0;

  ConfigurationOptionState stateLocal = CONFIGURATION_OPTION_STATE_NONE;
  ConfigurationOptionState statePeer = CONFIGURATION_OPTION_STATE_NONE;

  unsigned int flagsLocal = 0;
  unsigned int flagsPeer = 0;

  ConfigurationOption* next = nullptr;
};

} } } /* namespace particle::net::ppp */

#endif /* __cplusplus */

#endif /* HAL_NETWORK_LWIP_PPP_CONFIGURATION_OPTION_H */
